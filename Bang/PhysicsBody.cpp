struct Shape;

static void ApplyForce(RigidBody* pBody, v2 pForce)
{
	pBody->force += pForce;
}

static void ApplyImpulse(RigidBody* pBody, v2 pImpulse, v2 pContact)
{
	pBody->velocity += pBody->inverse_mass * pImpulse;
	pBody->angular_velocity += pBody->inverse_inertia * Cross(pContact, pImpulse);
}

static void SetStatic(RigidBody* pBody)
{
	pBody->inertia = 0.0f;
	pBody->inverse_inertia = 0.0f;
	pBody->mass = 0.0f;
	pBody->inverse_mass = 0.0f;
}

static void SetOrientation(RigidBody* pBody, float pRadians)
{
	pBody->orient = pRadians;
	SetOrient(pBody->shape, pRadians);
}