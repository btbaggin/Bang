struct Manifold
{
	RigidBody *A;
	RigidBody *B;

	float penetration;     // Depth of penetration from collision
	v2 normal;          // From A to B
	v2 contacts[2];     // Points of contact during collision
	u32 contact_count; // Number of contacts that occured during collision
	float e;               // Mixed restitution
	float df;              // Mixed dynamic friction
	float sf;              // Mixed static friction
};


static void InitializeManifold(Manifold* m, float pDeltaTime)
{
	// Calculate average restitution
	m->e = min(m->A->restitution, m->B->restitution);

	// Calculate static and dynamic friction
	m->sf = std::sqrt(m->A->static_friction * m->A->static_friction);
	m->df = std::sqrt(m->A->dynamic_friction * m->A->dynamic_friction);

	for (u32 i = 0; i < m->contact_count; ++i)
	{
		// Calculate radii from COM to contact
		v2 ra = m->contacts[i] - m->A->entity->position;
		v2 rb = m->contacts[i] - m->B->entity->position;

		v2 rv = m->B->velocity + Cross(m->B->angular_velocity, rb) -
			m->A->velocity - Cross(m->A->angular_velocity, ra);


		// Determine if we should perform a resting collision or not
		// The idea is if the only thing moving this object is gravity,
		// then the collision should be performed without any restitution
		if (HMM_LengthSquared(rv) < HMM_LengthSquared(pDeltaTime * GRAVITY) + EPSILON)
			m->e = 0.0f;
	}
}

void ApplyImpulse(Manifold* m)
{
	// Early out and positional correct if both objects have infinite mass
	if (Equal(m->A->inverse_mass + m->B->inverse_mass, 0))
	{
		m->A->velocity = {};
		m->B->velocity = {};
		return;
	}

	for (u32 i = 0; i < m->contact_count; ++i)
	{
		// Calculate radii from COM to contact
		v2 ra = m->contacts[i] - m->A->entity->position;
		v2 rb = m->contacts[i] - m->B->entity->position;

		// Relative velocity
		v2 rv = m->B->velocity + Cross(m->B->angular_velocity, rb) -
			m->A->velocity - Cross(m->A->angular_velocity, ra);

		// Relative velocity along the normal
		float contactVel = HMM_Dot(rv, m->normal);

		// Do not resolve if velocities are separating
		if (contactVel > 0)
			return;

		float raCrossN = Cross(ra, m->normal);
		float rbCrossN = Cross(rb, m->normal);
		float invMassSum = m->A->inverse_mass + m->B->inverse_mass + (raCrossN * raCrossN) * m->A->inverse_inertia + (rbCrossN * rbCrossN) * m->B->inverse_inertia;

		// Calculate impulse scalar
		float j = -(1.0f + m->e) * contactVel;
		j /= invMassSum;
		j /= (float)m->contact_count;

		// Apply impulse
		v2 impulse = m->normal * j;
		ApplyImpulse(m->A, impulse * -1, ra);
		ApplyImpulse(m->B, impulse, rb);

		// Friction impulse
		rv = m->B->velocity + Cross(m->B->angular_velocity, rb) -
			 m->A->velocity - Cross(m->A->angular_velocity, ra);

		v2 t = rv - (m->normal * HMM_Dot(rv, m->normal));
		t = HMM_Normalize(t);

		// j tangent magnitude
		float jt = -HMM_Dot(rv, t);
		jt /= invMassSum;
		jt /= (float)m->contact_count;

		// Don't apply tiny friction impulses
		if (Equal(jt, 0.0f))
			return;

		// Coulumb's law
		v2 tangentImpulse;
		if (std::abs(jt) < j * m->sf)
			tangentImpulse = t * jt;
		else
			tangentImpulse = t * -j * m->df;

		// Apply friction impulse
		ApplyImpulse(m->A, tangentImpulse * -1, ra);
		ApplyImpulse(m->B, tangentImpulse, rb);
	}
}

void static PositionalCorrection(Manifold* m)
{
	const float k_slop = 0.05f; // Penetration allowance
	const float percent = 0.4f; // Penetration percentage to correct
	v2 correction = (max(m->penetration - k_slop, 0.0f) / (m->A->inverse_mass + m->B->inverse_mass)) * m->normal * percent;
	m->A->entity->position -= correction * m->A->inverse_mass;
	m->B->entity->position += correction * m->B->inverse_mass;
}