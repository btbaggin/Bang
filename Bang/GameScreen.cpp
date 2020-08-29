static void LoadGame(GameState* pState, GameScreen* pScreen)
{
	pState->map = PushStruct(pState->world_arena, TiledMap);
	LoadTiledMap(pState->map, "C:\\Users\\admin\\Desktop\\test.json", g_transstate.trans_arena);
}

static void UpdateGame(GameState* pState, float pDeltaTime, u32 pPredictionId)
{
	u32 flags = 0;
	if (IsKeyDown(KEY_Left)) flags |= 1 << INPUT_MoveLeft;
	if (IsKeyDown(KEY_Right)) flags |= 1 << INPUT_MoveRight;
	if (IsKeyDown(KEY_Up)) flags |= 1 << INPUT_MoveUp;
	if (IsKeyDown(KEY_Down)) flags |= 1 << INPUT_MoveDown;
	if (IsKeyDown(KEY_Space)) flags |= 1 << INPUT_Shoot;

	ClientInput i = {};
	i.client_id = g_net.client_id;
	i.prediction_id = pPredictionId;
	i.dt = pDeltaTime;
	i.flags = flags;
	u32 size = WriteMessage(g_net.buffer, &i, ClientInput, CLIENT_MESSAGE_Input);
	SocketSend(&g_net.send_socket, g_net.server_ip, g_net.buffer, size);

	Client* c = g_net.clients + g_net.client_id;
	Player* p = g_state.players.items + g_net.client_id;
	UpdatePlayer(p, pDeltaTime, flags);

	u32 index = pPredictionId & PREDICTION_BUFFER_MASK;

	PredictedMove* move = &g_net.moves[index];
	PredictedMoveResult* result = &g_net.results[index];

	move->dt = pDeltaTime;
	move->input = flags;
	result->position = p->entity->position;
	result->state = p->local_state;
}

static void RenderGame(GameState* pState, RenderState* pRender)
{
	SetZLayer(pRender, Z_LAYER_Background1);
	UpdateCamera(pState, pState->players.items[g_net.client_id].entity);
	PushSizedQuad(pRender, V2(0), V2((float)pState->map->width, (float)pState->map->height), pState->map->bitmap);
}

static void UpdateGameInterface(GameState* pState, Interface* pInterface, float pDeltaTime)
{

}

static void RenderGameInterface(GameState* pState, Interface* pInterface, RenderState* pRender)
{
	float ms = 0;
	for (u32 i = 0; i < ArrayCount(g_net.latency_ms); i++)
	{
		ms += g_net.latency_ms[i];
	}
	ms /= ArrayCount(g_net.latency_ms);

	char buffer[10];
	sprintf(buffer, "Ping: %dms", (u32)ms);
	PushText(pRender, FONT_Debug, buffer, V2(0), COLOR_BLACK);
}