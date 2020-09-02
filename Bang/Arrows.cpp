static Arrows* CreateArrow(GameState* pState, ArrowsEvent* pEvent)
{
	Arrows* b = CreateEntity(&pState->entities, Arrows);
	b->sound = LoopSound(g_transstate.assets, SOUND_Arrows, 0.75F);
	b->life = 15;
	b->position = pEvent->position;

	return b;
}

void Arrows::Update(GameState* pState, float pDeltatime, u32 pInputFlags)
{
	//if(!sound) sound = LoopSound(g_transstate.assets, SOUND_Arrows, 0.75F);
	life -= pDeltatime;
	if (life <= 0)
	{
		StopSound(sound);
		RemoveEntity(&pState->entities, this);
	}
}

void Arrows::Render(RenderState* pState)
{
#ifndef _SERVER
	PushSizedQuad(pState, position - V2(64), V2(128), V4(1, 0, 0, 0.5F));
#endif
}
