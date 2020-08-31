#pragma once
const u32 MAX_ENTITIES = 1024;

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

struct EntityIndex
{
	u32 index;
	u32 version;
};

struct RigidBody;
struct Entity
{
	v2 position;
	RigidBody* body;

	EntityIndex index = {};
};

struct EntityList
{
	u32 end_index;
	Entity* entities;
	u16* versions;
	FreeList<u32> free_indices;
};

struct ParticleCreationOptions
{
	v3 color;
	float life_min;
	float life_max;
	float size_min;
	float size_max;
	v2 direction;
	float spread;
	float speed_min;
	float speed_max;
};


struct Particle
{
	v2 velocity;
	v3 color;
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

struct Player
{
	Entity* entity;

	//Game start state
	char name[NAME_LENGTH];
	PLAYER_TEAMS team;
	PLAYER_ROLES role;

	//Local state
	AnimatedBitmap bitmap;
	bool flip; //TODO remove eventually?
	Timer invuln_timer;

	Timer attack_choose_timer;
	//ParticleSystem dust;

	//Synced state
	LocalPlayerState local_state;
	SyncedPlayerState state;

	PlayingSound* walking;
};

//struct Beer
//{
//	Entity* entity;
//};

static void UpdatePlayer(Player* pEntity, float pDeltaTime, u32 pFlags);