enum PLAYER_ANIMATIONS : u8
{
	PLAYER_ANIMATION_Attack,
	PLAYER_ANIMATION_Idle,
	PLAYER_ANIMATION_Walk,
};

const float ATTACK_TIME = 1.0f;
const u32 PLAYER_SIZE = 64;
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


static Player* FindPlayersWithinAttackRange(GameState* pState, Player* pPlayer, PLAYER_TEAMS pTeam)
{
	u32 range = GetSetting(&pState->config, "player_attack_range")->i;
	Entity* entities[20];
	u32 count;
	FindEntitiesWithinRange(&pState->entities, pPlayer->position, (float)range, entities, &count, ENTITY_TYPE_Player);

	for (u32 i = 0; i < count; i++)
	{
		Player* p = (Player*)entities[i];
		if (p != pPlayer && p->team == pTeam && !TimerIsStarted(&p->invuln_timer))
		{
			return p;
		}
	}

	return nullptr;
}

static Player* CreatePlayer(GameState* pState, GameNetState* pNet, char* pName)
{
	Player* p = CreateEntity(&pState->entities, Player);
	p->position = V2((float)Random(0, pState->map->width), (float)Random(0, pState->map->height));
	strcpy(p->name, pName);
	p->state.team_attack_choice = ATTACK_ON_CD;

	//ParticleCreationOptions* options = PushStruct(pState->world_arena, ParticleCreationOptions);
	//options->color = V3(1);
	//options->direction = V2(0);
	//options->life_min = 0.5F; options->life_max = 1.0F;
	//options->size_min = 10; options->size_max = 32;
	//options->speed_min = 1; options->speed_max = 5;
	//options->spread = 0;
	//p.dust = SpawnParticleSystem(3, 5, BITMAP_MainMenu1, options);

	u32 health = GetSetting(&g_state.config, "player_health")->i;
	p->local_state = {};
	p->state.health = health;
	p->state.animation = PLAYER_ANIMATION_Idle;

	RigidBodyCreationOptions o;
	o.density = 1.0F;
	o.type = SHAPE_Poly;
	o.width = 40;
	o.offset = V2(12, 0);
	o.height = PLAYER_SIZE;
	o.entity = p;
	o.material.dynamic_friction = 0.2F;
	o.material.static_friction = 0.1F;

	p->body = AddRigidBody(pState->world_arena, &pState->physics, &o);

	u32* lengths = PushArray(pState->world_arena, u32, 3);
	lengths[0] = 6; lengths[1] = 4; lengths[2] = 6;
	p->bitmap = CreateAnimatedBitmap(BITMAP_Character, 3, lengths, V2(48));

#ifndef _SERVER
	p->walking = LoopSound(g_transstate.assets, SOUND_Walking, 0.25F);
	PauseSound(p->walking);
#endif

	pState->players.AddItem(p);
	return p;
}

void Player::Render(RenderState* pState)
{
#ifndef _SERVER
	const float MARGIN = 5.0F;
	const float HEADER_HEIGHT = 24.0F;
	const float HEADER_WIDTH = 96.0F;
	v2 pos = position - V2((HEADER_WIDTH - PLAYER_SIZE) / 2.0F, HEADER_HEIGHT / 2.0F);

	v4 color = V4(1);
	switch (role)
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
	SetZLayer(pState, Z_LAYER_Player);
	PushSizedQuad(pState, pos, V2(HEADER_WIDTH + MARGIN * 2, HEADER_HEIGHT + MARGIN * 2), color);

	if (state.team_attack_choice >= 0)
	{
		PushSizedQuad(pState, pos - V2(24, 0), V2(24), team_colors[state.team_attack_choice], GetBitmap(g_transstate.assets, BITMAP_Target));
	}

	pos += V2(MARGIN);

	//Name
	PushText(pState, FONT_Normal, name, pos, V4(1));

	//Health bar
	u32 max_health = GetSetting(&g_state.config, "player_health")->i;
	PushSizedQuad(pState, pos + V2(0, GetFontSize(FONT_Normal)), V2(HEADER_WIDTH, 12), V4(1, 0, 0, 1));
	PushSizedQuad(pState, pos + V2(0, GetFontSize(FONT_Normal)), V2(HEADER_WIDTH * (state.health / (float)max_health), 12), V4(0, 1, 0, 1));


	const v2 size = V2(PLAYER_SIZE);
	color = team_colors[team];
	//Player
	PushSizedQuad(pState, position + V2(0, 35), V2(48, 48), V4(1, 1, 1, 0.5), GetBitmap(g_transstate.assets, BITMAP_Shadow));
	RenderAnimation(pState, position, size, color, &bitmap, flip);

	//SetZLayer(pState, Z_LAYER_Ui);
	//PushParticleSystem(pState, &pEntity->dust);
#endif
}

void Player::Update(GameState* pState, float pDeltaTime, u32 pInputFlags)
{
	float speed = GetSetting(&g_state.config, "player_speed")->f;

	bool attacking = false;
	v2 velocity = {};
	if (HasFlag(1 << INPUT_MoveLeft, pInputFlags))
	{
		flip = true;
		velocity.X -= speed * pDeltaTime;
	}
	if (HasFlag(1 << INPUT_MoveRight, pInputFlags))
	{
		flip = false;
		velocity.X += speed * pDeltaTime;
	}
	if (HasFlag(1 << INPUT_MoveUp, pInputFlags))
	{
		velocity.Y -= speed * pDeltaTime;
	}
	if (HasFlag(1 << INPUT_MoveDown, pInputFlags))
	{
		velocity.Y += speed * pDeltaTime;
	}
	if (HasFlag(1 << INPUT_Shoot, pInputFlags) && bitmap.current_animation != PLAYER_ANIMATION_Attack)
	{
		if (state.team_attack_choice == ATTACK_ON_CD)
		{
			state.team_attack_choice = ATTACK_ROLLING;
			ResetTimer(&attack_choose_timer, 3.0F);
		}
		else if (state.team_attack_choice >= 0)
		{
			Player* attackee = FindPlayersWithinAttackRange(pState, this, (PLAYER_TEAMS)state.team_attack_choice);
			if (attackee)
			{
				AttackPlayer(attackee);
				attacking = true;
				state.team_attack_choice = ATTACK_ON_CD;
			}
		}
	}

	//Update animation
	if (bitmap.current_animation != PLAYER_ANIMATION_Attack || AnimationIsComplete(&bitmap))
	{
		if (attacking) SetAnimation(&bitmap, PLAYER_ANIMATION_Attack, ATTACK_TIME / 4.0F);
		else if (IsZero(velocity)) SetAnimation(&bitmap, PLAYER_ANIMATION_Idle, 0.25F);
		else SetAnimation(&bitmap, PLAYER_ANIMATION_Walk, 0.25F);

	}
	UpdateAnimation(&bitmap, pDeltaTime);
	state.animation = bitmap.current_animation;

	if (state.team_attack_choice == ATTACK_ROLLING && TickTimer(&attack_choose_timer, pDeltaTime))
	{
		StopTimer(&attack_choose_timer);
		state.team_attack_choice = ATTACK_PENDING;

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
		ResumeLoopSound(walking);
	}
	else
	{
		PauseSound(walking);
	}
#endif

	//Invulnerability after being attacked
	if (TickTimer(&invuln_timer, pDeltaTime)) StopTimer(&invuln_timer);

	if (local_state.beers < MAX_BEERS)
	{
		Entity* entities[10];
		u32 count;
		FindEntitiesWithinRange(&pState->entities, position, 64.0F, entities, &count, ENTITY_TYPE_Beer);
		if (count > 0)
		{
			local_state.beers++;
			RemoveEntity(&pState->entities, entities[0]);
		}
	}

	position += velocity;
	position.X = clamp(0.0F, position.X, (float)g_state.map->width);
	position.Y = clamp(0.0F, position.Y, (float)g_state.map->height);
};