static ParticleSystem SpawnParticleSystem(u32 pCount, u32 pPerSecond, BITMAPS pBitmap, ParticleCreationOptions* pOptions)
{
	ParticleSystem sys;
	sys.particle_count = pCount;
	sys.last_particle_used = pCount;
	sys.particles = PushArray(g_state.world_arena, Particle, pCount);
	sys.time_per_particle = 1.0F / pPerSecond;
	sys.particle_time = 0;
	sys.creation = pOptions;
	sys.texture = pBitmap;

	static const GLfloat g_vertex_buffer_data[] = {
	 -0.5f, -0.5f, 10.0f,
	 -0.5f,  0.5f, 10.0f,
	  0.5f, -0.5f, 10.0f,
	  0.5f,  0.5f, 10.0f,
	};
	glGenVertexArrays(1, &sys.VAO);
	glBindVertexArray(sys.VAO);

	glGenBuffers(1, &sys.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, sys.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &sys.PBO);
	glBindBuffer(GL_ARRAY_BUFFER, sys.PBO);
	glBufferData(GL_ARRAY_BUFFER, pCount * 4 * sizeof(float), NULL, GL_STREAM_DRAW);

	glGenBuffers(1, &sys.CBO);
	glBindBuffer(GL_ARRAY_BUFFER, sys.CBO);
	glBufferData(GL_ARRAY_BUFFER, pCount * 4 * sizeof(u8), NULL, GL_STREAM_DRAW);

	//vertices
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//positions
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//colors
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

	glVertexAttribDivisor(0, 0); // particles vertices : always reuse
	glVertexAttribDivisor(1, 1); // positions : one per quad
	glVertexAttribDivisor(2, 1); // color : one per quad

	glBindVertexArray(0);

	for (u32 i = 0; i < pCount; i++)
	{
		Particle* p = sys.particles + i;
		p->life = 0;
	}

	return sys;
}

static Particle* FindUnusedParticle(ParticleSystem* pSystem)
{
	for (u32 i = pSystem->last_particle_used; i < pSystem->particle_count; i++)
	{
		Particle* p = pSystem->particles + i;
		if (p->life <= 0)
		{
			pSystem->last_particle_used = i;
			return p;
		}
	}

	for (u32 i = 0; i < pSystem->last_particle_used; i++)
	{
		Particle* p = pSystem->particles + i;
		if (p->life <= 0)
		{
			pSystem->last_particle_used = i;
			return p;
		}
	}

	pSystem->last_particle_used = 0;
	return pSystem->particles;
}

static void InitializeParticle(Particle* pParticle, v2 pPosition, ParticleCreationOptions* pOptions)
{
	if (pOptions->spawn_radius > 0)
	{
		float a = Random();
		float b = Random();
		if (b < a)
		{
			float temp = b;
			b = a;
			a = temp;
		}
		v2 offset = V2(b * pOptions->spawn_radius * cos(2 * (float)HMM_PI * a / b), b * pOptions->spawn_radius * sin(2 * (float)HMM_PI * a / b));
		pParticle->position = pPosition + offset;
	}
	else
	{
		pParticle->position = pPosition;
	}
	pParticle->r = pOptions->r;
	pParticle->g = pOptions->g;
	pParticle->b = pOptions->b;
	pParticle->a = pOptions->a;
	pParticle->size = Random(pOptions->size);
	pParticle->life = Random(pOptions->life);

	v2 random_direction = {};
	if (pOptions->spread != 0)
	{
		float phi = Random(0.0F, 2 * (float)HMM_PI);
		float cosTheta = Random(-1.0F, 1.0F);
		float u = Random();
		float theta = acos(cosTheta);
		float r = pOptions->spread * stbtt__cuberoot(u);

		random_direction = V2(r * sin(theta) * cos(phi), r * sin(theta) * sin(phi));
	}
	else
	{
		random_direction = {};
	}

	pParticle->velocity = (pOptions->direction + random_direction) * Random(pOptions->speed);
}

static void UpdateParticleSystem(ParticleSystem* pSystem, float pDeltaTime, ParticleUpdate* pUpdate, bool pAllowNewParticles = true)
{
	if (pAllowNewParticles)
	{
		pSystem->particle_time += pDeltaTime;
		while (pSystem->particle_time >= pSystem->time_per_particle)
		{
			Particle* p = FindUnusedParticle(pSystem);
			InitializeParticle(p, pSystem->position, pSystem->creation);
			pSystem->particle_time -= pSystem->time_per_particle;
		}
	}

	float* positions = PushArray(g_transstate.trans_arena, float, pSystem->particle_count * 4);
	u8* colors = PushArray(g_transstate.trans_arena, u8, pSystem->particle_count * 4);
	u32 count = 0;
	for (u32 i = 0; i < pSystem->particle_count; i++)
	{
		Particle* p = pSystem->particles + i;
		p->life -= pDeltaTime;
		if (p->life > 0)
		{
			p->position += p->velocity * pDeltaTime;
			if(pUpdate) pUpdate(p, pDeltaTime);

			p->camera_distance = HMM_LengthSquared(p->position - g_state.camera.position);

			*(positions + (4 * count + 0)) = p->position.X;
			*(positions + (4 * count + 1)) = p->position.Y;
			*(positions + (4 * count + 2)) = g_transstate.render_state->z_index;
			*(positions + (4 * count + 3)) = p->size;

			*(colors + (4 * count + 0)) = p->r;
			*(colors + (4 * count + 1)) = p->g;
			*(colors + (4 * count + 2)) = p->b;
			*(colors + (4 * count + 3)) = p->a;
			count++;
		}
		else
		{
			p->camera_distance = -1.0F;
		}
	}
	std::sort(pSystem->particles, pSystem->particles + pSystem->particle_count);
	pSystem->alive_particle_count = count;

	glBindVertexArray(pSystem->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, pSystem->PBO);
	glBufferData(GL_ARRAY_BUFFER, pSystem->particle_count * 4 * sizeof(float), NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, count * 4 * sizeof(float), positions);

	glBindBuffer(GL_ARRAY_BUFFER, pSystem->CBO);
	glBufferData(GL_ARRAY_BUFFER, pSystem->particle_count * 4 * sizeof(u8), NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, count * 4 * sizeof(u8), colors);
}

void DestroyParticleSystem(ParticleSystem* pSystem)
{
	//TODO push old creation options and particle arrays
	glDeleteVertexArrays(1, &pSystem->VAO);
	glDeleteBuffers(1, &pSystem->PBO);
	glDeleteBuffers(1, &pSystem->CBO);
}

