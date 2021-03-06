class GameStartModal : public ModalWindowContent
{
	bool is_host;
	ElementGroup group = {};
	const float MARGIN = 5.0F;
	Textbox name = {};
	Textbox ip[4] = {};
	virtual void Render(v2 pPosition, RenderState* pState)
	{
		UpdateElementGroup(&group);

		PushText(pState, FONT_Debug, "Name", pPosition, COLOR_BLACK);
		pPosition.Y += GetFontSize(FONT_Debug) + MARGIN;

		RenderTextbox(pState, &name, pPosition);
		pPosition.Y += GetFontSize(FONT_Debug) + MARGIN;

		if (!is_host)
		{
			PushText(pState, FONT_Debug, "Server IP", pPosition, COLOR_BLACK);
			pPosition.Y += GetFontSize(FONT_Debug) + MARGIN;
			for (u32 i = 0; i < ArrayCount(ip); i++)
			{
				RenderTextbox(pState, &ip[i], pPosition);
				pPosition.X += ip[i].width + MARGIN;
			}
		}
	}
	MODAL_RESULTS Update(GameState* pState, float pDeltaTime)
	{
		if (IsKeyPressed(KEY_Enter))
		{
			if (!name.string || name.stringlen == 0)
			{
				DisplayErrorMessage("Name is required", ERROR_TYPE_Warning);
				return MODAL_RESULT_None;
			}
				u8 ip_value[4];
			if (!is_host)
			{
				for (u32 i = 0; i < ArrayCount(ip); i++) ip_value[i] = atoi(ip[i].string);
			}
			else
			{
				ip_value[0] = 127; ip_value[1] = 0; ip_value[2] = 0; ip_value[3] = 1;
			}

			IPEndpoint server = Endpoint(ip_value[0], ip_value[1], ip_value[2], ip_value[3], PORT);

			switch (AttemptJoinServer(&g_net, name.string, server))
			{
			case JOIN_STATUS_Ok:
				return MODAL_RESULT_Ok;

			case JOIN_STATUS_GameStarted:
				DisplayErrorMessage("Game has already started", ERROR_TYPE_Warning);
				NetEnd(&g_net);
				break;

			case JOIN_STATUS_LobbyFull:
				DisplayErrorMessage("Game lobby is full", ERROR_TYPE_Warning);
				NetEnd(&g_net);
				break;

			case JOIN_STATUS_ServerNotFound:
				DisplayErrorMessage("Unable to reach server", ERROR_TYPE_Warning);
				NetEnd(&g_net);
				break;
			}
		}
		else if (IsKeyPressed(KEY_Escape)) return MODAL_RESULT_Cancel;

		return MODAL_RESULT_None;
	}

	virtual void OnAccept() 
	{ 
#ifndef _SERVER
		ResetTimer(&g_net.ping_timer, PING_TIME);
#endif
		strcpy(g_net.clients[g_net.client_id].name, name.string);
		EndScreen(1.0F);
	};

	float GetHeight()
	{
		if (is_host)
		{
			return (GetFontSize(FONT_Debug) + MARGIN) * 2;
		}
		else
		{
			return (GetFontSize(FONT_Debug) + MARGIN) * 4;
		}
	}

public:
	GameStartModal(bool pIsHost)
	{
		float width = GetModalSize(MODAL_SIZE_Third);
		CreateTextbox(&name, &group, width, FONT_Debug);
		is_host = pIsHost;
		for (u32 i = 0; i < ArrayCount(ip); i++)
		{
			CreateTextbox(&ip[i], &group, (width - (MARGIN * 4)) / 4.0F, FONT_Debug);
		}
	}

};