enum PLAYER_ANIMATIONS : u8
{
	PLAYER_ANIMATION_Attack,
	PLAYER_ANIMATION_Idle,
	PLAYER_ANIMATION_Walk,
};

static void SetAnimation(AnimatedBitmap* pBitmap, u32 pAnimation, float pDuration)
{
	if (pBitmap->current_animation != pAnimation)
	{
		pBitmap->current_animation = pAnimation;
		pBitmap->current_index = 0;
		pBitmap->frame_time = 0;
	}
	pBitmap->frame_duration = pDuration;
}

static void AttackPlayer(Player* pPlayer)
{
	pPlayer->state.health--;
	float time = GetSetting(&g_state.config, "player_invuln_time")->f;
	ResetTimer(&pPlayer->invuln_timer, time);
}

static Player* FindPlayersWithinAttackRange(Player* pPlayer, PLAYER_TEAMS pTeam)
{
	u32 range = GetSetting(&g_state.config, "player_attack_range")->i;
	for (u32 i = 0; i < MAX_PLAYERS; i++)
	{
		Player* p = g_state.players.items + i;
		if (p != pPlayer && 
			IsEntityValid(&g_state.entities, p->entity) &&
			p->team == pTeam &&
			!TimerIsStarted(&p->invuln_timer))
		{
			if (HMM_LengthSquared(p->entity->position - pPlayer->entity->position) < range * range)
			{
				return p;
			}
		}
	}

	return nullptr;
}

const float ATTACK_TIME = 1.0f;
const u32 PLAYER_SIZE = 64;
static Player* CreatePlayer(GameState* pState, GameNetState* pNet, char* pName)
{
	Entity* e = AddEntity(&pState->entities);
	e->position = V2((float)Random(0, pState->map->width), (float)Random(0, pState->map->height));

	Player p = {};
	p.entity = e;
	strcpy(p.name, pName);
	p.state.team_attack_choice = ATTACK_ON_CD;
	//ParticleCreationOptions* options = PushStruct(pState->world_arena, ParticleCreationOptions);
	//options->color = V3(1);
	//options->direction = V2(0);
	//options->life_min = 0.5F; options->life_max = 1.0F;
	//options->size_min = 10; options->size_max = 32;
	//options->speed_min = 1; options->speed_max = 5;
	//options->spread = 0;
	//p.dust = SpawnParticleSystem(3, 5, BITMAP_MainMenu1, options);

	u32 health = GetSetting(&g_state.config, "player_health")->i;
	p.local_state = {};
	p.state.health = health;
	p.state.animation = PLAYER_ANIMATION_Idle;

	RigidBodyCreationOptions o;
	o.density = 1.0F;
	o.type = SHAPE_Poly;
	o.width = 40;
	o.offset = V2(12, 0);
	o.height = PLAYER_SIZE;
	o.entity = e;
	o.material.dynamic_friction = 0.2F;
	o.material.static_friction = 0.1F;

	e->body = AddRigidBody(pState->world_arena, &pState->physics, &o);

	u32* lengths = PushArray(pState->world_arena, u32, 3);
	lengths[0] = 6; lengths[1] = 4; lengths[2] = 6;
	p.bitmap = CreateAnimatedBitmap(BITMAP_Character, 3, lengths, V2(48));

	pState->players.AddItem(p);
	return pState->players.items + pState->players.count - 1;
}

static void UpdatePlayer(Player* pEntity, float pDeltaTime, u32 pFlags)
{
	float speed = GetSetting(&g_state.config, "player_speed")->f;

	bool attacking = false;
	v2 velocity = {};
	if (HasFlag(1 << INPUT_MoveLeft, pFlags))
	{
		pEntity->flip = true;
		velocity.X -= speed * pDeltaTime;
	}
	if (HasFlag(1 << INPUT_MoveRight, pFlags))
	{
		pEntity->flip = false;
		velocity.X += speed * pDeltaTime;
	}
	if (HasFlag(1 << INPUT_MoveUp, pFlags))
	{
		velocity.Y -= speed * pDeltaTime;
	}
	if (HasFlag(1 << INPUT_MoveDown, pFlags))
	{
		velocity.Y += speed * pDeltaTime;
	}
	if (HasFlag(1 << INPUT_Shoot, pFlags) && pEntity->bitmap.current_animation != PLAYER_ANIMATION_Attack)
	{
		if (pEntity->state.team_attack_choice == ATTACK_ON_CD)
		{
			pEntity->state.team_attack_choice = ATTACK_ROLLING;
			ResetTimer(&pEntity->attack_choose_timer, 3.0F);
		}
		else if(pEntity->state.team_attack_choice >= 0)
		{
			Player* attackee = FindPlayersWithinAttackRange(pEntity, (PLAYER_TEAMS)pEntity->state.team_attack_choice);
			if (attackee)
			{
				AttackPlayer(attackee);
				attacking = true;
				pEntity->state.team_attack_choice = ATTACK_ON_CD;
			}
		}
	}

	//Update animation
	if (pEntity->bitmap.current_animation != PLAYER_ANIMATION_Attack || AnimationIsComplete(&pEntity->bitmap))
	{
		if(attacking) SetAnimation(&pEntity->bitmap, PLAYER_ANIMATION_Attack, ATTACK_TIME / 4.0F);
		else if (IsZero(velocity)) SetAnimation(&pEntity->bitmap, PLAYER_ANIMATION_Idle, 0.25F);
		else SetAnimation(&pEntity->bitmap, PLAYER_ANIMATION_Walk, 0.25F);

	}
	UpdateAnimation(&pEntity->bitmap, pDeltaTime);
	pEntity->state.animation = pEntity->bitmap.current_animation;

	//Invulnerability after being attacked
	if (TickTimer(&pEntity->invuln_timer, pDeltaTime)) StopTimer(&pEntity->invuln_timer);

	if (pEntity->state.team_attack_choice == ATTACK_ROLLING && TickTimer(&pEntity->attack_choose_timer, pDeltaTime))
	{
		StopTimer(&pEntity->attack_choose_timer);
		pEntity->state.team_attack_choice = ATTACK_PENDING;

#ifndef _SERVER
		GameScreen* game = (GameScreen*)g_interface.current_screen_data;
		u32 num[PLAYER_TEAM_COUNT];
		GenerateRandomNumbersWithNoDuplicates(num, PLAYER_TEAM_COUNT);
		game->attack_choices[0] = (PLAYER_TEAMS)num[0];
		game->attack_choices[1] = (PLAYER_TEAMS)num[1];
#endif
	}

	//pEntity->dust.position = pEntity->entity->position;
	//UpdateParticleSystem(&pEntity->dust, pDeltaTime, nullptr);

#ifndef _SERVER
	if (!IsZero(velocity))
	{
		if (!pEntity->walking) pEntity->walking = LoopSound(g_transstate.assets, SOUND_Walking, 0.25F);
		else ResumeLoopSound(pEntity->walking);
	}
	else
	{
		if(pEntity->walking) PauseSound(pEntity->walking);
	}
#endif

	pEntity->entity->position += velocity;
	pEntity->entity->position.X = clamp(0.0F, pEntity->entity->position.X, (float)g_state.map->width);
	pEntity->entity->position.Y = clamp(0.0F, pEntity->entity->position.Y, (float)g_state.map->height);
}

#ifndef _SERVER
static void RenderPlayerHeader(RenderState* pState, Player* pEntity)
{
	const float MARGIN = 5.0F;
	const float HEADER_HEIGHT = 24.0F;
	const float HEADER_WIDTH = 96.0F;
	v2 pos = pEntity->entity->position - V2((HEADER_WIDTH - PLAYER_SIZE) / 2.0F, HEADER_HEIGHT / 2.0F);

	v4 color = V4(1);
	switch (pEntity->role)
	{
	case PLAYER_ROLE_Sheriff:
		color = V4(1, 1, 0, 1);
		break;
	case PLAYER_ROLE_Outlaw:
		color = V4(0.3F, 0.3F, 0.3F, 1);
		break;
	case PLAYER_ROLE_Renegade:
		color = V4(1, 0, 0, 1);
		break;
	case PLAYER_ROLE_Deputy:
		color = V4(1, 1, 0, 1);
		break;
	case PLAYER_ROLE_Unknown:
		break;
	}

	//Header background
	SetZLayer(pState, Z_LAYER_Background2);
	PushSizedQuad(pState, pos, V2(HEADER_WIDTH + MARGIN * 2, HEADER_HEIGHT + MARGIN * 2), color);

	if (pEntity->state.team_attack_choice >= 0)
	{
		PushSizedQuad(pState, pos - V2(24, 0), V2(24), team_colors[pEntity->state.team_attack_choice], GetBitmap(g_transstate.assets, BITMAP_Target));
	}

	pos += V2(MARGIN);

	//Name
	SetZLayer(pState, Z_LAYER_Background3);
	PushText(pState, FONT_Normal, pEntity->name, pos, V4(1));

	//Health bar
	u32 max_health = GetSetting(&g_state.config, "player_health")->i;
	PushSizedQuad(pState, pos + V2(0, GetFontSize(FONT_Normal)), V2(HEADER_WIDTH, 12), V4(1, 0, 0, 1));
	SetZLayer(pState, Z_LAYER_Background4);
	PushSizedQuad(pState, pos + V2(0, GetFontSize(FONT_Normal)), V2(HEADER_WIDTH * (pEntity->state.health / (float)max_health), 12), V4(0, 1, 0, 1));



}

static void RenderPlayer(RenderState* pState, Player* pEntity)
{
	const v2 size = V2(PLAYER_SIZE);
	v4 color = team_colors[pEntity->team];
	//Player
	SetZLayer(pState, Z_LAYER_Player);
	RenderAnimation(pState, pEntity->entity->position, size, color, &pEntity->bitmap, pEntity->flip);

	//SetZLayer(pState, Z_LAYER_Ui);
	//PushParticleSystem(pState, &pEntity->dust);
}
#endif