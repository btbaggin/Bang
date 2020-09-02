//https://github.com/RandyGaul/cute_headers/blob/master/cute_tiled.h
//http://www.mapeditor.org/
#include "LevelCollisionMask.cpp"
#ifndef _SERVER
static void RenderTiledmap(Assets* pAssets, const char* pPath, s32 pWidth, s32 pHeight, TilesetList pTiles, cute_tiled_layer_t* pLayer)
{
	for (u32 i = 0; i < (u32)pLayer->data_count; i++)
	{
		int gid = pLayer->data[i];
		if (gid)
		{
			for (u32 j = 0; j < pTiles.count; j++)
			{
				Tileset tile = pTiles.tiles[j];
				cute_tiled_tileset_t* set = tile.set;
				if (gid >= set->firstgid && gid < set->firstgid + set->tilecount)
				{
					u32 w = set->tilewidth;
					u32 h = set->tileheight;
					u32 columns = set->columns;

					int gid_index = gid - set->firstgid;
					u32 r = gid_index / columns;
					u32 c = gid_index % columns;

					u32 px = c * w + set->spacing;
					u32 py = r * h + set->spacing;
					float width = (float)tile.bitmap->width;
					float height = (float)tile.bitmap->height;
					tile.bitmap->uv_min = V2(px / width, py / height);
					tile.bitmap->uv_max = V2((px + w) / width, (py + h) / height);

					u32 ir = pHeight - (i / pWidth) - 1;
					u32 ic = i % pWidth;
					SetZLayer(g_transstate.render_state, Z_LAYER_Background1);
					PushSizedQuad(g_transstate.render_state, V2(ic * (float)set->tilewidth, ir * (float)set->tileheight), V2((float)set->tilewidth, (float)set->tileheight), tile.bitmap);
					break;
				}
			}
		}
	}
}

static TilesetList LoadTilesets(char* pPath, cute_tiled_map_t* pMap, MemoryStack* pStack)
{
	PathRemoveFileSpecA(pPath);

	TilesetList list = {};
	cute_tiled_tileset_t* set = pMap->tilesets;
	while (set)
	{
		char path[MAX_PATH];
		PathCombineA(path, pPath, set->image.ptr);

		Tileset* tiles = PushStruct(pStack, Tileset);
		tiles->bitmap = LoadBitmapAsset(g_transstate.assets, path);
		SendTextureToGraphicsCard(tiles->bitmap);
		
		if (!list.tiles) list.tiles = tiles;
		tiles->set = set;
		list.count++;

		set = set->next;
	}

	return list;
}

static void FreeTilesets(TilesetList pList)
{
	for (u32 i = 0; i < pList.count; i++)
	{
		Tileset* set = pList.tiles + i;
		Free(set->bitmap);
		glDeleteTextures(1, &set->bitmap->texture);
	}
}
#endif

static void LoadTiledMap(TiledMap* pMap, const char* pPath, MemoryStack* pStack)
{
	char path[MAX_PATH];
	GetFullPath(pPath, path);

	TemporaryMemoryHandle h = BeginTemporaryMemory(pStack);	
	cute_tiled_map_t* tiled_map = cute_tiled_load_map_from_file(path, 0);

	// get map width and height
	pMap->width = tiled_map->width * tiled_map->tilewidth;
	pMap->height = tiled_map->height * tiled_map->tileheight;
	
	cute_tiled_layer_t* layer = tiled_map->layers;
#ifndef _SERVER
	Bitmap* bitmap = AllocStruct(g_transstate.assets, Bitmap);
	bitmap->uv_min = V2(0);
	bitmap->uv_max = V2(1);
	bitmap->width = (s32)pMap->width;
	bitmap->height = (s32)pMap->height;

	TilesetList tile_list = LoadTilesets(path, tiled_map, pStack);

	u32 fb = BeginRenderToTexture(bitmap, g_transstate.render_state);

	// Render each layer to our temporary bitmap
	while (layer)
	{ 
		RenderTiledmap(g_transstate.assets, path, tiled_map->width, tiled_map->height, tile_list, layer);
		layer = layer->next;
	}

	EndRenderToTexture(bitmap, g_transstate.render_state, fb);
	pMap->bitmap = bitmap;

	FreeTilesets(tile_list);
#endif

	layer = tiled_map->layers;
	while (layer)
	{
		GetLevelCollisionMask(tiled_map, layer, pStack);
		layer = layer->next;
	}

	cute_tiled_free_map(tiled_map);
	EndTemporaryMemory(h);
}
