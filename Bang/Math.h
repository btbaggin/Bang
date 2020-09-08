#pragma once

static void GenerateRandomNumbersWithNoDuplicates(u32* pNumbers, u32 pCount)
{
	//Generate numbers
	for (u32 i = 0; i < pCount; i++) pNumbers[i] = i;

	//Shuffle array randomly
	for (u32 i = 0; i < pCount - 1; i++)
	{
		u32 j = i + rand() / (RAND_MAX / (pCount - i) + 1);
		u32 t = pNumbers[j];
		pNumbers[j] = pNumbers[i];
		pNumbers[i] = t;
	}
}

const float EPSILON = 0.0001f;

inline static bool IsZero(v2 a) { return a.X == 0 && a.Y == 0; }

static bool operator<(v2 l, v2 r)
{
	return l.X < r.X && l.Y < r.Y;
}
static bool operator>(v2 l, v2 r)
{
	return l.X > r.X && l.Y > r.Y;
}

inline float Cross(v2 a, v2 b)
{
	return a.X * b.Y - a.Y * b.X;
}

inline v2 Cross(v2 v, float a)
{
	return { a * v.Y, -a * v.X };
}

inline v2 Cross(float a, const v2 v)
{
	return { -a * v.Y, a * v.X };
}

inline float DistSqr(v2 a, v2 b)
{
	v2 c = a - b;
	return HMM_Dot(c, c);
}

inline static bool Equal(float a, float b)
{
	// <= instead of < for NaN comparison safety
	return std::abs(a - b) <= EPSILON;
}

struct mat2
{
	union
	{
		struct
		{
			float m00, m01;
			float m10, m11;
		};

		float m[2][2];
		float v[4];
	};
};

inline static void Rotate(mat2* pMat, float pRadians)
{
	float c = std::cos(pRadians);
	float s = std::sin(pRadians);

	pMat->m00 = c; pMat->m01 = -s;
	pMat->m10 = s; pMat->m11 = c;
}

mat2 Transpose(mat2* pMat)
{
	return { pMat->m00, pMat->m10, pMat->m01, pMat->m11 };
}

const v2 operator*(mat2 m, v2 rhs)
{
	return { m.m00 * rhs.X + m.m01 * rhs.Y, m.m10 * rhs.X + m.m11 * rhs.Y };
}

static inline u32 Random(u32 pMin, u32 pMax)
{
	return rand() % (pMax - pMin) + pMin;
}

static inline float Random()
{
	return (float)rand() / (float)(RAND_MAX + 1);
}

static inline float Random(float pMin, float pMax)
{
	return Random() * (pMax - pMin) + pMin;
}
static inline float Random(Range pRange)
{
	return Random(pRange.min, pRange.max);
}

#define Megabytes(size) size * 1024 * 1024

struct Rect 
{
	s32 x;
	s32 y;
	s32 w;
	s32 h;
};

bool isPointInside(Rect pRect, v2 pPoint) 
{ 
	return pPoint.X >= pRect.x && pPoint.X < pRect.x + pRect.w && pPoint.Y >= pRect.y && pPoint.Y < pRect.y + pRect.h; 
}