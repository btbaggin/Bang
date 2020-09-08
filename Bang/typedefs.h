#pragma once
#include "stdint.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath/HandmadeMath.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef unsigned char u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef signed char s8;
typedef hmm_vec4 v4;
typedef hmm_vec3 v3;
typedef hmm_vec2 v2;
typedef hmm_mat4 mat4;

struct Range
{
	float min;
	float max;
};

#include "Math.h"

#define clamp HMM_Clamp

static v2 V2(float xy)
{
	return { xy, xy };
}
static v2 V2(float x, float y)
{
	return { x, y };
}
static v3 V3(float x, float y, float z)
{
	return { x, y, z };
}
static v3 V3(float xyz)
{
	return { xyz, xyz, xyz };
}
static v4 V4(float x, float y, float z, float w)
{
	return { x, y, z, w };
}
static v4 V4(float xyzw)
{
	return { xyzw, xyzw, xyzw, xyzw };
}

static v2 Lerp(v2 a, float time, v2 b)
{
	v2 result;
	result.X = HMM_Lerp(a.X, time, b.X);
	result.Y = HMM_Lerp(a.Y, time, b.Y);
	if (abs(result.X - b.X) < 0.05) result.X = b.X;
	if (abs(result.Y - b.Y) < 0.05) result.Y = b.Y;
	return result;
}

static float SmoothStep(float edge0, float edge1, float x)
{
	return clamp(0.0, (x - edge0) / (edge1 - edge0), 1.0);
	return x * x*(3 - 2 * x);
}

#define ArrayCount(a) sizeof(a) / sizeof(a[0])
#define HasFlag(flag, value) (flag & value) != 0
