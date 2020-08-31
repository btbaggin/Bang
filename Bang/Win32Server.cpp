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

GameNetState g_net = {};
GameState g_state = {};
GameTransState g_transstate = {};

#include "Win32Common.cpp"
#include "Memory.cpp"
#include "Logger.cpp"
#include "Assets.cpp"
#include "Memory.cpp"
#include "ConfigFile.cpp"
#include "EntityList.cpp"
#include "NetcodeCommon.cpp"
#include "Collision.cpp"
#include "Player.cpp"
#include "Level.cpp"

static void SendMessageToAllConnectedClients(GameNetState* pState, u32 pSize)
{
	for (u32 i = 0; i < MAX_PLAYERS; i++)
	{
		Client* client = pState->clients + i;

		if (IsClientConnected(client))
		{
			SocketSend(&pState->listen_socket, client->endpoint, pState->buffer, pSize);
		}
	}
}

static void ResetClient(Client* pClient)
{
	pClient->endpoint = {};
	pClient->current_prediction_id = 0;
	pClient->name[0] = 0;
}

int main()
{
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

	u32 trans_size = Megabytes(32);
	void* trans_memory = VirtualAlloc(NULL, trans_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	ZeroMemory(trans_memory, trans_size);

	g_transstate.trans_arena = CreateMemoryStack(trans_memory, trans_size);

	GameTime gametime = {};
	gametime.time_scale = 1.0F;

	NetStart(&g_net, &g_net.listen_socket);

	IPEndpoint local_endpoint = {};
	local_endpoint.ip = INADDR_ANY;
	local_endpoint.port = PORT;
	BindSocket(&g_net.listen_socket, &local_endpoint);

	LogInfo("Server started on port %d", PORT);

	//Game loop
	g_state.is_running = true;
	while (g_state.is_running)
	{
		LARGE_INTEGER i2;
		QueryPerformanceCounter(&i2);
		__int64 start = i2.QuadPart;

		Tick(&gametime);

		TemporaryMemoryHandle h = BeginTemporaryMemory(g_transstate.trans_arena);

		u32 bytes_received = 0;
		IPEndpoint from = {};
		while (SocketReceive(&g_net.listen_socket, g_net.buffer, SOCKET_BUFFER_SIZE, &bytes_received, &from))
		{
			CLIENT_MESSAGES message = (CLIENT_MESSAGES)g_net.buffer[0];
			u8 client_id = g_net.buffer[1];

			Client* client = g_net.clients + client_id;

			char from_ip[50];
			EndpointToStr(from_ip, from);

			switch (message)
			{
			case CLIENT_MESSAGE_Join:
			{
				LogInfo("Received join message from %d (%s)", client_id, from_ip);
				JoinResult j = {};
				j.result = JOIN_STATUS_Ok;

				u8 slot = INT8_MAX;
				char name[NAME_LENGTH];
				if (!g_net.game_started)
				{
					ReadMessage(g_net.buffer, join, Join);
					strcpy(name, join.name);

					for (u8 i = 0; i < MAX_PLAYERS; ++i)
					{
						Client* c = g_net.clients + i;
						if (!IsClientConnected(c))
						{
							slot = i;
							break;
						}
					}
				}
				else
				{
					j.result = JOIN_STATUS_GameStarted;
				}

				if (slot < INT16_MAX)
				{
					j.slot = slot;
					u32 size = WriteMessage(g_net.buffer, &j, JoinResult, SERVER_MESSAGE_JoinResult);
					if (SocketSend(&g_net.listen_socket, from, g_net.buffer, size))
					{
						Client* client = g_net.clients + slot;
						client->endpoint = from;
						client->time_since_last_packet = 0.0F;
						client->current_prediction_id = 0;
						strcpy(client->name, name);

						JoinAnnoucement ja = {};
						for (u32 i = 0; i < MAX_PLAYERS; i++)
						{
							Client c = g_net.clients[i];
							strcpy(ja.names[i], c.name);
						}
						size = WriteMessage(g_net.buffer, &ja, JoinAnnoucement, SERVER_MESSAGE_JoinAnnouncement);
						SendMessageToAllConnectedClients(&g_net, size);
					}
					else
					{
						DisplayErrorMessage("Unable to send join reply", ERROR_TYPE_Warning);
					}
				}
				else
				{
					j.result = JOIN_STATUS_LobbyFull;
					u32 size = WriteMessage(g_net.buffer, &j, JoinResult, SERVER_MESSAGE_JoinResult);
					SocketSend(&g_net.listen_socket, from, g_net.buffer, size);
				}
			}
			break;

			case CLIENT_MESSAGE_Leave:
			{
				LogInfo("Received leave message from %d (%s)", client_id, from_ip);
				ReadMessage(g_net.buffer, l, ClientLeave);
				ResetClient(client);

				if (g_net.game_started)
				{
					Player* p = g_state.players.items + l.client_id;
					RemoveEntity(&g_state.entities, p->entity);
					RemoveRigidBody(p->entity, &g_state.physics);
				}

				u32 size = WriteMessage(g_net.buffer, &l, ClientLeave, SERVER_MESSAGE_LeaveAnnoucement);
				SendMessageToAllConnectedClients(&g_net, size);
			}
			break;

			case CLIENT_MESSAGE_GameStart:
			{
				LogInfo("Received game start message from %d (%s)", client_id, from_ip);
				g_net.game_started = true;
				u32 t = (u32)time(NULL);
				srand(t);

				g_state.map = PushStruct(g_state.world_arena, TiledMap);
				LoadTiledMap(g_state.map, "..\\..\\Resources\\level.json", g_transstate.trans_arena);

				//Get number of connected players
				u32 players = 0;
				for (u32 i = 0; i < MAX_PLAYERS; i++)
					if (IsClientConnected(g_net.clients + i)) players++;

				//Generate numbers for roles
				u32 numbers[MAX_PLAYERS];
				GenerateRandomNumbersWithNoDuplicates(numbers, players);

				//We need to displaye who the sherif is and teams to everyone
				GameStartAnnoucement s = {};
				s.time = t;
				for (u32 i = 0; i < MAX_PLAYERS; i++)
				{
					Client* c = g_net.clients + i;
					s.roles[i] = PLAYER_ROLE_Unknown;
					if (IsClientConnected(c)) 
					{
						Player* p = CreatePlayer(&g_state, &g_net, c->name);

						if (numbers[i] == 0) s.roles[i] = PLAYER_ROLE_Sheriff;
						s.teams[i] = (PLAYER_TEAMS)(i % PLAYER_TEAM_COUNT);
					}
				}

				//Generate your local role so its secret to everyone else
				for (u32 i = 0; i < MAX_PLAYERS; i++)
				{
					Client* c = g_net.clients + i;
					Player* p = g_state.players.items + i;
					if (IsClientConnected(c))
					{
						if (s.roles[i] == PLAYER_ROLE_Unknown)
						{
							// 1/4 players are deputies
							// 1/4 players are renegades
							// 1/2 players are outlaws
							if (numbers[i] < players / 4) s.roles[i] = PLAYER_ROLE_Deputy;
							else if (numbers[i] < players / 2) s.roles[i] = PLAYER_ROLE_Renegade;
							else s.roles[i] = PLAYER_ROLE_Outlaw;
						}
						p->role = s.roles[i];
						p->team = s.teams[i];

						u32 size = WriteMessage(g_net.buffer, &s, GameStartAnnoucement, SERVER_MESSAGE_GameStartAnnoucement);
						SocketSend(&g_net.listen_socket, c->endpoint, g_net.buffer, size);
						if(s.roles[i] != PLAYER_ROLE_Sheriff) s.roles[i] = PLAYER_ROLE_Unknown;
					}
				}
			}
			break;

			case CLIENT_MESSAGE_Input:
			{
				ReadMessage(g_net.buffer, i, ClientInput);

				client->current_prediction_id = i.prediction_id;
				client->dt = i.dt;
				client->input_flags = i.flags;

				Player* p = g_state.players.items + client_id;
				if(i.attack_choice >= 0) p->state.team_attack_choice = i.attack_choice;
				UpdatePlayer(p, i.dt, i.flags);
			}
			break;

			case CLIENT_MESSAGE_Ping:
				client->time_since_last_packet = 0;
				break;
			}
		}
		StepPhysics(&g_state.physics, ExpectedSecondsPerFrame);

		bool client_connected = false;
		for (u32 i = 0; i < MAX_PLAYERS; ++i)
		{
			Client* client = g_net.clients + i;
			if (IsClientConnected(client))
			{
				client_connected = true;
				client->time_since_last_packet += gametime.delta_time;
				if (client->time_since_last_packet > CLIENT_TIMEOUT)
				{
					char ip[50];
					EndpointToStr(ip, client->endpoint);
					LogInfo("Client %d (%s) has been inactive and is begin disconnected", i, ip);
					ResetClient(client);

					ClientLeave l;
					l.client_id = i;
					u32 size = WriteMessage(g_net.buffer, &l, ClientLeave, CLIENT_MESSAGE_Leave);
					SendMessageToAllConnectedClients(&g_net, size);
				}
			}
		}

		if (g_net.game_started)
		{
			SendCurrentGameState(&g_net, &g_state);

			if (!client_connected)
			{
				LogInfo("Ending game because no players are connected");
				g_net.game_started = false;
				ClearEntityList(&g_state.entities);
				g_state.physics.bodies.clear();
			}
		}

		ProcessTaskCallbacks(&g_state.callbacks);

		EndTemporaryMemory(h);

		SleepUntilFrameEnd(start, computer_frequency);
	}

	NetEnd(&g_net);

	CloseHandle(work_queue.Semaphore);

	VirtualFree(world_memory, 0, MEM_RELEASE);
	VirtualFree(trans_memory, 0, MEM_RELEASE);

	LogInfo("Exiting \n\n");
	CloseLogger();

	return 0;
}