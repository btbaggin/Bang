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
	const float RANGE = 10.0F;
	float min = original_pos.Y - RANGE / 2;
	float max = original_pos.Y + RANGE / 2;

	float y = clamp(0.0, (position.Y - min) / (max - min), 1.0);
	if (y >= 1) up = false; else if(y <= 0) up = true;
	if (up) y += pDeltaTime / 3;
	else y -= pDeltaTime / 3;

	position.Y = min + (y * RANGE);

	life -= pDeltaTime;

	if (life <= 0)
	{
		RemoveEntity(&pState->entities, this);
	}
}

void Beer::Render(RenderState* pState)
{
#ifndef _SERVER
	PushSizedQuad(pState, original_pos + V2(0, 24), V2(24, 32), V4(0, 0, 0, 0.5F), GetBitmap(g_transstate.assets, BITMAP_Shadow));

	PushSizedQuad(pState, position, V2(32), GetBitmap(g_transstate.assets, BITMAP_Beer));
#endif
}