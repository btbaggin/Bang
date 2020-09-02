#pragma once
#define CUTE_TILED_IMPLEMENTATION
#include "cute\cute_tiled.h"

struct Bitmap;
struct Tileset
{
	Bitmap* bitmap;
	cute_tiled_tileset_t* set;
};

struct TilesetList
{
	Tileset* tiles;
	u32 count;
};

struct TiledMap
{
	Bitmap* bitmap;
	u32 width;
	u32 height;
};