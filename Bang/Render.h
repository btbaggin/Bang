#pragma once
#include "shaders.h"
const float Z_INDEX_DEPTH = 20.0F;
enum Z_LAYERS
{
	Z_LAYER_Background1,
	Z_LAYER_Background2,
	Z_LAYER_Background3,
	Z_LAYER_Background4,
	Z_LAYER_Player,
	Z_LAYER_Ui,
	Z_LAYER_Modal,
	Z_LAYER_MAX,
};

enum RENDER_GROUP_ENTRY_TYPE : u8
{
	RENDER_GROUP_ENTRY_TYPE_Line,
	RENDER_GROUP_ENTRY_TYPE_Text,
	RENDER_GROUP_ENTRY_TYPE_Quad,
	RENDER_GROUP_ENTRY_TYPE_Matrix,
	RENDER_GROUP_ENTRY_TYPE_ParticleSystem,
};

enum RENDER_TEXT_FLAGS : u8
{
	RENDER_TEXT_FLAG_Clip = 1
};



struct Renderable_Text
{
	u32 first_index;
	u32 first_vertex;
	u32 index_count;
	u32 texture;
	v2 clip_min;
	v2 clip_max;
	u8 flags;
};

struct Renderable_Quad
{
	u32 first_index;
	u32 first_vertex;
	u32 texture;
};

struct Renderable_Line
{
	u32 first_vertex;
	float size;
};

struct Renderable_Matrix
{
	mat4 matrix;
};

struct Renderable_ParticleSystem
{
	u32 particle_count;
	u32 texture;
	u32 VAO;
	u32 VBO;
	u32 CBO;
	u32 PBO;
};

struct RenderEntry
{
	RENDER_GROUP_ENTRY_TYPE type;
	u8 size;
};

struct Vertex
{
	v3 position;
	v4 color;
	v2 uv;
};

struct GlyphQuad
{
	Vertex vertices[4];
};

struct GLProgram
{
	u32 id;
	u32 texture;
	u32 mvp;
	u32 font;
};

struct RenderState
{
	MemoryStack* arena;
	TemporaryMemoryHandle memory;
	u32 VAO;
	u32 VBO;
	u32 EBO;

	float z_index;

	GLProgram program;
	GLProgram particle_program;

	MemoryStack* vertices;
	u32 vertex_count;

	MemoryStack* indices;
	u32 index_count;

	u32 entry_count;
};