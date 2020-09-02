/*
TODO:
	Arrows
	Particle systems for arrows and dust
	intro screen for role before game starts

CLEANUP:
	Remove flip from player?
	Player hurt animation thingy
	Outline player when you can attack?

BUGS:
	Player render with transparent overlap
*/

#pragma comment(lib, "XInput.lib") 

#define GLEW_STATIC
#include <glew/glew.h>
#include <gl/GL.h>
#include "gl/wglext.h"
#include <Shlwapi.h>
#include <mmreg.h>
#include "intrin.h"
#include <Windows.h>
#include <dsound.h>

#include "Bang.h"
#include "Input.h"
#include "Render.h"
#include "TextureAtlas.h"
#include "Interface.h"

struct PlatformWorkQueue
{
	u32 volatile CompletionTarget;
	u32 volatile CompletionCount;
	u32 volatile NextEntryToWrite;
	u32 volatile NextEntryToRead;
	HANDLE Semaphore;

	WorkQueueEntry entries[QUEUE_ENTRIES];
};

struct PlatformWindow
{
	float width;
	float height;

	HWND handle;
	HGLRC rc;
	HDC dc;
};

struct win32_thread_info
{
	u32 ThreadIndex;
	PlatformWorkQueue* queue;
};

struct win32_sound
{
	u32 samples_per_second;
	u32 buffer_size;
	u32 bytes_per_sample;
	u32 safety_bytes;

	LPDIRECTSOUNDBUFFER primary_buffer;
	LPDIRECTSOUNDBUFFER secondary_buffer;
};

Interface g_interface = {};
GameNetState g_net = {};
GameState g_state = {};
GameTransState g_transstate = {};
GameInput g_input = {};

#include "Logger.cpp"
#include "Memory.cpp"
#include "ConfigFile.cpp"
#include "Assets.cpp"
#include "Input.cpp"
#include "Camera.cpp"
#include "ParticleSystem.cpp"
#include "Render.cpp"
#include "EntityList.cpp"
#include "NetcodeCommon.cpp"
#include "Collision.cpp"
#include "Level.cpp"
#include "Interface.cpp"
#include "Player.cpp"
#include "Beer.cpp"
#include "Arrows.cpp"
#include "Client.cpp"
#include "Win32Common.cpp"

static void SwapBuffers(PlatformWindow* pWindow)
{
	SwapBuffers(pWindow->dc);
}

//
// Main loop
//
const LPCWSTR WINDOW_CLASS = L"Bang";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CreateOpenGLWindow(PlatformWindow* pForm, HINSTANCE hInstance, u32 pWidth, u32 pHeight, LPCWSTR pTitle, bool pFullscreen)
{
	WNDCLASSW wcex = {};
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = WINDOW_CLASS;
	RegisterClassW(&wcex);

	const long style = WS_OVERLAPPEDWINDOW;
	HWND fakeHwnd = CreateWindowW(WINDOW_CLASS, L"Fake Window", style, 0, 0, 1, 1, NULL, NULL, hInstance, NULL);

	HDC fakeDc = GetDC(fakeHwnd);

	PIXELFORMATDESCRIPTOR fakePfd = {};
	fakePfd.nSize = sizeof(fakePfd);
	fakePfd.nVersion = 1;
	fakePfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	fakePfd.iPixelType = PFD_TYPE_RGBA;
	fakePfd.cColorBits = 32;
	fakePfd.cAlphaBits = 8;
	fakePfd.cDepthBits = 24;

	int id = ChoosePixelFormat(fakeDc, &fakePfd);
	if (!id) return false;

	if (!SetPixelFormat(fakeDc, id, &fakePfd)) return false;

	HGLRC fakeRc = wglCreateContext(fakeDc);
	if (!fakeRc) return false;

	if (!wglMakeCurrent(fakeDc, fakeRc)) return false;

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
	wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
	if (!wglChoosePixelFormatARB) return false;

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
	wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
	if (!wglCreateContextAttribsARB) return false;

	u32 x = 0;
	u32 y = 0;
	if (!pFullscreen)
	{
		RECT rect = { 0L, 0L, (LONG)pWidth, (LONG)pHeight };
		AdjustWindowRect(&rect, style, false);
		pForm->width = (float)(rect.right - rect.left);
		pForm->height = (float)(rect.bottom - rect.top);

		RECT primaryDisplaySize;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &primaryDisplaySize, 0);	// system taskbar and application desktop toolbars not included
		x = (u32)(primaryDisplaySize.right - pForm->width) / 2;
		y = (u32)(primaryDisplaySize.bottom - pForm->height) / 2;
	}

	pForm->handle = CreateWindowW(WINDOW_CLASS, pTitle, style, x, y, (int)pForm->width, (int)pForm->height, NULL, NULL, hInstance, NULL);
	pForm->dc = GetDC(pForm->handle);

	if (pFullscreen)
	{
		SetWindowLong(pForm->handle, GWL_STYLE, wcex.style & ~(WS_CAPTION | WS_THICKFRAME));

		// On expand, if we're given a window_rect, grow to it, otherwise do
		// not resize.
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(pForm->handle, MONITOR_DEFAULTTONEAREST), &mi);
		pForm->width = (float)(mi.rcMonitor.right - mi.rcMonitor.left);
		pForm->height = (float)(mi.rcMonitor.bottom - mi.rcMonitor.top);

		SetWindowPos(pForm->handle, NULL, mi.rcMonitor.left, mi.rcMonitor.top,
			(int)pForm->width, (int)pForm->height,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	const int pixelAttribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_SAMPLES_ARB, 4,
		0
	};

	int pixelFormatID; UINT numFormats;
	const bool status = wglChoosePixelFormatARB(pForm->dc, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);
	if (!status || !numFormats) return false;

	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(pForm->dc, pixelFormatID, sizeof(PFD), &PFD);
	SetPixelFormat(pForm->dc, pixelFormatID, &PFD);

	const int major_min = 4, minor_min = 0;
	const int contextAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, major_min,
		WGL_CONTEXT_MINOR_VERSION_ARB, minor_min,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		//		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
				0
	};

	pForm->rc = wglCreateContextAttribsARB(pForm->dc, 0, contextAttribs);
	if (!pForm->rc) return false;

	// delete temporary context and window
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(fakeRc);
	ReleaseDC(fakeHwnd, fakeDc);
	DestroyWindow(fakeHwnd);

	if (!wglMakeCurrent(pForm->dc, pForm->rc)) return false;

	return true;
}

static void DestroyGlWindow(PlatformWindow* pForm)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(pForm->rc);
	ReleaseDC(pForm->handle, pForm->dc);
	DestroyWindow(pForm->handle);
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

win32_sound Win32InitDirectSound(HWND pWindow, uint32_t pSamplesPerSecond, uint32_t pSeconds, u32 pBytesPerSample)
{
	win32_sound sound = {};
	sound.samples_per_second = pSamplesPerSecond;
	sound.buffer_size = pSamplesPerSecond * pBytesPerSample * pSeconds;
	sound.bytes_per_sample = pBytesPerSample;
	sound.safety_bytes = (sound.samples_per_second * sound.bytes_per_sample / UPDATE_FREQUENCY) / 3;
	HMODULE dSound = LoadLibraryA("dsound.dll");
	if (dSound)
	{
		direct_sound_create* create = (direct_sound_create*)GetProcAddress(dSound, "DirectSoundCreate");
		LPDIRECTSOUND direct_sound;
		if (create && SUCCEEDED(create(0, &direct_sound, 0)))
		{
			WAVEFORMATEX format = {};
			format.wFormatTag = WAVE_FORMAT_PCM;
			format.nChannels = 2;
			format.nSamplesPerSec = pSamplesPerSecond;
			format.wBitsPerSample = 16;
			format.nBlockAlign = (format.nChannels * format.wBitsPerSample) / 8;
			format.nAvgBytesPerSec = pSamplesPerSecond * format.nBlockAlign;

			if (!SUCCEEDED(direct_sound->SetCooperativeLevel(pWindow, DSSCL_PRIORITY))) return sound;

			DSBUFFERDESC bufferdescription = {};
			bufferdescription.dwSize = sizeof(bufferdescription);
			bufferdescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
			if (!SUCCEEDED(direct_sound->CreateSoundBuffer(&bufferdescription, &sound.primary_buffer, 0))) return sound;

			if (!SUCCEEDED(sound.primary_buffer->SetFormat(&format))) return sound;

			DSBUFFERDESC secondarybufferdesc = {};
			secondarybufferdesc.dwSize = sizeof(secondarybufferdesc);
			secondarybufferdesc.dwBufferBytes = sound.buffer_size;
			secondarybufferdesc.lpwfxFormat = &format;
			secondarybufferdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
			if (!SUCCEEDED(direct_sound->CreateSoundBuffer(&secondarybufferdesc, &sound.secondary_buffer, 0))) return sound;


		}
		else DisplayErrorMessage("Unable to load direct sound", ERROR_TYPE_Warning);
	}

	return sound;
}

void Win32GetInput(GameInput* pInput, HWND pHandle)
{
	memcpy(pInput->previous_keyboard_state, pInput->current_keyboard_state, 256);
	pInput->previous_controller_buttons = pInput->current_controller_buttons;

	GetKeyboardState((PBYTE)pInput->current_keyboard_state);

	POINT point;
	GetCursorPos(&point);
	ScreenToClient(pHandle, &point);

	pInput->mouse_position = V2((float)point.x, (float)point.y);

	XINPUT_GAMEPAD_EX state = {};
	DWORD result = g_input.XInputGetState(0, &state);
	if (result == ERROR_SUCCESS)
	{
		pInput->current_controller_buttons = state.wButtons;
		pInput->left_stick = V2((float)state.sThumbLX, (float)state.sThumbLY);
		pInput->right_stick = V2((float)state.sThumbRX, (float)state.sThumbRY);

		float length = HMM_LengthSquaredVec2(pInput->left_stick);
		if (length <= XINPUT_INPUT_DEADZONE * XINPUT_INPUT_DEADZONE)
		{
			pInput->left_stick = V2(0);
		}
		length = HMM_LengthSquaredVec2(pInput->right_stick);
		if (length <= XINPUT_INPUT_DEADZONE * XINPUT_INPUT_DEADZONE)
		{
			pInput->right_stick = V2(0);
		}
	}
}

static void RenderGameElements(GameState* pState, RenderState* pRender)
{
	if(g_interface.current_screen == SCREEN_Game)
	{
		RenderGame(pState, pRender);
	}

	SetZLayer(pRender, Z_LAYER_Player);
	for (u32 i = 0; i < pState->entities.end_index; i++)
	{
		Entity* e = pState->entities.entities[i];
		if (IsEntityValid(&pState->entities, e))
		{
			e->Render(pRender);
		}
	}
}

#define LOCAL_TESTING
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance,
					  _In_ LPWSTR    lpCmdLine,
					  _In_ int       nCmdShow)
{
	InitializeLogger();

	PlatformWindow form = {};
	g_state.form = &form;
	if (!CreateOpenGLWindow(g_state.form, hInstance, 720, 480, L"Bang", false))
	{
		MessageBoxA(nullptr, "Unable to initialize window", "Error", MB_OK);
		return 1;
	}
	ShowWindow(g_state.form->handle, nCmdShow);
	UpdateWindow(g_state.form->handle);

	PlatformWorkQueue work_queue = {};
	win32_thread_info threads[THREAD_COUNT];
	InitWorkQueue(&work_queue, threads, &g_state);

	LARGE_INTEGER i;
	QueryPerformanceFrequency(&i);
	double computer_frequency = double(i.QuadPart);

	glewExperimental = true; // Needed for core profile
	glewInit();

	//Memory initialization
	u32 world_size = Megabytes(8);
	void* world_memory = VirtualAlloc(NULL, world_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	ZeroMemory(world_memory, world_size);

	g_state.world_arena = CreateMemoryStack(world_memory, world_size);
	g_state.entities = GetEntityList(g_state.world_arena);
	g_state.config = LoadConfigFile(CONFIG_FILE_LOCATION, g_state.world_arena);
	g_state.screen_reset = BeginTemporaryMemory(g_state.world_arena);

	TransitionScreen(&g_state, &g_interface, SCREEN_MainMenu);

	u32 trans_size = Megabytes(64);
	void* trans_memory = VirtualAlloc(NULL, trans_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	ZeroMemory(trans_memory, trans_size);

	g_transstate.trans_arena = CreateMemoryStack(trans_memory, trans_size);
	g_transstate.render_state = new RenderState();


	//Renderer init
	InitializeRenderer(g_transstate.render_state);

	//Audio init
	win32_sound sound = Win32InitDirectSound(form.handle, 48000, 1, sizeof(s16) * 2);
	sound.secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);

	//Asset init
	LoadAssets(g_transstate.trans_arena, Megabytes(32));

	//Input init
	HINSTANCE xinput_dll;
	char system_path[MAX_PATH];
	GetSystemDirectoryA(system_path, sizeof(system_path));

	char xinput_path[MAX_PATH];
	PathCombineA(xinput_path, system_path, "xinput1_3.dll");

	FILE* f = fopen(xinput_path, "r");
	if (f)
	{
		fclose(f);
		xinput_dll = LoadLibraryA(xinput_path);
	}
	else
	{
		PathCombineA(xinput_path, system_path, "xinput1_4.dll");
		xinput_dll = LoadLibraryA(xinput_path);
	}
	assert(xinput_dll);

	//Assign it to getControllerData for easier use
	g_input.XInputGetState = (get_gamepad_ex)GetProcAddress(xinput_dll, (LPCSTR)100);
	g_input.layout = GetKeyboardLayout(0);

	GameTime time = {};
	time.time_scale = 1.0F;

	//Game loop
	u32 prediction_id = 0;
	bool soundvalid = false;
	u32 sample_index;
	MSG msg;
	g_state.is_running = true;
	while (g_state.is_running)
	{
		LARGE_INTEGER i2;
		QueryPerformanceCounter(&i2);
		__int64 start = i2.QuadPart;

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) g_state.is_running = false;
		}
		CheckForConfigUpdates(&g_state.config);

		Win32GetInput(&g_input, g_state.form->handle);
		Tick(&time);

		ProcessServerMessages(&g_net, prediction_id, time.delta_time);

		TemporaryMemoryHandle h = BeginTemporaryMemory(g_transstate.trans_arena);

		UpdateInterface(&g_state, &g_interface, time.delta_time);
		if(g_interface.current_screen == SCREEN_Game) UpdateGame(&g_state, time.delta_time, prediction_id);
		StepPhysics(&g_state.physics, ExpectedSecondsPerFrame);
		ProcessEvents(&g_state, &g_state.events);

		ProcessTaskCallbacks(&g_state.callbacks);

		v2 size = V2(g_state.form->width, g_state.form->height);
		BeginRenderPass(size, g_transstate.render_state);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderGameElements(&g_state, g_transstate.render_state);
		RenderRigidBodies(g_transstate.render_state, &g_state.physics);

		EndRenderPass(size, g_transstate.render_state);

		RenderInterface(&g_state, &g_interface, g_transstate.render_state);
		SwapBuffers(g_state.form);

		EndTemporaryMemory(h);

		{ //Process audio
			DWORD write_pointer;
			DWORD read_pointer;
			if (SUCCEEDED(sound.secondary_buffer->GetCurrentPosition(&read_pointer, &write_pointer)))
			{
				if (!soundvalid)
				{
					sample_index = write_pointer / sound.bytes_per_sample;
					soundvalid = true;
				}

				DWORD lock_pointer = (sample_index * sound.bytes_per_sample) % sound.buffer_size;

				//Check how far we should write to fill the next frame
				DWORD ExpectedBytesPerFrame = sound.samples_per_second * sound.bytes_per_sample / UPDATE_FREQUENCY;
				DWORD ExpectedFrameBoundaryByte = read_pointer + ExpectedBytesPerFrame;

				DWORD adjusted_write = write_pointer;
				if (adjusted_write < read_pointer) adjusted_write += sound.buffer_size;

				//Check if we are speedy enough to be frame perfect.
				DWORD target = 0;
				bool card_is_latent = (adjusted_write + sound.safety_bytes >= ExpectedFrameBoundaryByte);

				if (card_is_latent) target = write_pointer + ExpectedBytesPerFrame + sound.safety_bytes;
				else target = ExpectedFrameBoundaryByte + ExpectedBytesPerFrame;
				target = target % sound.buffer_size;

				//Calculate how much to write based on how much the audio card has played
				DWORD bytes_to_write;
				if (lock_pointer > target)
				{
					bytes_to_write = sound.buffer_size - lock_pointer;
					bytes_to_write += target;
				}
				else
				{
					bytes_to_write = target - lock_pointer;
				}

				void* region1; DWORD region1size;
				void* region2; DWORD region2size;
				if (SUCCEEDED(sound.secondary_buffer->Lock(lock_pointer, bytes_to_write, &region1, &region1size, &region2, &region2size, 0)))
				{
					//Write sound samples
					GameSoundBuffer buffer = {};
					buffer.samples_per_second = sound.samples_per_second;
					buffer.sample_count = region1size / sound.bytes_per_sample;
					buffer.data = region1;
					GameGetSoundSamples(&g_state, &g_transstate, &buffer);

					sample_index += buffer.sample_count;

					if (region2)
					{
						buffer.sample_count = region2size / sound.bytes_per_sample;
						buffer.data = region2;
						GameGetSoundSamples(&g_state, &g_transstate, &buffer);
						sample_index += buffer.sample_count;
					}

					sound.secondary_buffer->Unlock(region1, region1size, region2, region2size);
				}
			}
			else
			{
				soundvalid = false;
			}
		}

		prediction_id++;
		SleepUntilFrameEnd(start, computer_frequency);
	}

	ClientLeave l = {};
	l.client_id = g_net.client_id;
	u32 size = WriteMessage(g_net.buffer, &l, ClientLeave, CLIENT_MESSAGE_Leave);
	SocketSend(&g_net.send_socket, g_net.server_ip, g_net.buffer, size);

	NetEnd(&g_net);

	FreeAllAssets(g_transstate.assets);
	DisposeRenderState(g_transstate.render_state);
	DestroyGlWindow(g_state.form);
	CloseHandle(work_queue.Semaphore);
	UnregisterClassW(WINDOW_CLASS, hInstance);

	VirtualFree(world_memory, 0, MEM_RELEASE);
	VirtualFree(trans_memory, 0, MEM_RELEASE);

	LogInfo("Exiting \n\n");
	CloseLogger();

	return 0;
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED)
		{
			g_state.form->width = 0;
			g_state.form->height = 0;
		}
		else
		{
			g_state.form->width = (float)(lParam & 0xFFFF);
			g_state.form->height = (float)((lParam >> 16) & 0xFFFF);
		}

		glViewport(0, 0, (int)g_state.form->width, (int)g_state.form->height);
		return 0;
	}

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}