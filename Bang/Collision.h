#pragma once
#define MAX_POLY_VERTEX_COUNT 16
#define PHYSICS_ITERATIONS 10
const v2 GRAVITY = V2(0);

#define DEBUG_DRAW_RIGIDBODIES (0 && !_SERVER && _DEBUG)

enum SHAPES
{
	SHAPE_Circle,
	SHAPE_Poly,
	SHAPE_Count
};

struct RigidBody;
struct Shape
{
	RigidBody* body;
	SHAPES type;

	union
	{
		float radius;
		struct
		{
			mat2 u; // Orientation matrix from model to world
			u32 vertex_count;
			v2 vertices[MAX_POLY_VERTEX_COUNT];
			v2 normals[MAX_POLY_VERTEX_COUNT];
#if DEBUG_DRAW_RIGIDBODIES
			v2 size;
			v2 offset;
#endif
		};
	};
};

struct RigidBody
{
	Entity* entity;
	v2 velocity;

	float angular_velocity;
	float torque;
	float orient; // radians

	v2 force;

	// Set by shape
	float inertia;
	float inverse_inertia;
	float mass; 
	float inverse_mass;

	// http://gamedev.tutsplus.com/tutorials/implementation/how-to-create-a-custom-2d-physics-engine-friction-scene-and-jump-table/
	float static_friction;
	float dynamic_friction;
	float restitution;

	Shape *shape;
};

struct PhysicsMaterial
{
	float static_friction;
	float dynamic_friction;
	float restitution;
};

struct RigidBodyCreationOptions
{
	SHAPES type;
	Entity* entity;
	PhysicsMaterial material;

	float radius;
	float width;
	float height;
	v2 offset;

	float density;
};

struct PhysicsScene
{
	std::vector<RigidBody*> bodies;
};

void SetOrient(Shape* pShape, float radians);
