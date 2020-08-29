#pragma once
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>
#include "time.h"

#include "typedefs.h"
const u32 MAX_PLAYERS = 8;
const u32 NAME_LENGTH = 20;
const u32 THREAD_COUNT = 4;
const u32 QUEUE_ENTRIES = 256;
const u32 UPDATE_FREQUENCY = 60;
float ExpectedSecondsPerFrame = 1.0F / UPDATE_FREQUENCY;

#include "Timer.h"
#include "Memory.h"
#include "FreeList.h"
#include "ConfigFile.h"
#include "Platform.h"
#include "Assets.h"
#include "Netcode.h"
#include "Entity.h"
#include "Level.h"
#include "Collision.h"

enum ERROR_TYPE
{
	ERROR_TYPE_Error,
	ERROR_TYPE_Warning
};

enum INPUT_ACTIONS
{
	INPUT_MoveLeft,
	INPUT_MoveRight,
	INPUT_MoveUp,
	INPUT_MoveDown,
	INPUT_Shoot,
};



struct MainMenuScreen
{
	s16 menu_index;
	ParalaxBitmap backgound;
};

struct LobbyScreen
{
};

struct GameScreen
{
};

typedef bool add_work_queue_entry(PlatformWorkQueue* pQueue, work_queue_callback pCallback, void* pData);
typedef void complete_all_work(PlatformWorkQueue* pQueue);

struct WorkQueueEntry
{
	work_queue_callback* callback;
	void* data;
};


struct GameTime
{
	float delta_time;
	float unscaled_delta_time;
	float time_scale;
	long current_time;
};

struct Camera
{
	v2 position;
	float zoom;
};

struct PlayingSound;
struct GameState
{
	TemporaryMemoryHandle screen_reset;

	EntityList entities;

	StaticList<Player, MAX_PLAYERS> players;
	//StaticList<Beer, 3> beers;

	PhysicsScene physics;
	Camera camera;
	TiledMap* map;

	MemoryStack* world_arena;

	PlatformWorkQueue* work_queue;
	TaskCallbackQueue callbacks;

	PlayingSound* FirstPlaying;
	PlatformWindow* form;

	ConfigFile config;

	bool is_running;
};

struct Assets;
struct RenderState;
struct GameTransState
{
	MemoryStack* trans_arena;
	RenderState* render_state;
	PlatformWorkQueue* high_priority;
	PlatformWorkQueue* low_priority;
	Task tasks[4];
	TaskCallbackQueue queue;

	Assets* assets;
	FreeList<PlayingSound*> available_sounds;
};

struct GameSoundBuffer
{
	u32 samples_per_second;
	u32 sample_count;
	void* data;
};

inline void Tick(GameTime* pTime)
{
	auto current_time = clock();
	pTime->unscaled_delta_time = (current_time - pTime->current_time) / (float)CLOCKS_PER_SEC;
	pTime->delta_time = pTime->unscaled_delta_time * pTime->time_scale;
	pTime->current_time = current_time;
}

#define Verify(condition, message, type) if (!condition) { DisplayErrorMessage(message, type); return; }