#include "Textbox.cpp"
#include "Button.cpp"

static void LoadNextScreen()
{
	EndTemporaryMemory(g_state.game_reset);
	ClearEntityList(&g_state.entities);
	ClearAllSounds(&g_state, &g_transstate);

	g_interface.current_screen = g_interface.current_screen->CreateNextScreen(g_state.world_arena);
	g_interface.current_screen->Load(&g_state);
}
static void EndScreen(float pTransition)
{
	g_interface.transition_time = pTransition;
	if (pTransition <= 0) LoadNextScreen();
}
static inline bool IsTransitioning()
{
	return g_interface.transition_time > 0;
}

static float GetModalSize(MODAL_SIZES pSize)
{
	float screen_width = g_state.form->width - (g_state.form->width / 5.0F);
	switch (pSize)
	{
	case MODAL_SIZE_Third:
		return screen_width / 3.0F;
	case MODAL_SIZE_Half:
		return screen_width / 2.0F;
	case MODAL_SIZE_Full:
		return screen_width;
	default:
		assert(false);
	}
	return screen_width;
}

static void UpdateElementGroup(ElementGroup* pGroup)
{
	//Cycle focus around all elements in the group
	if (IsKeyPressedWithoutDelay(KEY_Tab))
	{
		s32 focus_index = -1;
		for (u32 i = 0; i < pGroup->elements.count; i++)
		{
			UiElement* element = pGroup->elements[i];
			if (element->focused)
			{
				focus_index = i;
				break;
			}
		}
		if (focus_index > -1)
		{
			UiElement* element = pGroup->elements[focus_index];
			element->focused = false;
		}	

		if (IsKeyDown(KEY_Shift)) focus_index--;
		else focus_index++;

		if (focus_index > (s32)pGroup->elements.count - 1) focus_index = 0;
		else if (focus_index < 0) focus_index = pGroup->elements.count - 1;

		UiElement* focus= pGroup->elements[focus_index];
		focus->focused = true;
	}
}

static void DisplayModalWindow(Interface* pState, ModalWindowContent* pContent, MODAL_SIZES pSize)
{
	assert(pState->modal_index < MAX_MODALS);
	ModalWindow* window = &pState->modal[++pState->modal_index];
	window->content = pContent;
	window->size = pSize;
}

#include "ErrorModal.cpp"
#include "GameStartModal.cpp"
#include "Screen.cpp"
static void RenderModalWindow(RenderState* pState, Interface* pInterface)
{
	ModalWindow* window = &pInterface->modal[pInterface->modal_index];
	ModalWindowContent* content = window->content;
	PushQuad(pState, V2(0), V2(g_state.form->width, g_state.form->height), V4(0.5F, 0.5F, 0.5F, 0.3F));

	float width = GetModalSize(window->size) + 20;
	float height = content->GetHeight() + 20;

	v4 color = GetSetting(&g_state.config, "modal_background")->V4;
	v2 pos = V2((g_state.form->width - width) / 2.0F, (g_state.form->height - height) / 2.0F);
	PushSizedQuad(pState, pos, V2(width, height), color);
	content->Render(pos + V2(10), pState);
}

static void HandleModals(GameState* pState, Interface* pInterface, float pDeltaTime)
{
	if (pInterface->modal_index >= 0)
	{
		ModalWindow* modal = &pInterface->modal[pInterface->modal_index];
		MODAL_RESULTS result = modal->content->Update(pState, pDeltaTime);
		if (result == MODAL_RESULT_Ok)
		{
			modal->content->OnAccept();
			pInterface->modal_index--;
			delete modal->content;
		}
		else if (result == MODAL_RESULT_Cancel)
		{
			pInterface->modal_index--;
			delete modal->content;
		}
	}
}

static void RenderInterface(GameState* pState, Interface* pInterface, RenderState* pRender)
{
	v2 size = V2(pState->form->width, pState->form->height);
	BeginRenderPass(size, pRender);

	mat4 m = HMM_Orthographic(0.0F, size.Width, size.Height, 0.0F, -Z_LAYER_MAX * Z_INDEX_DEPTH, 10.0F);
	PushMatrix(pRender, m);
	glDisable(GL_DEPTH_TEST);

	SetZLayer(pRender, Z_LAYER_Ui);
	pInterface->current_screen->RenderInterface(pRender, pState);

	if (pInterface->modal_index >= 0) RenderModalWindow(pRender, pInterface);

	EndRenderPass(size, pRender);
	glEnable(GL_DEPTH_TEST);
}