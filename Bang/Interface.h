#pragma once

#define STB_TEXTEDIT_CHARTYPE   char
#define STB_TEXTEDIT_STRING     Textbox
// get the base type
#include "stb/stb_textedit.h"


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

struct Button : UiElement
{
	v2 size;
	v4 color;
	bool clicked;
	char text[10];
	BITMAPS bitmap;
};

struct Textbox : public UiElement
{
	char *string;
	int stringlen;
	FONTS font;
	float width;
	float font_x;
	STB_TexteditState state;
};


struct Interface;
struct Screen
{
	virtual void Load(GameState* pState) = 0;

	virtual void Update(GameState* pState, float pDeltaTime, u32 pPredictionId) = 0;
	virtual void Render(RenderState* pRender, GameState* pState) = 0;

	virtual void UpdateInterface(GameState* pState, Interface* pInterface, float pDeltaTime) = 0;
	virtual void RenderInterface(RenderState* pRender, GameState* pState) = 0;

	virtual Screen* CreateNextScreen(MemoryStack* pStack) = 0;
};

struct Interface
{
	Screen* current_screen;
	float transition_time;

	ModalWindow modal[MAX_MODALS];
	s32 modal_index = -1;
};

struct ElementGroup
{
	StaticList<UiElement*, 10> elements;
};

struct MainMenu : Screen
{
	ParalaxBitmap background;
	s16 menu_index;
	void Load(GameState* pState);

	void Update(GameState* pState, float pDeltaTime, u32 pPredictionId) { };
	void Render(RenderState* pRender, GameState* pState) { };

	void UpdateInterface(GameState* pState, Interface* pInterface, float pDeltaTime);
	void RenderInterface(RenderState* pRender, GameState* pState);

	Screen* CreateNextScreen(MemoryStack* pStack);
};

struct LobbyScreen : Screen
{
	Button start_button;
	void Load(GameState* pState);

	void Update(GameState* pState, float pDeltaTime, u32 pPredictionId) { };
	void Render(RenderState* pRender, GameState* pState) { };

	void UpdateInterface(GameState* pState, Interface* pInterface, float pDeltaTime);
	void RenderInterface(RenderState* pRender, GameState* pState);

	Screen* CreateNextScreen(MemoryStack* pStack);
};

struct GameScreen : Screen
{
	float intro_screen = 3.0F;
	PLAYER_TEAMS attack_choices[2];
	PlayingSound* sound;

	void Load(GameState* pState);

	void Update(GameState* pState, float pDeltaTime, u32 pPredictionId);
	void Render(RenderState* pRender, GameState* pState);

	void UpdateInterface(GameState* pState, Interface* pInterface, float pDeltaTime);
	void RenderInterface(RenderState* pRender, GameState* pState);

	Screen* CreateNextScreen(MemoryStack* pStack);
};

//TODO allocate next screen
