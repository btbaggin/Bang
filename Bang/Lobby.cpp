static void LoadLobby(GameState* pState, LobbyScreen* pScreen)
{
	pScreen->start_button = CreateButton(V2(80, 40), "Start", V4(0.75F));
}

static void UpdateLobby(GameState* pState, Interface* pInterface, float pDeltaTime)
{
	LobbyScreen* lobby = (LobbyScreen*)pInterface->current_screen_data;
	if (g_net.client_id == 0)
	{
		v2 pos = V2(pState->form->width - 10, pState->form->height - 10) - lobby->start_button.size;
		if (IsClicked(&lobby->start_button, pos))
		{
			GameStart s = {};
			s.client_id = g_net.client_id;
			u32 size = WriteMessage(g_net.buffer, &s, GameStart, CLIENT_MESSAGE_GameStart);
			SocketSend(&g_net.send_socket, g_net.server_ip, g_net.buffer, size);
		}
	}
}

static void RenderLobby(GameState* pState, Interface* pInterface, RenderState* pRender)
{
	LobbyScreen* lobby = (LobbyScreen*)pInterface->current_screen_data;
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
		v2 button_position = V2(pState->form->width - 10, pState->form->height - 10) - lobby->start_button.size;
		RenderButton(pRender, &lobby->start_button, button_position);
	}
}