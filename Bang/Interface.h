#pragma once

static const v4 COLOR_BLACK = V4(0, 0, 0, 1);

const s32 MAX_MODALS = 2;
enum MODAL_RESULTS
{
	MODAL_RESULT_None,
	MODAL_RESULT_Ok,
	MODAL_RESULT_Cancel,
};

enum MODAL_SIZES
{
	MODAL_SIZE_Third,
	MODAL_SIZE_Half,
	MODAL_SIZE_Full,
};

enum GAME_SCREENS
{
	SCREEN_MainMenu,
	SCREEN_Lobby,
	SCREEN_Game,
};

bool IsKeyPressed(KEYS pKey);
class ModalWindowContent
{
public:
	virtual MODAL_RESULTS Update(GameState* pState, float pDeltaTime)
	{
		if (IsKeyPressed(KEY_Enter)) return MODAL_RESULT_Ok;
		else if (IsKeyPressed(KEY_Escape)) return MODAL_RESULT_Cancel;

		return MODAL_RESULT_None;
	}
	virtual void Render(v2 pPosition, RenderState* pState) = 0;
	virtual void OnAccept() { };
	virtual float GetHeight() = 0;
};
struct ModalWindow
{
	ModalWindowContent* content;
	MODAL_SIZES size;
};

struct UiElement
{
	bool focused;
};

struct Interface
{
	GAME_SCREENS current_screen;
	void* current_screen_data;

	ModalWindow modal[MAX_MODALS];
	s32 modal_index = -1;
};

struct ElementGroup
{
	StaticList<UiElement*, 10> elements;
};

static void TransitionScreen(GameState* pState, Interface* pInterface, GAME_SCREENS pLevel);