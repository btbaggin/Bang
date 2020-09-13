// Half width and half height
static void SetBox(Shape* pShape, float hw, float hh, v2 pOffset)
{
	//pShape->vertex_count = 4;
	//pShape->vertices[0] = V2(-hw, -hh);
	//pShape->vertices[1] = V2(hw, -hh);
	//pShape->vertices[2] = V2(hw, hh);
	//pShape->vertices[3] = V2(-hw, hh);
	//pShape->normals[0] = V2(0.0f, -1.0f);
	//pShape->normals[1] = V2(1.0f, 0.0f);
	//pShape->normals[2] = V2(0.0f, 1.0f);
	//pShape->normals[3] = V2(-1.0f, 0.0f);

	pShape->vertex_count = 4;
	pShape->vertices[0] = pOffset;
	pShape->vertices[1] = V2(pOffset.X + hw, pOffset.Y);
	pShape->vertices[2] = V2(pOffset.X + hw, pOffset.Y + hh);
	pShape->vertices[3] = V2(pOffset.X, pOffset.Y + hh);
	pShape->normals[0] = V2(0.0f, -1.0f);
	pShape->normals[1] = V2(1.0f, 0.0f);
	pShape->normals[2] = V2(0.0f, 1.0f);
	pShape->normals[3] = V2(-1.0f, 0.0f);
}

void Set(Shape* pShape, v2* vertices, u32 count)
{
	// No hulls with less than 3 vertices (ensure actual polygon)
	assert(count > 2 && count <= MAX_POLY_VERTEX_COUNT);
	assert(pShape->type == SHAPE_Poly);
	count = min((s32)count, MAX_POLY_VERTEX_COUNT);

	// Find the right most point on the hull
	s32 rightMost = 0;
	float highestXCoord = vertices[0].X;
	for (u32 i = 1; i < count; ++i)
	{
		float x = vertices[i].X;
		if (x > highestXCoord)
		{
			highestXCoord = x;
			rightMost = i;
		}

		// If matching x then take farthest negative y
		else if (x == highestXCoord)
			if (vertices[i].Y < vertices[rightMost].Y)
				rightMost = i;
	}

	s32 hull[MAX_POLY_VERTEX_COUNT];
	s32 outCount = 0;
	s32 indexHull = rightMost;

	for (;;)
	{
		hull[outCount] = indexHull;

		// Search for next index that wraps around the hull
		// by computing cross products to find the most counter-clockwise
		// vertex in the set, given the previos hull index
		s32 nextHullIndex = 0;
		for (s32 i = 1; i < (s32)count; ++i)
		{
			// Skip if same coordinate as we need three unique
			// points in the set to perform a cross product
			if (nextHullIndex == indexHull)
			{
				nextHullIndex = i;
				continue;
			}

			// Cross every set of three unique vertices
			// Record each counter clockwise third vertex and add
			// to the output hull
			// See : http://www.oocities.org/pcgpe/math2d.html
			v2 e1 = vertices[nextHullIndex] - vertices[hull[outCount]];
			v2 e2 = vertices[i] - vertices[hull[outCount]];
			float c = Cross(e1, e2);
			if (c < 0.0f)
				nextHullIndex = i;

			// Cross product is zero then e vectors are on same line
			// therefor want to record vertex farthest along that line
			if (c == 0.0f && HMM_LengthSquared(e2) > HMM_LengthSquared(e1))
				nextHullIndex = i;
		}

		++outCount;
		indexHull = nextHullIndex;

		// Conclude algorithm upon wrap-around
		if (nextHullIndex == rightMost)
		{
			pShape->vertex_count = outCount;
			break;
		}
	}

	// Copy vertices into shape's vertices
	for (u32 i = 0; i < pShape->vertex_count; ++i)
		pShape->vertices[i] = vertices[hull[i]];

	// Compute face normals
	for (u32 i1 = 0; i1 < pShape->vertex_count; ++i1)
	{
		u32 i2 = i1 + 1 < pShape->vertex_count ? i1 + 1 : 0;
		v2 face = pShape->vertices[i2] - pShape->vertices[i1];

		// Ensure no zero-length edges, because that's bad
		assert(HMM_LengthSquared(face) > EPSILON * EPSILON);

		// Calculate normal with 2D cross product between vector and scalar
		pShape->normals[i1] = V2(face.Y, -face.X);
		pShape->normals[i1] = HMM_Normalize(pShape->normals[i1]);
	}
}

// The extreme point along a direction within a polygon
static v2 GetSupport(Shape* pShape, v2 dir)
{
	float bestProjection = -FLT_MAX;
	v2 bestVertex = {};

	for (u32 i = 0; i < pShape->vertex_count; ++i)
	{
		v2 v = pShape->vertices[i];
		float projection = HMM_Dot(v, dir);

		if (projection > bestProjection)
		{
			bestVertex = v;
			bestProjection = projection;
		}
	}

	return bestVertex;
}

static Shape* CreateCircle(MemoryStack* pStack, RigidBody* pBody, float pRadius, float pDensity)
{
	Shape* s = PushStruct(pStack, Shape);
	s->body = pBody;
	s->body->mass = PI * pRadius * pRadius * pDensity;
	s->body->inverse_mass = (s->body->mass) ? 1.0f / s->body->mass : 0.0f;
	s->body->inertia = s->body->mass * pRadius * pRadius;
	s->body->inverse_inertia = (s->body->inertia) ? 1.0f / s->body->inertia : 0.0f;

	return s;
}

static Shape* CreatePolygon(MemoryStack* pStack, RigidBody* pBody, float pWidth, float pHeight, v2 pOffset, float pDensity)
{
	Shape* s = PushStruct(pStack, Shape);
	s->body = pBody;
	//SetBox(s, pWidth / 2.0F, pHeight / 2.0F);
	SetBox(s, pWidth, pHeight, pOffset);

	//Compute mass
	// Calculate centroid and moment of interia
	//v2 c = {};
	float area = 0.0f;
	float I = 0.0f;
	const float k_inv3 = 1.0f / 3.0f;

	for (u32 i1 = 0; i1 < s->vertex_count; ++i1)
	{
		// Triangle vertices, third vertex implied as (0, 0)
		v2 p1(s->vertices[i1]);
		u32 i2 = i1 + 1 < s->vertex_count ? i1 + 1 : 0;
		v2 p2 = s->vertices[i2];

		float D = Cross(p1, p2);
		float triangleArea = 0.5f * D;

		area += triangleArea;

		// Use area to weight the centroid average, not just vertex position
		//c += triangleArea * k_inv3 * (p1 + p2);

		float intx2 = p1.X * p1.X + p2.X * p1.X + p2.X * p2.X;
		float inty2 = p1.Y * p1.Y + p2.Y * p1.Y + p2.Y * p2.Y;
		I += (0.25f * k_inv3 * D) * (intx2 + inty2);
	}

	//c *= 1.0f / area;

	//// Translate vertices to centroid (make the centroid (0, 0)
	//// for the polygon in model space)
	//// Not really necessary, but I like doing this anyway
	//for (u32 i = 0; i < s->vertex_count; ++i)
	//	s->vertices[i] -= c;

	s->body->mass = pDensity * area;
	s->body->inverse_mass = (s->body->mass) ? 1.0f / s->body->mass : 0.0f;
	s->body->inertia = I * pDensity;
	s->body->inverse_inertia = s->body->inertia ? 1.0f / s->body->inertia : 0.0f;
	s->u = { 1, 0, 0, 1 };
#if DEBUG_DRAW_RIGIDBODIES
	s->size = V2(pWidth, pHeight);
	s->offset = pOffset;
#endif

	return s;
}

static void SetOrient(Shape* pShape, float radians)
{
	if (pShape->type == SHAPE_Poly)
	{
		Rotate(&pShape->u, radians);
	}
}