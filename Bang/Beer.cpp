static Beer* CreateBeer(GameState* pState, SpawnBeerEvent* pEvent)
{
	Beer* b = CreateEntity(&pState->entities, Beer);
	b->position = pEvent->position;
	b->scale = g_state.map->tile_size;
	b->original_pos = pEvent->position;
	b->life = 20.0F;

	return b;
}

void Beer::Update(GameState* pState, float pDeltaTime, CurrentInput pInput)
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
	PushEllipse(pState, original_pos + V2(scale.Width / 2, scale.Height), V2(scale.Width / 3, scale.Height / 6), SHADOW_COLOR);

	PushSizedQuad(pState, position, scale, GetBitmap(g_transstate.assets, BITMAP_Beer));
	//PushOutline(pState, 3);
}
#endif