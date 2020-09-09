PARTICLE_UPDATE(UpdateArrowParticles)
{
	pParticle->position += pParticle->velocity * pDeltaTime;
}

static Arrows* CreateArrow(GameState* pState, ArrowsEvent* pEvent)
{
	Arrows* b = CreateEntity(&pState->entities, Arrows);
#ifndef _SERVER
	b->sound = LoopSound(g_transstate.assets, SOUND_Arrows, 0.75F, b);

	ParticleCreationOptions* options = PushStruct(pState->world_arena, ParticleCreationOptions);
	options->r = 255; options->g = 255; options->b = 255; options->a = 255;
	options->direction = V2(0, 1);
	options->life = { 0.25F, 0.5F };
	options->size = { 18, 18 };
	options->speed = { 1, 5 };
	options->spread = GetSetting(&pState->config, "arrow_radius")->f / 2.0F;
	b->system = SpawnParticleSystem(100, 300, BITMAP_Arrow, options);
	b->system.position = pEvent->position;
#endif
	b->life = GetSetting(&pState->config, "arrow_duration")->f;
	b->position = pEvent->position;

	return b;
}

void Arrows::Update(GameState* pState, float pDeltatime, u32 pInputFlags)
{
	float range = GetSetting(&pState->config, "arrow_radius")->f;

	Entity* entities[MAX_PLAYERS];
	u32 count = FindEntitiesWithinRange(&pState->entities, position, range, entities, MAX_PLAYERS, ENTITY_TYPE_Player);

	for (u32 i = 0; i < count; i++)
	{
		Player* p = (Player*)entities[i];
		DamagePlayer(p);
	}

#ifndef _SERVER
	UpdateParticleSystem(&system, pDeltatime, UpdateArrowParticles);
#endif
	life -= pDeltatime;
	if (life <= 0)
	{
#ifndef _SERVER
		StopSound(sound);
#endif
		RemoveEntity(&pState->entities, this);
	}
}

#ifndef _SERVER
void Arrows::Render(RenderState* pState)
{
	float range = GetSetting(&g_state.config, "arrow_radius")->f;
	PushEllipse(pState, position, V2(range), V4(1, 0, 0, 0.3F));

	PushParticleSystem(pState, &system);
}
#endif
