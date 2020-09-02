//https://github.com/RandyGaul/ImpulseEngine
#include "PhysicsBody.cpp"
#include "Shape.cpp"
#include "Manifold.cpp"

static void CircletoCircle(Manifold* pManifold, RigidBody* a, RigidBody* b)
{
	Shape *A = a->shape;
	Shape *B = b->shape;

	// Calculate translational vector, which is normal
	v2 normal = b->entity->position - a->entity->position;

	float dist_sqr = HMM_LengthSquared(normal);
	float radius = A->radius + B->radius;

	// Not in contact
	if (dist_sqr >= radius * radius)
	{
		pManifold->contact_count = 0;
		return;
	}

	float distance = sqrt(dist_sqr);

	pManifold->contact_count = 1;

	if (distance == 0.0f)
	{
		pManifold->penetration = A->radius;
		pManifold->normal = V2(1, 0);
		pManifold->contacts[0] = a->entity->position;
	}
	else
	{
		pManifold->penetration = radius - distance;
		pManifold->normal = normal / distance; // Faster than using Normalized since we already performed sqrt
		pManifold->contacts[0] = pManifold->normal * A->radius + a->entity->position;
	}
}

void CircletoPolygon(Manifold* pManifold, RigidBody* a, RigidBody* b)
{
	Shape *A = a->shape;
	Shape *B = b->shape;
	assert(A->type == SHAPE_Circle);
	assert(B->type == SHAPE_Poly);

	pManifold->contact_count = 0;

	// Transform circle center to Polygon model space
	v2 center = a->entity->position;
	center = Transpose(&B->u) * (center - b->entity->position);

	// Find edge with minimum penetration
	// Exact concept as using support points in Polygon vs Polygon
	float separation = -FLT_MAX;
	u32 faceNormal = 0;
	for (u32 i = 0; i < B->vertex_count; ++i)
	{
		float s = HMM_Dot(B->normals[i], center - B->vertices[i]);

		if (s > A->radius)
			return;

		if (s > separation)
		{
			separation = s;
			faceNormal = i;
		}
	}

	// Grab face's vertices
	v2 v1 = B->vertices[faceNormal];
	u32 i2 = faceNormal + 1 < B->vertex_count ? faceNormal + 1 : 0;
	v2 V2 = B->vertices[i2];

	// Check to see if center is within polygon
	if (separation < EPSILON)
	{
		pManifold->contact_count = 1;
		pManifold->normal = (B->u * B->normals[faceNormal]) * -1;
		pManifold->contacts[0] = pManifold->normal * A->radius + a->entity->position;
		pManifold->penetration = A->radius;
		return;
	}

	// Determine which voronoi region of the edge center of circle lies within
	float dot1 = HMM_Dot(center - v1, V2 - v1);
	float dot2 = HMM_Dot(center - V2, v1 - V2);
	pManifold->penetration = A->radius - separation;

	// Closest to v1
	if (dot1 <= 0.0f)
	{
		if (DistSqr(center, v1) > A->radius * A->radius)
			return;

		pManifold->contact_count = 1;
		v2 n = v1 - center;
		n = B->u * n;
		n = HMM_Normalize(n);
		pManifold->normal = n;
		v1 = B->u * v1 + b->entity->position;
		pManifold->contacts[0] = v1;
	}

	// Closest to v2
	else if (dot2 <= 0.0f)
	{
		if (DistSqr(center, V2) > A->radius * A->radius)
			return;

		pManifold->contact_count = 1;
		v2 n = V2 - center;
		V2 = B->u * V2 + b->entity->position;
		pManifold->contacts[0] = V2;
		n = B->u * n;
		n = HMM_Normalize(n);
		pManifold->normal = n;
	}

	// Closest to face
	else
	{
		v2 n = B->normals[faceNormal];
		if (HMM_Dot(center - v1, n) > A->radius)
			return;

		n = B->u * n;
		pManifold->normal = n* -1;
		pManifold->contacts[0] = pManifold->normal * A->radius + a->entity->position;
		pManifold->contact_count = 1;
	}
}

void PolygontoCircle(Manifold* pManifold, RigidBody* a, RigidBody* b)
{
	CircletoPolygon(pManifold, b, a);
	pManifold->normal = pManifold->normal * -1;
}

float FindAxisLeastPenetration(u32* pIndex, Shape* A, Shape* B)
{
	float bestDistance = -FLT_MAX;
	u32 bestIndex = 0;

	for (u32 i = 0; i < A->vertex_count; ++i)
	{
		// Retrieve a face normal from A
		v2 n = A->normals[i];
		v2 nw = A->u * n;

		// Transform face normal into B's model space
		mat2 buT = Transpose(&B->u);
		n = buT * nw;

		// Retrieve support point from B along -n
		v2 s = GetSupport(B, n * -1);

		// Retrieve vertex on face from A, transform into
		// B's model space
		v2 v = A->vertices[i];
		v = A->u * v + A->body->entity->position;
		v -= B->body->entity->position;
		v = buT * v;

		// Compute penetration distance (in B's model space)
		float d = HMM_Dot(n, s - v);

		// Store greatest distance
		if (d > bestDistance)
		{
			bestDistance = d;
			bestIndex = i;
		}
	}

	*pIndex = bestIndex;
	return bestDistance;
}

static void FindIncidentFace(v2* v, Shape* RefPoly, Shape* IncPoly, u32 referenceIndex)
{
	v2 referenceNormal = RefPoly->normals[referenceIndex];

	// Calculate normal in incident's frame of reference
	referenceNormal = RefPoly->u * referenceNormal; // To world space
	Transpose(&IncPoly->u);
	referenceNormal =  IncPoly->u * referenceNormal; // To incident's model space

	// Find most anti-normal face on incident polygon
	s32 incidentFace = 0;
	float minDot = FLT_MAX;
	for (u32 i = 0; i < IncPoly->vertex_count; ++i)
	{
		float dot = HMM_Dot(referenceNormal, IncPoly->normals[i]);
		if (dot < minDot)
		{
			minDot = dot;
			incidentFace = i;
		}
	}

	// Assign face vertices for incidentFace
	v[0] = IncPoly->u * IncPoly->vertices[incidentFace] + IncPoly->body->entity->position;
	incidentFace = incidentFace + 1 >= (s32)IncPoly->vertex_count ? 0 : incidentFace + 1;
	v[1] = IncPoly->u * IncPoly->vertices[incidentFace] + IncPoly->body->entity->position;
}

static s32 Clip(v2 pNormal, float c, v2* face)
{
	u32 sp = 0;
	v2 out[2] = { face[0], face[1] };

	// Retrieve distances from each endpoint to the line
	// d = ax + by - c
	float d1 = HMM_Dot(pNormal, face[0]) - c;
	float d2 = HMM_Dot(pNormal, face[1]) - c;

	// If negative (behind plane) clip
	if (d1 <= 0.0f) out[sp++] = face[0];
	if (d2 <= 0.0f) out[sp++] = face[1];

	// If the points are on different sides of the plane
	if (d1 * d2 < 0.0f) // less than to ignore -0.0f
	{
		// Push interesection point
		float alpha = d1 / (d1 - d2);
		out[sp] = face[0] + alpha * (face[1] - face[0]);
		++sp;
	}

	// Assign our new converted values
	face[0] = out[0];
	face[1] = out[1];

	assert(sp != 3);

	return sp;
}

inline static bool BiasGreaterThan(float a, float b)
{
	return a >= b * 0.95f + a * 0.01f;
}

static void PolygontoPolygon(Manifold* pManifold, RigidBody* a, RigidBody* b)
{
	Shape *A = a->shape;
	Shape *B = b->shape;
	assert(A->type == SHAPE_Poly);
	assert(B->type == SHAPE_Poly);
	pManifold->contact_count = 0;

	// Check for a separating axis with A's face planes
	u32 faceA;
	float penetrationA = FindAxisLeastPenetration(&faceA, A, B);
	if (penetrationA >= 0.0f)
		return;

	// Check for a separating axis with B's face planes
	u32 faceB;
	float penetrationB = FindAxisLeastPenetration(&faceB, B, A);
	if (penetrationB >= 0.0f)
		return;

	u32 referenceIndex;
	bool flip; // Always point from a to b

	Shape* RefPoly; // Reference
	Shape* IncPoly; // Incident

	// Determine which shape contains reference face
	if (BiasGreaterThan(penetrationA, penetrationB))
	{
		RefPoly = A;
		IncPoly = B;
		referenceIndex = faceA;
		flip = false;
	}

	else
	{
		RefPoly = B;
		IncPoly = A;
		referenceIndex = faceB;
		flip = true;
	}

	// World space incident face
	v2 incidentFace[2];
	FindIncidentFace(incidentFace, RefPoly, IncPoly, referenceIndex);

	//        y
	//        ^  ->n       ^
	//      +---c ------posPlane--
	//  x < | i |\
    //      +---+ c-----negPlane--
	//             \       v
	//              r
	//
	//  r : reference face
	//  i : incident poly
	//  c : clipped point
	//  n : incident normal

	// Setup reference face vertices
	v2 v1 = RefPoly->vertices[referenceIndex];
	referenceIndex = referenceIndex + 1 == RefPoly->vertex_count ? 0 : referenceIndex + 1;
	v2 V2 = RefPoly->vertices[referenceIndex];

	// Transform vertices to world space
	v1 = RefPoly->u * v1 + RefPoly->body->entity->position;
	V2 = RefPoly->u * V2 + RefPoly->body->entity->position;

	// Calculate reference face side normal in world space
	v2 sidePlaneNormal = (V2 - v1);
	sidePlaneNormal = HMM_Normalize(sidePlaneNormal);

	// Orthogonalize
	v2 refFaceNormal = { sidePlaneNormal.Y, -sidePlaneNormal.X };

	// ax + by = c
	// c is distance from origin
	float refC = HMM_Dot(refFaceNormal, v1);
	float negSide = -HMM_Dot(sidePlaneNormal, v1);
	float posSide = HMM_Dot(sidePlaneNormal, V2);

	// Clip incident face to reference face side planes
	if (Clip(sidePlaneNormal * -1, negSide, incidentFace) < 2)
		return; // Due to floating point error, possible to not have required points

	if (Clip(sidePlaneNormal, posSide, incidentFace) < 2)
		return; // Due to floating point error, possible to not have required points

	  // Flip
	pManifold->normal = flip ? refFaceNormal * -1 : refFaceNormal;

	// Keep points behind reference face
	u32 cp = 0; // clipped points behind reference face
	float separation = HMM_Dot(refFaceNormal, incidentFace[0]) - refC;
	if (separation <= 0.0f)
	{
		pManifold->contacts[cp] = incidentFace[0];
		pManifold->penetration = -separation;
		++cp;
	}
	else
		pManifold->penetration = 0;

	separation = HMM_Dot(refFaceNormal, incidentFace[1]) - refC;
	if (separation <= 0.0f)
	{
		pManifold->contacts[cp] = incidentFace[1];

		pManifold->penetration += -separation;
		++cp;

		// Average penetration
		pManifold->penetration /= (float)cp;
	}

	pManifold->contact_count = cp;
}

typedef void(*CollisionCallback)(Manifold *m, RigidBody *a, RigidBody *b);
CollisionCallback Dispatch[SHAPE_Count][SHAPE_Count] = {
  { CircletoCircle, CircletoPolygon },
  { PolygontoCircle, PolygontoPolygon },
};

static void SolveManifold(Manifold* m)
{
	Dispatch[m->A->shape->type][m->B->shape->type](m, m->A, m->B);
}

// see http://www.niksula.hut.fi/~hkankaan/Homepages/gravity.html
void IntegrateForces(RigidBody* pBody, float pDeltaTime)
{
	if (pBody->inverse_mass == 0.0f)
		return;

	pBody->velocity += (pBody->force * pBody->inverse_mass + GRAVITY) * (pDeltaTime / 2.0f);
	pBody->angular_velocity += pBody->torque * pBody->inverse_inertia * (pDeltaTime / 2.0f);
}

void IntegrateVelocity(RigidBody* pBody, float pDeltaTime)
{
	if (pBody->inverse_mass == 0.0f)
		return;

	pBody->entity->position += pBody->velocity * pDeltaTime;
	pBody->orient += pBody->angular_velocity * pDeltaTime;
	SetOrient(pBody->shape, pBody->orient);
	IntegrateForces(pBody, pDeltaTime);
}

void StepPhysics(PhysicsScene* pScene, float pDeltaTime)
{
	// Generate new collision info
	std::vector<Manifold> contacts;
	for (u32 i = 0; i < pScene->bodies.size(); ++i)
	{
		RigidBody *A = pScene->bodies[i];
		for (u32 j = i + 1; j < pScene->bodies.size(); ++j)
		{
			RigidBody *B = pScene->bodies[j];
			if (A->inverse_mass == 0 && B->inverse_mass == 0)
				continue;
			Manifold m = {};
			m.A = A;
			m.B = B;
			SolveManifold(&m);
			if (m.contact_count)
				contacts.emplace_back(m);
		}
	}

	// Integrate forces
	for (u32 i = 0; i < pScene->bodies.size(); ++i)
		IntegrateForces(pScene->bodies[i], pDeltaTime);

	// Initialize collision
	for (u32 i = 0; i < contacts.size(); ++i)
		InitializeManifold(&contacts[i], pDeltaTime);

	// Solve collisions
	for (u32 j = 0; j < PHYSICS_ITERATIONS; ++j)
		for (u32 i = 0; i < contacts.size(); ++i)
			ApplyImpulse(&contacts[i]);

	// Integrate velocities
	for (u32 i = 0; i < pScene->bodies.size(); ++i)
		IntegrateVelocity(pScene->bodies[i], pDeltaTime);

	// Correct positions
	for (u32 i = 0; i < contacts.size(); ++i)
		PositionalCorrection(&contacts[i]);

	// Clear all forces
	for (u32 i = 0; i < pScene->bodies.size(); ++i)
	{
		RigidBody *b = pScene->bodies[i];
		b->force = V2(0, 0);
		b->torque = 0;
	}
}

static RigidBody* AddRigidBody(MemoryStack* pStack, PhysicsScene* pScene, RigidBodyCreationOptions* pOptions)
{
	RigidBody* body = PushZerodStruct(pStack, RigidBody);
	body->entity = pOptions->entity;
	body->orient = 0;
	body->static_friction = pOptions->material.static_friction;
	body->dynamic_friction = pOptions->material.dynamic_friction;
	body->restitution = pOptions->material.restitution;


	Shape* shape = nullptr;
	switch (pOptions->type)
	{
	case SHAPE_Circle:
		shape = CreateCircle(pStack, body, pOptions->radius, pOptions->density);
		break;
	case SHAPE_Poly:
		shape = CreatePolygon(pStack, body, pOptions->width, pOptions->height, pOptions->offset, pOptions->density);
		break;
	default:
		assert(false);
	}
	shape->type = pOptions->type;
	body->shape = shape;

	pScene->bodies.push_back(body);
	return body;
}

static void RemoveRigidBody(Entity* pBody, PhysicsScene* pScene)
{
	for (auto iter = pScene->bodies.begin(); iter != pScene->bodies.end(); ++iter)
	{
		if ((*iter)->entity == pBody)
		{
			pScene->bodies.erase(iter);
			break;
		}
	}
}

static void RenderRigidBodies(RenderState* pState, PhysicsScene* pScene)
{
#if DEBUG_DRAW_RIGIDBODIES
	for (u32 i = 0; i < pScene->bodies.size(); ++i)
	{
		SetZLayer(pState, Z_LAYER_Ui);
		RigidBody *A = pScene->bodies[i];
		PushSizedQuad(pState, A->entity->position + A->shape->offset, A->shape->size, V4(1, 0, 0, 0.75F));
	}
#endif
}