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

#ifdef _SERVER
static void UpdateAnimation(AnimatedBitmap* pBitmap, float pDeltaTime) { }
#endif // _SERVER

static Player* FindPlayersWithinAttackRange(Player* pPlayer)
{
	u32 range = GetSetting(&g_state.config, "player_attack_range")->i;
	for (u32 i = 0; i < MAX_PLAYERS; i++)
	{
		Player* p = g_state.players.items + i;
		if (p != pPlayer && IsEntityValid(&g_state.entities, p->entity))
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
		Player* attackee = FindPlayersWithinAttackRange(pEntity);
		if (attackee)
		{
			attackee->state.health--;
			attacking = true;
			ResetTimer(&pEntity->attack_timer, 1.0F);
		}
	}

	if (pEntity->bitmap.current_animation != PLAYER_ANIMATION_Attack || TickTimer(&pEntity->attack_timer, pDeltaTime))
	{
		if(attacking) SetAnimation(&pEntity->bitmap, PLAYER_ANIMATION_Attack, ATTACK_TIME / 4.0F);
		else if (IsZero(velocity)) SetAnimation(&pEntity->bitmap, PLAYER_ANIMATION_Idle, 0.25F);
		else SetAnimation(&pEntity->bitmap, PLAYER_ANIMATION_Walk, 0.25F);

	}
	UpdateAnimation(&pEntity->bitmap, pDeltaTime);
	pEntity->state.animation = pEntity->bitmap.current_animation;

#ifndef _SERVER
	if (!IsZero(velocity))
	{
		if (!pEntity->walking) pEntity->walking = LoopSound(g_transstate.assets, SOUND_Walking, 0.1F);
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

const u32 PLAYER_SIZE = 64;
static Player* CreatePlayer(GameState* pState, GameNetState* pNet, char* pName)
{
	Entity* e = AddEntity(&pState->entities);
	e->position = V2((float)Random(0, pState->map->width), (float)Random(0, pState->map->height));

	Player p = {};
	p.entity = e;
	strcpy(p.name, pName);
	
	u32 health = GetSetting(&g_state.config, "player_health")->i;
	p.local_state = {};
	p.state.health = health;

	RigidBodyCreationOptions o;
	o.density = 1.0F;
	o.type = SHAPE_Poly;
	o.width = 40;
	o.offset = V2(12, 0);
	o.height = PLAYER_SIZE;

	o.entity = e;

	PhysicsMaterial m = {};
	m.dynamic_friction = 0.2F;
	m.static_friction = 0.1F;
	o.material = m;
	e->body = AddRigidBody(pState->world_arena, &pState->physics, &o);

#ifndef _SERVER
	u32* lengths = PushArray(pState->world_arena, u32, 3);
	lengths[0] = 6; lengths[1] = 4; lengths[2] = 6;
	p.bitmap = CreateAnimatedBitmap(BITMAP_Character, 3, lengths, V2(48));
#endif

	pState->players.AddItem(p);
	return pState->players.items + pState->players.count - 1;
}

#ifndef _SERVER
static void RenderPlayer(RenderState* pState, Player* pEntity)
{
	const float MARGIN = 5.0F;
	const float ICON_SIZE = 24.0F;
	const float HEADER_HEIGHT = 24.0F;
	const float HEADER_WIDTH = 96.0F;
	const float HEALTH_WIDTH = HEADER_WIDTH - ICON_SIZE;
	const v2 size = V2(PLAYER_SIZE);
	v2 pos = pEntity->entity->position - V2((HEADER_WIDTH - PLAYER_SIZE) / 2.0F, HEADER_HEIGHT / 2.0F);

	//Header background
	v4 header_color = GetSetting(&g_state.config, "header_background")->V4;
	SetZLayer(pState, Z_LAYER_Background2);
	PushSizedQuad(pState, pos, V2(HEADER_WIDTH + MARGIN * 2, HEADER_HEIGHT + MARGIN * 2), header_color);
	pos += V2(MARGIN);

	BITMAPS bitmap = BITMAP_None;
	v4 color = V4(1);
	switch (pEntity->role)
	{
	case PLAYER_ROLE_Sheriff:
		bitmap = BITMAP_Sheriff;
		color = V4(1, 1, 0, 1);
		break;
	case PLAYER_ROLE_Outlaw:
		bitmap = BITMAP_Outlaw;
		color = V4(0.3F, 0.3F, 0.3F, 1);
		break;
	case PLAYER_ROLE_Renegade:
		bitmap = BITMAP_Renegade;
		color = V4(1, 0, 0, 1);
		break;
	case PLAYER_ROLE_Deputy:
		bitmap = BITMAP_Deputy;
		color = V4(1, 1, 0, 1);
		break;
	case PLAYER_ROLE_Unknown:
		bitmap = BITMAP_Unknown;
		break;
	}
	//Role icon
	SetZLayer(pState, Z_LAYER_Background3);
	PushSizedQuad(pState, pos, V2(ICON_SIZE), color, GetBitmap(g_transstate.assets, bitmap));

	//Name
	PushText(pState, FONT_Normal, pEntity->name, pos + V2(ICON_SIZE, 0), V4(1));

	//Health bar
	u32 max_health = GetSetting(&g_state.config, "player_health")->i;
	PushSizedQuad(pState, pos + V2(ICON_SIZE, GetFontSize(FONT_Normal)), V2(HEALTH_WIDTH, 12), V4(1, 0, 0, 1));
	SetZLayer(pState, Z_LAYER_Background4);
	PushSizedQuad(pState, pos + V2(ICON_SIZE, GetFontSize(FONT_Normal)), V2(HEALTH_WIDTH * (pEntity->state.health / (float)max_health), 12), V4(0, 1, 0, 1));

	//Player
	SetZLayer(pState, Z_LAYER_Player);
	RenderAnimation(pState, pEntity->entity->position, size, pEntity->color, &pEntity->bitmap, pEntity->flip);
}
#endif