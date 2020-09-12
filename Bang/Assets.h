#pragma once
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb\stb_truetype.h"
#include "typedefs.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_GIF
#include "stb\stb_image.h"

//Stub out any related asset method since assets arent loaded on the server
#ifdef _SERVER
#define SpawnParticleSystem(...) {}
#define UpdateParticleSystem(...)
#define LoopSound(...) nullptr
#define PauseSound(...)
#define StopSound(...)
#define PlaySound(...) nullptr
#define ResumeLoopSound(...)
#define FreeAsset(...)
#endif

enum SOUND_STATUS : u8
{
	SOUND_STATUS_Play,
	SOUND_STATUS_Pause,
	SOUND_STATUS_Stop,
	SOUND_STATUS_Loop
};

enum ASSET_STATE
{
	ASSET_STATE_Unloaded,
	ASSET_STATE_Queued,
	ASSET_STATE_Loaded
};

enum ASSET_TYPES : u8
{
	ASSET_TYPE_Bitmap,
	ASSET_TYPE_TexturePack,
	ASSET_TYPE_Font,
	ASSET_TYPE_Sound,
};

enum BITMAPS : u8
{
	//Unpacked textures go here
	BITMAP_None,
	BITMAP_Title,
	BITMAP_MainMenu1,
	BITMAP_MainMenu2,
	BITMAP_MainMenu3,
	BITMAP_MainMenu4,
	BITMAP_MainMenu5,
	BITMAP_Character,
	BITMAP_Target,
	BITMAP_Beer,
	BITMAP_Dust,
	BITMAP_Arrow,

	BITMAP_COUNT
};

enum FONTS : u8
{
	FONT_Normal,
	FONT_Debug,
	FONT_Title,

	FONT_COUNT
};

enum SOUNDS : u8
{
	SOUND_Beep,
	SOUND_Walking,
	SOUND_Arrows,
	SOUND_Background,
	SOUND_Sword,
	SOUND_COUNT
};

struct Task
{
	MemoryStack* arena;
	TemporaryMemoryHandle reset;
	bool is_used;
};

typedef void TaskCompleteCallback(void* pData);
struct TaskCallback
{
	TaskCompleteCallback* callback;
	void* data;
};

struct TaskCallbackQueue
{
	TaskCallback callbacks[2][32];
	u64 i;
};

struct Sound
{
	u32 channels;
	List<s16> samples;
};

struct Entity;
struct PlayingSound
{
	float volume;
	SOUNDS sound;
	Sound* loaded_sound;
	u32 samples_played;
	Entity* entity;
	SOUND_STATUS status;

	PlayingSound* next;
};


struct FontInfo
{
	float size;
	u32 atlasWidth;
	u32 atlasHeight;
	stbtt_packedchar* charInfo;
	float baseline;
	u8* data;
	u32 texture;
};

struct Bitmap
{
	s32 width;
	s32 height;
	s32 channels;
	u32 texture;
	v2 uv_min;
	v2 uv_max;
	u8* data;
};

struct ParalaxBitmap
{
	u32 layers;
	BITMAPS bitmaps[5];
	v2 position[5];
	float speed;
};

struct AnimatedBitmap
{
	BITMAPS bitmap;
	u32 animation_count;
	u32 current_animation;
	u32 current_index;
	u32* animation_lengths;
	v2 frame_size;
	float frame_time;
	float frame_duration;
};

struct AssetSlot
{
	u32 state;
	u64 last_requested;
	union 
	{
		Bitmap* bitmap;
		FontInfo* font;
		Sound* sound;
	};
	float size;

	ASSET_TYPES type;
	const char* load_path;
};

struct Assets
{
	AssetSlot fonts[FONT_COUNT];
	AssetSlot bitmaps[BITMAP_COUNT];
	AssetSlot sounds[SOUND_COUNT];
	u32 blank_texture;

	MemoryPool* memory;
};

struct LoadAssetWork
{
	Assets* assets;
	TaskCallbackQueue* queue;
	AssetSlot* slot;
	char load_info[MAX_PATH];
};

struct PackedTextureEntry
{
	BITMAPS bitmap;
	const char* file;
};

struct PackedTexture
{
	//PackedTextureEntry textures[BITMAP_COUNT - BITMAP_TexturePack - 1];

	const char* image;
	const char* config;
};

struct TextureAtlasWork
{
	Bitmap* bitmap;
	PackedTexture* atlas;
};

struct wav_header_t
{
	char chunkID[4]; //"RIFF" = 0x46464952
	unsigned long chunkSize;
	char format[4]; //"WAVE" = 0x45564157
	char subchunk1ID[4]; //"fmt " = 0x20746D66
	unsigned long subchunk1Size;
	unsigned short audioFormat;
	unsigned short numChannels;
	unsigned long sampleRate;
	unsigned long byteRate;
	unsigned short blockAlign;
	unsigned short bitsPerSample;
};

struct chunk_t
{
	char ID[4]; //"data" = 0x61746164
	unsigned long size;
};

static AnimatedBitmap CreateAnimatedBitmap(BITMAPS pBitmap, u32 pAnimationCount, u32* pAnimationLengths, v2 pFrameSize)
{
	AnimatedBitmap bitmap;
	bitmap.bitmap = pBitmap;
	bitmap.animation_count = pAnimationCount;
	bitmap.current_animation = 0;
	bitmap.current_index = 0;
	bitmap.frame_size = pFrameSize;
	bitmap.animation_lengths = pAnimationLengths;
	bitmap.frame_time = 0;
	return bitmap;
}

static void UpdateAnimation(AnimatedBitmap* pBitmap, float pDeltaTime)
{
	if ((pBitmap->frame_time += pDeltaTime) >= pBitmap->frame_duration)
	{
		pBitmap->frame_time = 0;
		pBitmap->current_index++;
		if (pBitmap->current_index >= *(pBitmap->animation_lengths + pBitmap->current_animation))
		{
			pBitmap->current_index = 0;
		}
	}
}

static bool AnimationIsComplete(AnimatedBitmap* pBitmap)
{
	return pBitmap->current_index == pBitmap->animation_lengths[pBitmap->current_animation] - 1;
}

void RequestAsset(Assets* pAssets, AssetSlot* pAsset, ASSET_TYPES pType);
void FreeAsset(AssetSlot* pSlot);