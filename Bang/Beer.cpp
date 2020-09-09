static Beer* CreateBeer(GameState* pState, SpawnBeerEvent* pEvent)
{
	Beer* b = CreateEntity(&pState->entities, Beer);
	b->position = pEvent->position;
	b->original_pos = pEvent->position;
	b->life = 20.0F;

	return b;
}

void Beer::Update(GameState* pState, float pDeltaTime, u32 pInputFlags)
{
	const float RANGE = 7.0F;
	float min = original_pos.Y - RANGE / 2;
	float max = original_pos.Y + RANGE / 2;

	//float up and down
	float y = clamp(0.0, (position.Y - min) / (max - min), 1.0);
	if (y >= 1) up = false; else if(y <= 0) up = true;
	if (up) y += pDeltaTime / 3;
	else y -= pDeltaTime / 3;

	position.Y = min + (y * RANGE);

	life -= pDeltaTime;
	if (life <= 0) RemoveEntity(&pState->entities, this);
}

#ifndef _SERVER
void Beer::Render(RenderState* pState)
{
	PushEllipse(pState, original_pos + V2(g_state.map->tile_size.Width / 2, g_state.map->tile_size.Height), V2(g_state.map->tile_size.Width / 3, g_state.map->tile_size.Height / 6), V4(0, 0, 0, 0.1F));

	PushSizedQuad(pState, position, g_state.map->tile_size, GetBitmap(g_transstate.assets, BITMAP_Beer));
}
#endif