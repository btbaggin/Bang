

void MainMenu::Load(GameState* pState)
{
	menu_index = 0;
	background = {};
	background.bitmaps[0] = BITMAP_MainMenu1;
	background.bitmaps[1] = BITMAP_MainMenu2;
	background.bitmaps[2] = BITMAP_MainMenu3;
	background.bitmaps[3] = BITMAP_MainMenu4;
	background.bitmaps[4] = BITMAP_MainMenu5;
	background.speed = GetSetting(&pState->config, "main_menu_backround_speed")->f;
	background.layers = 5;
}

void MainMenu::Update(GameState* pState, Interface* pInterface, float pDeltaTime, u32 pPredictionId)
{
	if (IsTransitioning())
	{
		for (u32 i = 1; i < background.layers; i++)
		{
			background.position[i].Y += 500 * (i * i * pDeltaTime);
		}
	}
	else
	{
		UpdateParalaxBitmap(&background, pDeltaTime, pState->form->width);

		if (IsKeyPressed(KEY_Down))
		{
			if (++menu_index > 2) menu_index = 2;
			else PlaySound(g_transstate.assets, SOUND_Beep, 0.75F);
		}
		else if (IsKeyPressed(KEY_Up))
		{
			if (--menu_index < 0) menu_index = 0;
			else PlaySound(g_transstate.assets, SOUND_Beep, 0.75F);
		}
		else if (IsKeyPressed(KEY_Enter))
		{
			switch (menu_index)
			{
			case 0:
			{
				PROCESS_INFORMATION pi;
				STARTUPINFOA si = {};
				si.dwFlags = STARTF_USESHOWWINDOW;
				si.wShowWindow = SW_MINIMIZE;
				CreateProcessA("BangServer.exe", NULL, NULL, NULL, true, 0, NULL, NULL, &si, &pi);

				pState->server_handle = pi.hProcess;
			}
			case 1:
				DisplayModalWindow(pInterface, new GameStartModal(menu_index == 0), MODAL_SIZE_Third);
				break;

			case 2:
				pState->is_running = false;
				break;
			}
		}
	}
}
void MainMenu::RenderInterface(RenderState* pRender, GameState* pState)
{
	v2 size = V2(g_state.form->width, g_state.form->height);
	RenderParalaxBitmap(pRender, g_transstate.assets, &background, size);

	if (!IsTransitioning())
	{
		PushSizedQuad(pRender, V2((size.X - 200) / 2.0F, size.Height * 0.1F), V2(200, 100), GetBitmap(g_transstate.assets, BITMAP_Title));

		v4 color = GetSetting(&pState->config, "menu_color")->V4;
		v4 color_selected = GetSetting(&pState->config, "menu_color_selected")->V4;
		v2 pos = V2(CenterTextHorizontally(FONT_Title, "Host Game", pState->form->width), size.Height * 0.5F);
		PushText(pRender, FONT_Title, "Host Game", pos, menu_index == 0 ? color_selected : color);

		pos = V2(CenterTextHorizontally(FONT_Title, "Join Game", pState->form->width), size.Height * 0.6F);
		PushText(pRender, FONT_Title, "Join Game", pos, menu_index == 1 ? color_selected : color);

		pos = V2(CenterTextHorizontally(FONT_Title, "Exit", pState->form->width), size.Height * 0.7F);
		PushText(pRender, FONT_Title, "Exit", pos, menu_index == 2 ? color_selected : color);
	}
}
Screen* MainMenu::CreateNextScreen(MemoryStack* pStack)
{
	return PushClass(pStack, LobbyScreen);
}




void LobbyScreen::Load(GameState* pState)
{
	start_button = CreateButton(V2(80, 40), "Start", V4(0.75F));
}

void LobbyScreen::Update(GameState* pState, Interface* pInterface, float pDeltaTime, u32 pPredictionId)
{
	if (g_net.client_id == 0)
	{
		v2 pos = V2(pState->form->width - 10, pState->form->height - 10) - start_button.size;
		if (IsClicked(&start_button, pos))
		{
			GameStart s = {};
			s.client_id = g_net.client_id;
			u32 size = WriteMessage(g_net.buffer, &s, GameStart, CLIENT_MESSAGE_GameStart);
			SocketSend(&g_net.send_socket, g_net.server_ip, g_net.buffer, size);
		}
	}
}
void LobbyScreen::RenderInterface(RenderState* pRender, GameState* pState)
{
	u32 rows = GetSetting(&pState->config, "lobby_rows")->i;
	u32 columns = MAX_PLAYERS / rows;
	v2 size = V2(pState->form->width / columns, pState->form->height / (MAX_PLAYERS / columns)) - V2(20);
	v2 pos = V2(10);

	PushSizedQuad(pRender, V2(0), V2(pState->form->width, pState->form->height), GetBitmap(g_transstate.assets, BITMAP_MainMenu1));
	for (u32 i = 0; i < MAX_PLAYERS; i++)
	{
		PushSizedQuad(pRender, pos, size, V4(0.5F));
		Client* c = g_net.clients + i;
		if (IsClientConnected(c))
		{
			v4 color = g_net.client_id == i ? V4(1, 1, 0, 1) : V4(1);
			PushText(pRender, FONT_Title, c->name, pos + V2(5), color);
		}

		pos.Y += size.Height + 10;
		if (i == rows - 1)
		{
			pos.Y = 10;
			pos.X += size.Width + 10;
		}
	}

	if (g_net.client_id == 0)
	{
		v2 button_position = V2(pState->form->width - 10, pState->form->height - 10) - start_button.size;
		RenderButton(pRender, &start_button, button_position);
	}
}
Screen* LobbyScreen::CreateNextScreen(MemoryStack* pStack)
{
	return PushClass(pStack, GameScreen);
}


void GameScreen::Load(GameState* pState)
{
	pState->game_started = true;
	pState->map = PushStruct(pState->world_arena, TiledMap);
	LoadTiledMap(pState->map, "..\\..\\Resources\\level2.json", g_transstate.trans_arena);

	sound = LoopSound(g_transstate.assets, SOUND_Background, 0.5F);
}


void GameScreen::Render(RenderState* pRender, GameState* pState)
{
	if (!IsTransitioning())
	{
		SetZLayer(pRender, Z_LAYER_Background1);
		UpdateCamera(pState, pState->players[g_net.client_id]);
		PushSizedQuad(pRender, V2(0), V2((float)pState->map->width, (float)pState->map->height), pState->map->bitmap);
	}
}

void GameScreen::Update(GameState* pState, Interface* pInterface, float pDeltaTime, u32 pPredictionId)
{
	intro_screen -= pDeltaTime;
	Player* p = pState->players[g_net.client_id];
	if (p->state.team_attack_choice == ATTACK_PENDING)
	{
		if (IsKeyPressed(KEY_Q)) p->state.team_attack_choice = attack_choices[0];
		else if (IsKeyPressed(KEY_E)) p->state.team_attack_choice = attack_choices[1];
	}

	if (!IsTransitioning())
	{
		u32 flags = 0;
		if (IsKeyDown(KEY_Left)) flags |= 1 << INPUT_MoveLeft;
		if (IsKeyDown(KEY_Right)) flags |= 1 << INPUT_MoveRight;
		if (IsKeyDown(KEY_Up)) flags |= 1 << INPUT_MoveUp;
		if (IsKeyDown(KEY_Down)) flags |= 1 << INPUT_MoveDown;
		if (IsKeyPressed(KEY_Space)) flags |= 1 << INPUT_Shoot;

		Client* c = g_net.clients + g_net.client_id;
		Player* p = g_state.players[g_net.client_id];

		if (IsEntityValid(&pState->entities, p))
		{
			ClientInput i = {};
			i.attack_choice = p->state.team_attack_choice;
			i.client_id = g_net.client_id;
			i.prediction_id = pPredictionId;
			i.dt = pDeltaTime;
			i.flags = flags;
			u32 size = WriteMessage(g_net.buffer, &i, ClientInput, CLIENT_MESSAGE_Input);
			SocketSend(&g_net.send_socket, g_net.server_ip, g_net.buffer, size);

			p->Update(pState, pDeltaTime, flags);
			
			u32 index = pPredictionId & PREDICTION_BUFFER_MASK;
			PredictedMove* move = &g_net.moves[index];
			PredictedMoveResult* result = &g_net.results[index];

			move->dt = pDeltaTime;
			move->input = flags;
			result->position = p->position;
			result->state = p->local_state;
		}
	}
}
void GameScreen::RenderInterface(RenderState* pRender, GameState* pState)
{
	if (IsTransitioning())
	{
		//Transitioning is after game is done, going back to main menu
		v2 screen = V2(pState->form->width, pState->form->height);
		PushQuad(pRender, V2(0), screen, V4(0, 0, 0, 1));
		const char* text = nullptr;
		switch (winner)
		{
		case PLAYER_ROLE_Sheriff:
			text = "Sheriff and Deputies win!";
			break;
		case PLAYER_ROLE_Outlaw:
			text = "Outlaws win!";
			break;
		case PLAYER_ROLE_Renegade:
			text = "Renegade wins!";
			break;
		default:
			assert(false);
		}
		v2 pos = CenterText(FONT_Title, text, screen);
		PushText(pRender, FONT_Title, text, pos, V4(1));
	}
	else if (intro_screen > 0)
	{
		//Into shows the goal when the game starts
		Player* p = pState->players[g_net.client_id];
		
		v2 pos = V2(0, pState->form->height - (pState->form->height / 4));
		PushQuad(pRender, pos, V2(pState->form->width, pState->form->height), V4(0, 0, 0, 0.8F));
		switch (p->role)
		{
		case PLAYER_ROLE_Sheriff:
			PushText(pRender, FONT_Title, "Eliminate all outlaws and renegades", pos, V4(1));
			break;
		case PLAYER_ROLE_Deputy:
			PushText(pRender, FONT_Title, "Help and protect the sheriff", pos, V4(1));
			break;
		case PLAYER_ROLE_Outlaw:
			PushText(pRender, FONT_Title, "Eliminate the sheriff", pos, V4(1));
			break;
		case PLAYER_ROLE_Renegade:
			PushText(pRender, FONT_Title, "Be the last player alive", pos, V4(1));
			break;
		default:
			assert(false);
		}
	}
	else
	{
		Player* p = pState->players[g_net.client_id];
		float ms = 0;
		for (u32 i = 0; i < ArrayCount(g_net.latency_ms); i++)
		{
			ms += g_net.latency_ms[i];
		}
		ms /= ArrayCount(g_net.latency_ms);

		char buffer[12];
		sprintf(buffer, "Ping: %dms", (u32)ms);
		PushText(pRender, FONT_Debug, buffer, V2(0), COLOR_BLACK);

		v2 button_size = V2(64);
		v2 button_1 = V2(pState->form->width / 4.0F, pState->form->height - button_size.Height - 20);
		v2 button_2 = V2(pState->form->width - button_1.X - button_size.Width, pState->form->height - button_size.Height - 20);
		if (p->state.team_attack_choice == ATTACK_ROLLING)
		{
			PushSizedQuad(pRender, button_1, button_size, team_colors[Random(0, (u32)PLAYER_TEAM_COUNT)], GetBitmap(g_transstate.assets, BITMAP_Target));
			PushSizedQuad(pRender, button_2, button_size, team_colors[Random(0, (u32)PLAYER_TEAM_COUNT)], GetBitmap(g_transstate.assets, BITMAP_Target));
		}
		else if (p->state.team_attack_choice == ATTACK_PENDING)
		{
			v2 button_mid = CenterText(FONT_Title, "Q", button_size);
			PushSizedQuad(pRender, button_1, button_size, team_colors[attack_choices[0]], GetBitmap(g_transstate.assets, BITMAP_Target));
			PushSizedQuad(pRender, button_2, button_size, team_colors[attack_choices[1]], GetBitmap(g_transstate.assets, BITMAP_Target));
			PushText(pRender, FONT_Title, "Q", button_1 + button_mid, V4(0, 0, 0, 1));
			PushText(pRender, FONT_Title, "E", button_2 + button_mid, V4(0, 0, 0, 1));
		}

		v2 beer_pos = V2(pState->form->width - 32, 0);
		for (u32 i = 0; i < p->local_state.beers; i++)
		{
			PushSizedQuad(pRender, beer_pos, V2(32), GetBitmap(g_transstate.assets, BITMAP_Beer));
			beer_pos.X -= 32;
		}
	}
};
Screen* GameScreen::CreateNextScreen(MemoryStack* pStack)
{
	return PushClass(pStack, MainMenu);
}
