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

struct Player
{
	Entity* entity;

	//Game start state
	char name[NAME_LENGTH];
	v4 color;
	PLAYER_ROLES role;

	//Local state
	AnimatedBitmap bitmap;
	bool flip; //TODO remove eventually?
	Timer attack_timer;

	//Synced state
	LocalPlayerState local_state;
	SyncedPlayerState state;

	PlayingSound* walking;
};

//struct Beer
//{
//	Entity* entity;
//};