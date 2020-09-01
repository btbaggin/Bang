void Beer::Update(GameState* pState, float pDeltaTime, u32 pInputFlags)
{
	v2 min = original_pos - V2(0, 5);
	v2 max = original_pos + V2(0, 5);


	y = clamp(0.0, (position.Y - min.Y) / (max.Y - min.Y), 1.0);
	//y = SmoothStep(min.Y, max.Y, position.Y);
	if (y >= 1) up = false; else if(y <= 0) up = true;
	if (up) y += pDeltaTime / 3;
	else y -= pDeltaTime / 3;

	position.Y = min.Y + (y * 10);
}

void Beer::Render(RenderState* pState)
{
#ifndef _SERVER
	PushSizedQuad(pState, original_pos + V2(0, 24), V2(24, 32), V4(0, 0, 0, 0.5F), GetBitmap(g_transstate.assets, BITMAP_Shadow));

	PushSizedQuad(pState, position, V2(32), GetBitmap(g_transstate.assets, BITMAP_Beer));

#endif
}