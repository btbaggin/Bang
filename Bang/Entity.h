#pragma once
const u32 MAX_ENTITIES = 1024;
const u32 MAX_BEERS = 3;

enum PLAYER_ROLES : u8
{
	PLAYER_ROLE_Unknown,
	PLAYER_ROLE_Sheriff,
	PLAYER_ROLE_Deputy,
	PLAYER_ROLE_Outlaw,
	PLAYER_ROLE_Renegade,
};

#define ATTACK_ON_CD -1
#define ATTACK_ROLLING -2
#define ATTACK_PENDING -3
enum PLAYER_TEAMS : u8
{
	PLAYER_TEAM_1,
	PLAYER_TEAM_2,
	PLAYER_TEAM_3,

	PLAYER_TEAM_COUNT,
};
v4 team_colors[] = { V4(1, 1, 0, 1), V4(0.5F, 0.75F, 1, 1), V4(0.45F, 0.5F, 0.2F, 1) };

enum ENTITY_TYPES : u8
{
	ENTITY_TYPE_None,
	ENTITY_TYPE_Player,
	ENTITY_TYPE_Beer,
	ENTITY_TYPE_Wall,
	ENTITY_TYPE_Arrows,
};

struct EntityIndex
{
	u32 index;
	u32 version;
};

struct RenderState;
struct RigidBody;
struct Entity
{
	v2 position;
	RigidBody* body;

	EntityIndex index = {};
	ENTITY_TYPES type;

	virtual void Render(RenderState* pState) = 0;
	virtual void Update(GameState* pState, float pDeltaTime, u32 pInputFlags) = 0;
};

struct EntityList
{
	u32 end_index;
	Entity** entities;
	u16* versions;
	FreeList<u32> free_indices;
};

struct ParticleCreationOptions
{
	float life_min;
	float life_max;
	float size_min;
	float size_max;
	v2 direction;
	float spread;
	float speed_min;
	float speed_max;
	u8 r, g, b, a;
};


struct Particle
{
	v2 velocity;
	u8 r, g, b, a;
	v2 position;
	float size;
	float life;
	float camera_distance;

	bool operator<(const Particle& that) const {
		// Sort in reverse order : far particles drawn first.
		return this->camera_distance > that.camera_distance;
	}
};

typedef void ParticleUpdate(Particle* pParticle, float pDeltaTime);

struct ParticleSystem
{
	u32 particle_count;
	u32 alive_particle_count;
	u32 last_particle_used;
	Particle* particles;
	v2 position;
	float time_per_particle;
	float particle_time;
	ParticleCreationOptions* creation;
	BITMAPS texture;

	u32 VAO;
	u32 VBO;
	u32 CBO;
	u32 PBO;
};

struct Player : public Entity
{
	//Game start state
	char name[NAME_LENGTH];
	PLAYER_TEAMS team;
	PLAYER_ROLES role;

	//Local state
	AnimatedBitmap bitmap;
	bool flip; //TODO remove eventually?
	Timer invuln_timer;
	bool death_message_sent;

	Timer attack_choose_timer;
	ParticleSystem dust;

	//Synced state
	LocalPlayerState local_state;
	SyncedPlayerState state;

	PlayingSound* walking;

	void Update(GameState* pState, float pDeltatime, u32 pInputFlags);
	void Render(RenderState* pState);
};

struct Wall : public Entity 
{ 
	void Update(GameState* pState, float pDeltatime, u32 pInputFlags) { }
	void Render(RenderState* pState) { }
};

struct Beer : public Entity
{
	bool up;
	float life;
	v2 original_pos;
	void Update(GameState* pState, float pDeltatime, u32 pInputFlags);
	void Render(RenderState* pState);
};

struct Arrows : public Entity
{
	PlayingSound* sound;
	float life;
	void Update(GameState* pState, float pDeltatime, u32 pInputFlags);
	void Render(RenderState* pState);
};

struct SpawnBeerEvent;
struct ArrowsEvent;
static Beer* CreateBeer(GameState* pState, SpawnBeerEvent* pEvent);
static Arrows* CreateArrow(GameState* pState, ArrowsEvent* pEvent);