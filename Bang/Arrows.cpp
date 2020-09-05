static Arrows* CreateArrow(GameState* pState, ArrowsEvent* pEvent)
{
	Arrows* b = CreateEntity(&pState->entities, Arrows);
#ifndef _SERVER
	b->sound = LoopSound(g_transstate.assets, SOUND_Arrows, 0.75F);
#endif
	b->life = 20;
	b->position = pEvent->position;

	return b;
}

void Arrows::Update(GameState* pState, float pDeltatime, u32 pInputFlags)
{
	//if(!sound) sound = LoopSound(g_transstate.assets, SOUND_Arrows, 0.75F);
	Entity* entities[MAX_PLAYERS];
	u32 count;
	FindEntitiesWithinRange(&pState->entities, position, 64, entities, &count, ENTITY_TYPE_Player);

	for (u32 i = 0; i < count; i++)
	{
		Player* p = (Player*)entities[i];
		DamagePlayer(p);
	}

	life -= pDeltatime;
	if (life <= 0)
	{
#ifndef _SERVER
		StopSound(sound);
#endif
		RemoveEntity(&pState->entities, this);
	}
}

void Arrows::Render(RenderState* pState)
{
#ifndef _SERVER
	PushSizedQuad(pState, position - V2(64), V2(128), V4(1, 0, 0, 0.5F));
#endif
}
