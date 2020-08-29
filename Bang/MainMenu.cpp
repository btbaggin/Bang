static void LoadMainMenu(GameState* pState, MainMenuScreen* pScreen)
{
	ParalaxBitmap background = {};
	background.bitmaps[0] = BITMAP_MainMenu1;
	background.bitmaps[1] = BITMAP_MainMenu2;
	background.bitmaps[2] = BITMAP_MainMenu3;
	background.bitmaps[3] = BITMAP_MainMenu4;
	background.bitmaps[4] = BITMAP_MainMenu5;
	background.speed = GetSetting(&pState->config, "main_menu_backround_speed")->f;
	background.layers = 5;

	pScreen->backgound = background;
}
static void UpdateMainMenu(GameState* pState, Interface* pInterface, float pDeltaTime)
{
	MainMenuScreen* level = (MainMenuScreen*)pInterface->current_screen_data;
	UpdateParalaxBitmap(&level->backgound, pDeltaTime, pState->form->width);

	if (IsKeyPressed(KEY_Down))
	{
		if (++level->menu_index > 2) level->menu_index = 2;
		else PlaySound(g_transstate.assets, SOUND_Beep, 0.75F);
	}
	else if (IsKeyPressed(KEY_Up))
	{
		if (--level->menu_index < 0) level->menu_index = 0;
		else PlaySound(g_transstate.assets, SOUND_Beep, 0.75F);
	}
	else if (IsKeyPressed(KEY_Enter))
	{
		switch (level->menu_index)
		{
			/*case 0:
			{
				TransitionScreen(pState, SCREEN_Lobby);
				StartServer();

				break;

			}*/
		case 1:
		{
			DisplayModalWindow(pInterface, new GameStartModal(), MODAL_SIZE_Third);
		}
		break;

		case 2:
			pState->is_running = false;
			break;
		}
	}
}

static void RenderMainMenu(GameState* pState, Interface* pInterface, RenderState* pRender)
{
	v2 size = V2(g_state.form->width, g_state.form->height);
	MainMenuScreen* level = (MainMenuScreen*)pInterface->current_screen_data;
	RenderParalaxBitmap(pRender, g_transstate.assets, &level->backgound, size);

	PushSizedQuad(pRender, V2((size.X - 200) / 2.0F, size.Height * 0.1F), V2(200, 100), GetBitmap(g_transstate.assets, BITMAP_Title));

	v4 color = GetSetting(&pState->config, "menu_color")->V4;
	v4 color_selected = GetSetting(&pState->config, "menu_color_selected")->V4;
	v2 pos = V2(CenterTextHorizontally(FONT_Title, "Host Game", pState->form->width), size.Height * 0.5F);
	PushText(pRender, FONT_Title, "Host Game", pos, level->menu_index == 0 ? color_selected : color);

	pos = V2(CenterTextHorizontally(FONT_Title, "Join Game", pState->form->width), size.Height * 0.6F);
	PushText(pRender, FONT_Title, "Join Game", pos, level->menu_index == 1 ? color_selected : color);

	pos = V2(CenterTextHorizontally(FONT_Title, "Exit", pState->form->width), size.Height * 0.7F);
	PushText(pRender, FONT_Title, "Exit", pos, level->menu_index == 2 ? color_selected : color);
}