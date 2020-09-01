static GLuint LoadShader(const char* pShader, GLenum pType)
{
	GLuint shader_id = glCreateShader(pType);

	glShaderSource(shader_id, 1, &pShader, 0);
	glCompileShader(shader_id);

	GLint result = GL_FALSE;
	int info_log_length;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		char* error = new char[info_log_length + 1];
		glGetShaderInfoLog(shader_id, info_log_length, 0, error);
		DisplayErrorMessage(error, ERROR_TYPE_Warning);
	}

	return shader_id;
}

static GLProgram CreateProgram(GLuint pVertex, GLuint pFragment)
{
	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, pVertex);
	glAttachShader(program_id, pFragment);
	glLinkProgram(program_id);

	GLint result = GL_FALSE;
	int info_log_length;
	glGetProgramiv(program_id, GL_LINK_STATUS, &result);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		char* error = new char[info_log_length + 1];
		glGetProgramInfoLog(program_id, info_log_length, 0, error);
		DisplayErrorMessage(error, ERROR_TYPE_Warning);
	}

	glDetachShader(program_id, pVertex);
	glDetachShader(program_id, pFragment);

	glDeleteShader(pVertex);
	glDeleteShader(pFragment);

	GLProgram prog;
	prog.id = program_id;
	prog.texture = glGetUniformLocation(program_id, "mainTex");
	prog.mvp = glGetUniformLocation(program_id, "MVP");
	prog.font = glGetUniformLocation(program_id, "font");
	return prog;
}

static void InitializeRenderer(RenderState* pState)
{
	void* render = malloc(Megabytes(6));
	pState->arena = CreateMemoryStack(render, Megabytes(6));
	pState->index_count = 0;
	pState->vertex_count = 0;
	pState->vertices = CreateSubStack(pState->arena, Megabytes(4));
	pState->indices = CreateSubStack(pState->arena, Megabytes(1) - sizeof(MemoryStack));

	glGenVertexArrays(1, &pState->VAO);
	glBindVertexArray(pState->VAO);

	glGenBuffers(1, &pState->VBO);
	glGenBuffers(1, &pState->EBO);
	glBindBuffer(GL_ARRAY_BUFFER, pState->VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pState->EBO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

	GLuint v_shader = LoadShader(VERTEX_SHADER, GL_VERTEX_SHADER);
	GLuint f_shader = LoadShader(FRAGMENT_SHADER, GL_FRAGMENT_SHADER);
	pState->program = CreateProgram(v_shader, f_shader);
	glUseProgram(pState->program.id);

	v_shader = LoadShader(PARTICLE_VERTEX_SHADER, GL_VERTEX_SHADER);
	f_shader = LoadShader(PARTICLE_FRAGMENT_SHADER, GL_FRAGMENT_SHADER);
	pState->particle_program = CreateProgram(v_shader, f_shader);

	glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
}

static void BeginRenderPass(v2 pFormSize, RenderState* pState)
{
	pState->vertex_count = 0;
	pState->index_count = 0;
	pState->entry_count = 0;

	pState->vertices->count = 0;
	pState->indices->count = 0;

	mat4 m = GetOrthoMatrix(&g_state.camera);
	glUniformMatrix4fv(pState->program.mvp, 1, GL_FALSE, &m.Elements[0][0]);
	pState->memory = BeginTemporaryMemory(pState->arena);
}

static void RenderRenderEntry(RenderState* pState, RenderEntry* pLastEntry, RenderEntry* pEntry)
{
	char* address = (char*)(pEntry + 1);
	switch (pEntry->type)
	{
	case RENDER_GROUP_ENTRY_TYPE_Matrix:
	{
		Renderable_Matrix* m = (Renderable_Matrix*)address;
		glUniformMatrix4fv(pState->program.mvp, 1, GL_FALSE, &m->matrix.Elements[0][0]);
	}
	break;

	case RENDER_GROUP_ENTRY_TYPE_Quad:
	{
		Renderable_Quad* vertices = (Renderable_Quad*)address;

		glActiveTexture(GL_TEXTURE0);
		glUniform1i(pState->program.texture, 0);
		glBindTexture(GL_TEXTURE_2D, vertices->texture);

		glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)(vertices->first_index * sizeof(u16)), vertices->first_vertex);
	}
	break;

	case RENDER_GROUP_ENTRY_TYPE_Text:
	{
		glUniform1i(pState->program.font, 1);

		Renderable_Text* text = (Renderable_Text*)address;

		glBindTexture(GL_TEXTURE_2D, text->texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(pState->program.texture, 0);

		if (HasFlag(text->flags, RENDER_TEXT_FLAG_Clip))
		{
			glEnable(GL_SCISSOR_TEST);
			glScissor((u32)text->clip_min.X, (u32)(g_state.form->height - (text->clip_min.Y + text->clip_max.Y)), (int)text->clip_max.X, (int)text->clip_max.Y);
		}

		glDrawElementsBaseVertex(GL_TRIANGLES, text->index_count, GL_UNSIGNED_SHORT, (void*)(text->first_index * sizeof(u16)), text->first_vertex);
		glDisable(GL_SCISSOR_TEST);

		glUniform1i(pState->program.font, 0);
	}

	case RENDER_GROUP_ENTRY_TYPE_Line:
	{
		Renderable_Line* line = (Renderable_Line*)address;

		glActiveTexture(GL_TEXTURE0);
		glUniform1i(pState->program.texture, 0);
		glBindTexture(GL_TEXTURE_2D, g_transstate.assets->blank_texture);

		glLineWidth(line->size);
		glDrawArrays(GL_LINES, line->first_vertex, 2);
	}
	break;

	case RENDER_GROUP_ENTRY_TYPE_ParticleSystem:
	{
		Renderable_ParticleSystem* sys = (Renderable_ParticleSystem*)address;
		glUseProgram(pState->particle_program.id);

		glBindVertexArray(sys->VAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, sys->texture);
		glUniform1i(pState->particle_program.texture, 0);

		mat4 m = GetOrthoMatrix(&g_state.camera);
		glUniformMatrix4fv(pState->particle_program.mvp, 1, GL_FALSE, &m.Elements[0][0]);

		//vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, sys->VBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//positions
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, sys->PBO);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//colors
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, sys->CBO);
		glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

		glVertexAttribDivisor(0, 0); // particles vertices : always reuse
		glVertexAttribDivisor(1, 1); // positions : one per quad
		glVertexAttribDivisor(2, 1); // color : one per quad

		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, sys->particle_count);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glBindVertexArray(0);

		glUseProgram(pState->program.id);
		glBindVertexArray(pState->VAO);
		glBindBuffer(GL_ARRAY_BUFFER, pState->VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pState->EBO);
		glBufferData(GL_ARRAY_BUFFER, pState->vertex_count * sizeof(Vertex), MemoryAddress(pState->vertices), GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pState->index_count * sizeof(u16), MemoryAddress(pState->indices), GL_STATIC_DRAW);
	}
	break;
	}
}

static void EndRenderPass(v2 pFormSize, RenderState* pState)
{
	glBindVertexArray(pState->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, pState->VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pState->EBO);
	glBufferData(GL_ARRAY_BUFFER, pState->vertex_count * sizeof(Vertex), MemoryAddress(pState->vertices), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, pState->index_count * sizeof(u16), MemoryAddress(pState->indices), GL_STATIC_DRAW);

	glViewport(0, 0, (int)pFormSize.Width, (int)pFormSize.Height);

	char* address = (char*)MemoryAddress(pState->arena, pState->memory.count);
	char* last_address = address;
	for (u32 i = 0; i < pState->entry_count; i++)
	{
		RenderEntry* header = (RenderEntry*)address;
		//address += sizeof(RenderEntry);

		RenderRenderEntry(pState, (RenderEntry*)last_address, header);
		
		last_address = address;
		address += sizeof(RenderEntry) + header->size;
	}

	glBindVertexArray(0);
	EndTemporaryMemory(pState->memory);
}

static u32 BeginRenderToTexture(Bitmap* pBitmap, RenderState* pState)
{
	GLuint buffer = 0;
	glGenFramebuffers(1, &buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer);

	glGenTextures(1, &pBitmap->texture);
	glBindTexture(GL_TEXTURE_2D, pBitmap->texture);

	//Reserve memory
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pBitmap->width, pBitmap->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pBitmap->texture, 0);

	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		DisplayErrorMessage("Unable to render to framebuffer", ERROR_TYPE_Error);
		return -1;
	}

	BeginRenderPass(V2((float)pBitmap->width, (float)pBitmap->height), pState);
	mat4 m = HMM_Orthographic(0.0F, (float)pBitmap->width, (float)pBitmap->height, 0.0F, -Z_LAYER_MAX * Z_INDEX_DEPTH, 10.0F);
	glUniformMatrix4fv(pState->program.mvp, 1, GL_FALSE, &m.Elements[0][0]);

	return buffer;
}

static void EndRenderToTexture(Bitmap* pBitmap, RenderState* pState, u32 pFramebuffer)
{
	// Render to our framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, pFramebuffer);
	EndRenderPass(V2((float)pBitmap->width, (float)pBitmap->height), pState);

	//Delete resources
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &pFramebuffer);
}

static inline
void* _PushRenderGroupEntry(RenderState* pState, u8 pSize, RENDER_GROUP_ENTRY_TYPE pType)
{
	RenderEntry* header = PushStruct(pState->arena, RenderEntry);
	header->type = pType;
	header->size = pSize;

	void* result = PushSize(pState->arena, pSize);
	pState->entry_count++;

	return result;
}
#define PushRenderGroupEntry(pState, pStruct, pType) (pStruct*)_PushRenderGroupEntry(pState, (u8)sizeof(pStruct), pType)

GlyphQuad GetGlyph(RenderState* pState, FontInfo* pFont, u32 pChar, v4 pColor, float* pX, float* pY)
{
	stbtt_aligned_quad quad;

	int index = pChar - ' ';
	if (index < 0 || index > '~' - ' ') index = 0;

	stbtt_GetPackedQuad(pFont->charInfo, pFont->atlasWidth, pFont->atlasHeight, index, pX, pY, &quad, 1);

	float x_min = quad.x0;
	float x_max = quad.x1;
	float y_min = quad.y1;
	float y_max = quad.y0;

	GlyphQuad info = {};
	info.vertices[0] = { { x_min, y_min, pState->z_index }, pColor, { quad.s0, quad.t1 } };
	info.vertices[1] = { { x_min, y_max, pState->z_index }, pColor, { quad.s0, quad.t0 } };
	info.vertices[2] = { { x_max, y_max, pState->z_index }, pColor, { quad.s1, quad.t0 } };
	info.vertices[3] = { { x_max, y_min, pState->z_index }, pColor, { quad.s1, quad.t1 } };

	return info;
}

static void _PushText(RenderState* pState, FONTS pFont, const char* pText, v2 pPosition, v4 pColor, v2 pClipMin, v2 pClipMax)
{
	pState->z_index += 0.5F;
	float x = pPosition.X;
	float y = pPosition.Y;

	FontInfo* font = GetFont(g_transstate.assets, pFont);
	if (font)
	{
		y += font->baseline;

		Renderable_Text* m2 = PushRenderGroupEntry(pState, Renderable_Text, RENDER_GROUP_ENTRY_TYPE_Text);
		m2->first_vertex = pState->vertex_count;
		m2->first_index = pState->index_count;
		m2->index_count = 0;
		m2->texture = font->texture;
		m2->flags = 0;

		if (pClipMin.X > 0) m2->flags |= RENDER_TEXT_FLAG_Clip;
		m2->clip_min = pClipMin;
		m2->clip_max = pClipMax;

		u32 base_v = 0;
		for (u32 i = 0; pText[i] != 0; i++)
		{
			const unsigned char c = pText[i];
			if (c == '\n')
			{
				y += font->size;
				x = pPosition.X;
			}
			GlyphQuad info = GetGlyph(pState, font, c, pColor, &x, &y);

			Vertex* v = PushArray(pState->vertices, Vertex, 4);
			for (u32 i = 0; i < 4; i++)
			{
				v[i] = info.vertices[i];
			}

			u16* indices = PushArray(pState->indices, u16, 6);
			indices[0] = base_v + 0; indices[1] = base_v + 2; indices[2] = base_v + 1;
			indices[3] = base_v + 0; indices[4] = base_v + 3; indices[5] = base_v + 2;

			m2->index_count += 6;
			pState->index_count += 6;
			pState->vertex_count += 4;
			base_v += 4;
		}
	}
}

static void PushText(RenderState* pState, FONTS pFont, const char* pText, v2 pPosition, v4 pColor)
{
	_PushText(pState, pFont, pText, pPosition, pColor, V2(0), V2(0));
}

static void PushClippedText(RenderState* pState, FONTS pFont, const char* pText, v2 pPosition, v4 pColor, v2 pClipMin, v2 pClipMax)
{
	_PushText(pState, pFont, pText, pPosition, pColor, pClipMin, pClipMax);
}

static void PushQuad(RenderState* pState, v2 pMin, v2 pMax, v4 pColor, Bitmap* pTexture)
{
	pState->z_index += 0.5F;
	v2 uv_min = pTexture ? pTexture->uv_min : V2(0);
	v2 uv_max = pTexture ? pTexture->uv_max : V2(1);

	Vertex* vertices = PushArray(pState->vertices, Vertex, 4);
	vertices[0] = { { pMin.X, pMin.Y, pState->z_index }, pColor, {uv_min.U, uv_min.V} };
	vertices[1] = { { pMin.X, pMax.Y, pState->z_index }, pColor, {uv_min.U, uv_max.V} };
	vertices[2] = { { pMax.X, pMax.Y, pState->z_index }, pColor, {uv_max.U, uv_max.V} };
	vertices[3] = { { pMax.X, pMin.Y, pState->z_index }, pColor, {uv_max.U, uv_min.V} };

	u16* indices = PushArray(pState->indices, u16, 6);
	indices[0] = 0; indices[1] = 1; indices[2] = 2;
	indices[3] = 2; indices[4] = 3; indices[5] = 0;

	Renderable_Quad* m = PushRenderGroupEntry(pState, Renderable_Quad, RENDER_GROUP_ENTRY_TYPE_Quad);
	m->first_vertex = pState->vertex_count;
	m->first_index = pState->index_count;
	m->texture = pTexture ? pTexture->texture : g_transstate.assets->blank_texture;

	pState->vertex_count += 4;
	pState->index_count += 6;
}
static void PushSizedQuad(RenderState* pState, v2 pMin, v2 pSize, v4 pColor, Bitmap* pTexture)
{
	PushQuad(pState, pMin, pMin + pSize, pColor, pTexture);
}

static void PushQuad(RenderState* pState, v2 pMin, v2 pMax, Bitmap* pTexture)
{
	PushQuad(pState, pMin, pMax, V4(1, 1, 1, 1), pTexture);
}
static void PushSizedQuad(RenderState* pState, v2 pMin, v2 pSize, Bitmap* pTexture)
{
	PushQuad(pState, pMin, pMin + pSize, V4(1, 1, 1, 1), pTexture);
}

static void PushQuad(RenderState* pState, v2 pMin, v2 pMax, v4 pColor)
{
	PushQuad(pState, pMin, pMax, pColor, nullptr);
}
static void PushSizedQuad(RenderState* pState, v2 pMin, v2 pSize, v4 pColor)
{
	PushQuad(pState, pMin, pMin + pSize, pColor, nullptr);
}

static void PushLine(RenderState* pState, v3 pP1, v3 pP2, float pSize, v4 pColor)
{
	pState->z_index += 0.5F;
	Renderable_Line* line = PushRenderGroupEntry(pState, Renderable_Line, RENDER_GROUP_ENTRY_TYPE_Line);
	line->first_vertex = pState->vertex_count;
	line->size = pSize;

	Vertex* v = PushArray(pState->vertices, Vertex, 2);
	*(v + 0) = { pP1, pColor, {0.0F, 0.0F} };
	*(v + 1) = { pP2, pColor, {0.0F, 0.0F} };

	pState->vertex_count += 2;
}

static void PushMatrix(RenderState* pState, mat4 pMatrix)
{
	Renderable_Matrix* m = PushRenderGroupEntry(pState, Renderable_Matrix, RENDER_GROUP_ENTRY_TYPE_Matrix);
	m->matrix = pMatrix;
}

static void PushParticleSystem(RenderState* pState, ParticleSystem* pSystem)
{
	pState->z_index += 0.5F;
	Renderable_ParticleSystem* m = PushRenderGroupEntry(pState, Renderable_ParticleSystem, RENDER_GROUP_ENTRY_TYPE_ParticleSystem);
	m->CBO = pSystem->CBO;
	m->PBO = pSystem->PBO;
	m->VAO = pSystem->VAO;
	m->VBO = pSystem->VBO;
	m->particle_count = pSystem->particle_count;

	Bitmap* bitmap = GetBitmap(g_transstate.assets, pSystem->texture);
	if (bitmap) m->texture = bitmap->texture;
	else m->texture = g_transstate.assets->blank_texture;
}

static inline void SetZLayer(RenderState* pState, Z_LAYERS pLayer)
{
	pState->z_index = pLayer * Z_INDEX_DEPTH;
}
static inline void SetZLayer(RenderState* pState, float pLayer)
{
	pState->z_index = pLayer;
}

static void RenderParalaxBitmap(RenderState* pState, Assets* pAssets, ParalaxBitmap* pBitmap, v2 pSize)
{
	for (u32 i = 0; i < pBitmap->layers; i++)
	{
		SetZLayer(pState, (float)i);
		v2 pos = pBitmap->position[i];
		Bitmap* bitmap = GetBitmap(pAssets, pBitmap->bitmaps[i]);
		if (pos.X > 0)
		{
			//Render an extra bitmap to fill the hole until the original one wraps around
			PushSizedQuad(pState, pos - V2(pSize.X, 0), pSize, bitmap);
		}
		PushSizedQuad(pState, pos, pSize, bitmap);
	}
}

static void RenderAnimation(RenderState* pState, v2 pPosition, v2 pSize, v4 pColor, AnimatedBitmap* pBitmap, bool pFlip = false)
{
	Bitmap* bitmap = GetBitmap(g_transstate.assets, pBitmap->bitmap);
	if (bitmap)
	{
		v2 frame = pBitmap->frame_size;
		frame.X *= pBitmap->current_index;
		frame.Y *= pBitmap->current_animation;

		bitmap->uv_min = V2(frame.X / bitmap->width, frame.Y / bitmap->height);
		bitmap->uv_max = bitmap->uv_min + (pBitmap->frame_size / V2((float)bitmap->width, (float)bitmap->height));
		if (pFlip)
		{
			float u = bitmap->uv_min.U;
			bitmap->uv_min.U = bitmap->uv_max.U;
			bitmap->uv_max.U = u;
		}

		PushSizedQuad(pState, pPosition, pSize, pColor, bitmap);
	}
}

static void DisposeRenderState(RenderState* pState)
{
	glDeleteProgram(pState->program.id);
	glDeleteBuffers(1, &pState->EBO);
	glDeleteBuffers(1, &pState->VBO);
	glDeleteVertexArrays(1, &pState->VAO);
}