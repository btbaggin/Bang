enum PLAYER_ANIMATIONS : u8
{
	PLAYER_ANIMATION_AttackRight,
	PLAYER_ANIMATION_IdleRight,
	PLAYER_ANIMATION_WalkRight,
	PLAYER_ANIMATION_AttackLeft,
	PLAYER_ANIMATION_IdleLeft,
	PLAYER_ANIMATION_WalkLeft,
};

const float ATTACK_TIME = 1.0f;
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

static void DamagePlayer(Player* pPlayer)
{
	if (!TimerIsStarted(&pPlayer->invuln_timer))
	{
		pPlayer->state.health--;
		float time = GetSetting(&g_state.config, "player_invuln_time")->f;
		ResetTimer(&pPlayer->invuln_timer, time);
	}
}

static void HealPlayer(Player* pPlayer)
{
	u32 max_health = GetSetting(&g_state.config, "player_health")->i;
	pPlayer->state.health++;
	if (pPlayer->state.health > max_health) pPlayer->state.health = max_health;
}

PARTICLE_UPDATE(UpdateDustParticles)
{
	pParticle->size -= pDeltaTime * 2;
	if (pParticle->size <= 0) pParticle->size = 0;
	pParticle->a -= min(pParticle->a, Random((u32)6, 9));
}

static Player* CreatePlayer(GameState* pState, GameNetState* pNet, char* pName)
{
	Player* p = CreateEntity(&pState->entities, Player);
	p->position = V2((float)Random(0, pState->map->width), (float)Random(0, pState->map->height));
	p->scale = V2(g_state.map->tile_size.Width * 0.8F, g_state.map->tile_size.Height);
	strcpy(p->name, pName);
	p->state.team_attack_choice = ATTACK_ON_CD;
	p->death_message_sent = false;

	ParticleCreationOptions* options = PushStruct(pState->world_arena, ParticleCreationOptions);
	options->r = 255; options->g = 255; options->b = 255; options->a = 255;
	options->direction = V2(0, -1);
	options->life = { 0.25F, 0.5F };
	options->size = { pState->map->tile_size.Width * 0.25F, pState->map->tile_size.Width * 0.75F };
	options->speed = { 1, 5 };
	options->spawn_radius = 5;
	options->spread = 1;
	p->dust = SpawnParticleSystem(10, 15, BITMAP_Dust, options);

	p->walking = LoopSound(g_transstate.assets, SOUND_Walking, 0.25F);
	PauseSound(p->walking);

	u32 health = GetSetting(&g_state.config, "player_health")->i;
	p->local_state = {};
	p->state.health = health;
	p->state.animation = PLAYER_ANIMATION_IdleRight;

	RigidBodyCreationOptions o;
	o.density = 1.0F;
	o.type = SHAPE_Poly;
	o.width = pState->map->tile_size.Height * 0.8F;
	o.height = pState->map->tile_size.Height;
	o.offset = V2(0, 0);
	o.entity = p;
	o.material.dynamic_friction = 0.2F;
	o.material.static_friction = 0.1F;

	AddRigidBody(pState->world_arena, &pState->physics, p, &o);

	u32* lengths = PushArray(pState->world_arena, u32, 6);
	lengths[0] = 6; lengths[1] = 4; lengths[2] = 6;
	lengths[3] = 6; lengths[4] = 4; lengths[5] = 6;
	p->bitmap = CreateAnimatedBitmap(BITMAP_Character, 6, lengths, V2(30, 48));

	pState->players.AddItem(p);
	return p;
}

void Player::Update(GameState* pState, float pDeltaTime, CurrentInput pInput)
{
	if (state.health > 0)
	{
		float speed = GetSetting(&g_state.config, "player_speed")->f * pState->map->tile_size.Width;
		Entity* interact = GetEntityUnderMouse(&pState->entities, pInput.mouse);
		if (interact && interact->type == ENTITY_TYPE_Player) ((Player*)interact)->highlight = true;

		bool attacking = false;
		v2 velocity = {};
		if (HasFlag(1 << INPUT_MoveLeft, pInput.flags))
		{
			flip = true;
			velocity.X -= speed * pDeltaTime;
		}
		if (HasFlag(1 << INPUT_MoveRight, pInput.flags))
		{
			flip = false;
			velocity.X += speed * pDeltaTime;
		}
		if (HasFlag(1 << INPUT_MoveUp, pInput.flags))
		{
			velocity.Y -= speed * pDeltaTime;
		}
		if (HasFlag(1 << INPUT_MoveDown, pInput.flags))
		{
			velocity.Y += speed * pDeltaTime;
		}
		if (HasFlag(1 << INPUT_Shoot, pInput.flags))
		{
			if (state.team_attack_choice == ATTACK_ON_CD)
			{
				float time = GetSetting(&pState->config, "attack_cooldown")->f;
				state.team_attack_choice = ATTACK_ROLLING;
				ResetTimer(&attack_choose_timer, time);
			}
		}
		else if (HasFlag(1 << INPUT_Interact, pInput.flags) && interact)
		{
			switch (interact->type)
			{
			case ENTITY_TYPE_Beer:
				if (local_state.beers < MAX_BEERS)
				{
					if (interact && HMM_LengthSquared(interact->position - position) < 32 * 32)
					{
						local_state.beers++;
						RemoveEntity(&pState->entities, interact);
					}
				}
				break;

			case ENTITY_TYPE_Player:
				Player* p = (Player*)interact;
				u32 range = GetSetting(&pState->config, "player_attack_range")->i;
				if (p != this && p->team == state.team_attack_choice && HMM_LengthSquared(p->position - position) < range * range)
				{
					PlaySound(g_transstate.assets, SOUND_Sword, 1.0F, this);
					DamagePlayer(p);
					attacking = true;
					state.team_attack_choice = ATTACK_ON_CD;
				}
				break;
			}
		}
		else if (HasFlag(1 << INPUT_Beer, pInput.flags) && interact)
		{
			if(interact->type == ENTITY_TYPE_Player)
			{
				HealPlayer((Player*)interact);
			}
		}

		//Update animation
		if ((bitmap.current_animation != PLAYER_ANIMATION_AttackRight && bitmap.current_animation != PLAYER_ANIMATION_AttackLeft) || AnimationIsComplete(&bitmap))
		{
			if (attacking)
			{
				SetAnimation(&bitmap, flip ? PLAYER_ANIMATION_AttackLeft : PLAYER_ANIMATION_AttackRight, ATTACK_TIME / 4.0F);
			}
			else if (IsZero(velocity))
			{
				SetAnimation(&bitmap, flip ? PLAYER_ANIMATION_IdleLeft : PLAYER_ANIMATION_IdleRight, 0.25F);
			}
			else
			{
				SetAnimation(&bitmap, flip ? PLAYER_ANIMATION_WalkLeft : PLAYER_ANIMATION_WalkRight, 0.25F);
			}

		}
		UpdateAnimation(&bitmap, pDeltaTime);
		state.animation = bitmap.current_animation;

		if (state.team_attack_choice == ATTACK_ROLLING && TickTimer(&attack_choose_timer, pDeltaTime))
		{
			StopTimer(&attack_choose_timer);
			state.team_attack_choice = ATTACK_PENDING;

	#ifndef _SERVER
			GameScreen* game = (GameScreen*)g_interface.current_screen;
			u32 num[PLAYER_TEAM_COUNT];
			GenerateRandomNumbersWithNoDuplicates(num, PLAYER_TEAM_COUNT);
			game->attack_choices[0] = (PLAYER_TEAMS)num[0];
			game->attack_choices[1] = (PLAYER_TEAMS)num[1];
	#endif
		}

		dust.position = position + V2(pState->map->tile_size.Width / 2, pState->map->tile_size.Height);
		UpdateParticleSystem(&dust, pDeltaTime, UpdateDustParticles, !IsZero(velocity));
		if (!IsZero(velocity)) ResumeLoopSound(walking);
		else PauseSound(walking);

		//Invulnerability after being attacked
		if (TickTimer(&invuln_timer, pDeltaTime)) StopTimer(&invuln_timer);

		position += velocity;
		position.X = clamp(0.0F, position.X, (float)g_state.map->width);
		position.Y = clamp(0.0F, position.Y, (float)g_state.map->height);
	}
};

#ifndef _SERVER
void Player::Render(RenderState* pState)
{
	const float MARGIN = 5.0F;
	const float HEADER_HEIGHT = g_state.map->tile_size.Height * 0.66F;
	const float HEADER_WIDTH = g_state.map->tile_size.Width * 1.5F;
	v2 pos = position - V2((HEADER_WIDTH - scale.Width) / 2.0F, HEADER_HEIGHT);

	v4 color = V4(1);
	switch (role)
	{
	case PLAYER_ROLE_Sheriff:
		color = GetSetting(&g_state.config, "sheriff_color")->V4;
		break;
	case PLAYER_ROLE_Outlaw:
		color = GetSetting(&g_state.config, "outlaw_color")->V4;
		break;
	case PLAYER_ROLE_Renegade:
		color = GetSetting(&g_state.config, "renegade_color")->V4;
		break;
	case PLAYER_ROLE_Deputy:
		color = GetSetting(&g_state.config, "sheriff_color")->V4;
		break;
	case PLAYER_ROLE_Unknown:
		break;
	}

	//Header background
	PushSizedQuad(pState, pos, V2(HEADER_WIDTH + MARGIN * 2, HEADER_HEIGHT + MARGIN * 2), color);

	if (state.team_attack_choice >= 0)
	{
		PushSizedQuad(pState, pos - V2(24, 0), V2(24), team_colors[state.team_attack_choice], GetBitmap(g_transstate.assets, BITMAP_Target));
	}
	pos += V2(MARGIN);

	//Name
	PushText(pState, FONT_Normal, name, pos, V4(1));

	if (state.health > 0)
	{
		//Health bar
		u32 max_health = GetSetting(&g_state.config, "player_health")->i;
		PushSizedQuad(pState, pos + V2(0, GetFontSize(FONT_Normal)), V2(HEADER_WIDTH, HEADER_HEIGHT * 0.5F), V4(1, 0, 0, 1));
		PushSizedQuad(pState, pos + V2(0, GetFontSize(FONT_Normal)), V2(HEADER_WIDTH * (state.health / (float)max_health), HEADER_HEIGHT * 0.5F), V4(0, 1, 0, 1));

		color = team_colors[team];
		//Player
		PushEllipse(pState, position + V2(scale.Width / 2, scale.Height), V2(scale.Width / 3, scale.Height / 6), SHADOW_COLOR);
		if (highlight)
		{
			//Better way to outline https://learnopengl.com/Advanced-OpenGL/Stencil-testing
			//I could use this if I ended up outlining more things. Would need to store a matrix on Renderable_Quad instead of chaging the vertices directly
			RenderAnimation(pState, position - V2(3), scale + V2(6), V4(1, 0, 0, 1), &bitmap);
			highlight = false;
		}
		RenderAnimation(pState, position, scale, color, &bitmap);
	}

	//SetZLayer(pState, Z_LAYER_Ui);
	PushParticleSystem(pState, &dust);
}
#endif