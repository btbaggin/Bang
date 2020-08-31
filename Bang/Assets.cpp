#include <fstream>
#include <sstream>
#include "Assets.h"
#include "Sound.cpp"
#include "Font.cpp"
#include "Bitmap.cpp"
static void AddTaskCallback(TaskCallbackQueue* pQueue, TaskCompleteCallback* pFunc, void* pData);

//static void AddBitmapAsset(Assets* pAssets, Bitmap* pTexture, TextureAtlas* pAtlas, BITMAPS pName, const char* pPath)
//{
//	AssetSlot* slot = pAssets->bitmaps + pName;
//	slot->type = ASSET_TYPE_Bitmap;
//	slot->state = ASSET_STATE_Loaded;
//
//	slot->bitmap = AllocStruct(pAssets, Bitmap);
//	memcpy(slot->bitmap, pTexture, sizeof(Bitmap));
//	for (u32 i = 0; i < pAtlas->count; i++)
//	{
//		TextureAtlasEntry e = pAtlas->entries[i];
//		if (strcmp(e.file, pPath) == 0)
//		{
//			slot->bitmap->uv_min = V2(e.u_min, e.v_min);
//			slot->bitmap->uv_max = V2(e.u_max, e.v_max);
//			return;
//		}
//	}
//}
//static void LoadTexturePack(void* pData)
//{
//	TextureAtlasWork* work = (TextureAtlasWork*)pData;
//	if (work->bitmap)
//	{
//		SendTextureToGraphicsCard(work->bitmap);
//
//		TextureAtlas atlas = ta_ReadTextureAtlas(work->atlas->config);
//		//for (u32 i = 0; i < ArrayCount(work->atlas->textures); i++)
//		//{
//			//PackedTextureEntry t = work->atlas->textures[i];
//			//AddBitmapAsset(g_transstate.assets, work->bitmap, &atlas, t.bitmap, t.file);
//		//}
//
//		ta_DisposeTextureAtlas(&atlas);
//	}
//	delete pData;
//	delete work->atlas;
//}

static Bitmap* LoadBitmapAsset(Assets* pAssets, const char* pPath)
{
	Bitmap* bitmap = AllocStruct(pAssets, Bitmap);
	bitmap->uv_min = V2(0);
	bitmap->uv_max = V2(1);

	bitmap->data = stbi_load(pPath, &bitmap->width, &bitmap->height, &bitmap->channels, STBI_rgb_alpha);
	if (!bitmap->data) return nullptr;

	return bitmap;
}



WORK_QUEUE_CALLBACK(LoadAssetBackground)
{
	LoadAssetWork* work = (LoadAssetWork*)pData;

	switch (work->slot->type)
	{
		case ASSET_TYPE_Bitmap:
			work->slot->bitmap = LoadBitmapAsset(work->assets, work->load_info);
			AddTaskCallback(work->queue, SendTextureToGraphicsCard, work->slot->bitmap);
			break;

		//case ASSET_TYPE_TexturePack:
		//{
		//	work->slot->bitmap = LoadBitmapAsset(work->assets, work->load_info);

		//	TextureAtlasWork* ta_work = new TextureAtlasWork();
		//	ta_work->bitmap = work->slot->bitmap;
		//	ta_work->atlas = (PackedTexture*)work->data;
		//	AddTaskCallback(work->queue, LoadTexturePack, ta_work);
		//}
		//break;

		case ASSET_TYPE_Font:
			work->slot->font = LoadFontAsset(work->assets, work->load_info, work->slot->size);
			AddTaskCallback(work->queue, SendFontToGraphicsCard, work->slot->font);
			break;

		case ASSET_TYPE_Sound:
			work->slot->sound = LoadSoundAsset(work->assets, work->load_info);
			break;
	}
	_InterlockedExchange(&work->slot->state, ASSET_STATE_Loaded);
	delete work;
}

static void LoadAsset(Assets* pAssets, AssetSlot* pSlot, ASSET_TYPES pType)
{
	if (_InterlockedCompareExchange(&pSlot->state, ASSET_STATE_Queued, ASSET_STATE_Unloaded) == ASSET_STATE_Unloaded)
	{
		LogInfo("Loading asset %s", pSlot->load_path);

		LoadAssetWork* work = new LoadAssetWork();
		GetFullPath(pSlot->load_path, work->load_info);
		work->slot = pSlot;
		work->assets = pAssets;
		work->queue = &g_state.callbacks;
		QueueUserWorkItem(g_state.work_queue, LoadAssetBackground, work);
	}
}

void RequestAsset(Assets* pAssets, AssetSlot* pAsset, ASSET_TYPES pType)
{
	pAsset->last_requested = __rdtsc();
	if (pAsset->state == ASSET_STATE_Unloaded)
	{
		LoadAsset(pAssets, pAsset, pType);
	}
}

static void AddBitmapAsset(Assets* pAssets, BITMAPS pName, const char* pPath)
{
	AssetSlot* slot = pAssets->bitmaps + pName;
	slot->type = ASSET_TYPE_Bitmap;
	slot->load_path = pPath;
}

static void AddFontAsset(Assets* pAssets, FONTS pName, const char* pPath, float pSize)
{
	AssetSlot* slot = pAssets->fonts + pName;
	slot->type = ASSET_TYPE_Font;
	slot->load_path = pPath;
	slot->size = pSize;
}

static void AddSoundAsset(Assets* pAssets, SOUNDS pName, const char* pPath)
{
	AssetSlot* slot = pAssets->sounds + pName;
	slot->type = ASSET_TYPE_Sound;
	slot->load_path = pPath;
}

//static void LoadPackedTexture(Assets* pAssets, BITMAPS pName, PackedTexture* pAtlas)
//{
//	AssetSlot* slot = pAssets->bitmaps + pName;
//	slot->type = ASSET_TYPE_TexturePack;
//	slot->load_path = pAtlas->image;
//
//	//LoadAsset(pAssets, slot, pAtlas);
//}

static Assets* LoadAssets(MemoryStack* pStack, u64 pSize)
{
	Assets* assets = PushStruct(pStack, Assets);
	g_transstate.assets = assets;
	void* AssetMemory = PushSize(pStack, pSize);

	assets->memory = CreateMemoryPool(AssetMemory, pSize);
	
	AddFontAsset(assets, FONT_Debug, "C:\\Windows\\Fonts\\cour.ttf", 20);
	AddFontAsset(assets, FONT_Normal, "..\\..\\Resources\\font.ttf", 14);
	AddFontAsset(assets, FONT_Title, "..\\..\\Resources\\font.ttf", 32);

	AddSoundAsset(assets, SOUND_Beep, "..\\..\\Resources\\beep.wav");
	AddSoundAsset(assets, SOUND_Walking, "..\\..\\Resources\\walking.wav");
	//AddBitmapAsset(assets, BITMAP_Background, ".\\Assets\\background.jpg");
	//AddBitmapAsset(assets, BITMAP_Placeholder, ".\\Assets\\placeholder.jpg");

	AddBitmapAsset(assets, BITMAP_MainMenu1, "..\\..\\Resources\\Main Menu\\1.png");
	AddBitmapAsset(assets, BITMAP_MainMenu2, "..\\..\\Resources\\Main Menu\\2.png");
	AddBitmapAsset(assets, BITMAP_MainMenu3, "..\\..\\Resources\\Main Menu\\3.png");
	AddBitmapAsset(assets, BITMAP_MainMenu4, "..\\..\\Resources\\Main Menu\\4.png");
	AddBitmapAsset(assets, BITMAP_MainMenu5, "..\\..\\Resources\\Main Menu\\5.png");
	AddBitmapAsset(assets, BITMAP_Title, "..\\..\\Resources\\title.png");
	AddBitmapAsset(assets, BITMAP_Character, "..\\..\\Resources\\character.png");
	AddBitmapAsset(assets, BITMAP_Target, "..\\..\\Resources\\renegade.png");
	/*PackedTexture* text = new PackedTexture();
	text->textures[0] = { BITMAP_Error, "error.png" };
	text->textures[1] = { BITMAP_Question, "question.png" };
	text->textures[2] = { BITMAP_ArrowUp, "arrow_up.png" };
	text->textures[3] = { BITMAP_ArrowDown, "arrow_down.png" };
	text->textures[4] = { BITMAP_ButtonA, "button_a.png" };
	text->textures[5] = { BITMAP_ButtonB, "button_b.png" };
	text->textures[6] = { BITMAP_ButtonX, "button_x.png" };
	text->textures[7] = { BITMAP_ButtonY, "button_y.png" };
	text->textures[8] = { BITMAP_App, "apps.png" };
	text->textures[9] = { BITMAP_Emulator, "emulator.png" };
	text->textures[10] = { BITMAP_Recent, "recents.png" };
	text->textures[11] = { BITMAP_Speaker, "speaker.png" };
	text->textures[12] = { BITMAP_Settings, "settings.png" };
	text->config = ".\\Assets\\atlas.tex";
	text->image = ".\\Assets\\packed.png";
	LoadPackedTexture(assets, BITMAP_TexturePack, text);*/

	//Put a 1px white texture so we get fully lit instead of only ambient lighting
	u8 data[] = { 255, 255, 255, 255 };
	glGenTextures(1, &assets->blank_texture);
	glBindTexture(GL_TEXTURE_2D, assets->blank_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return assets;
}

void FreeAsset(AssetSlot* pSlot)
{
	if (pSlot->state == ASSET_STATE_Loaded)
	{
		switch (pSlot->type)
		{
			case ASSET_TYPE_Bitmap:
				Free(pSlot->bitmap);
				glDeleteTextures(1, &pSlot->bitmap->texture);
				break;

			case ASSET_TYPE_Sound:
				Free(pSlot->sound);
				break;

			case ASSET_TYPE_Font:
				Free(pSlot->font);
				glDeleteTextures(1, &pSlot->font->texture);
				break;

			default:
				assert(false);
		}
		pSlot->bitmap = nullptr;
		pSlot->last_requested = 0;
		InterlockedExchange(&pSlot->state, ASSET_STATE_Unloaded);
	}
}

void FreeAllAssets(Assets* pAssets)
{
	for (u32 i = 0; i < BITMAP_COUNT; i++)
		FreeAsset(pAssets->bitmaps + i);
	for (u32 i = 0; i < FONT_COUNT; i++)
		FreeAsset(pAssets->fonts + i);
	for (u32 i = 0; i < SOUND_COUNT; i++)
		FreeAsset(pAssets->sounds + i);
}

void EvictOldestAsset(Assets* pAssets)
{
	AssetSlot* slot = nullptr;
	u64 request = __rdtsc();

	//TODO 


	FreeAsset(slot);
}