const float CAMERA_BOUNDS = 0.2F;

static void UpdateCamera(GameState* pState, Entity* pCharacter)
{
	const float min_bounds_x = pState->form->width * CAMERA_BOUNDS;
	const float max_bounds_x = pState->form->width * (1 - CAMERA_BOUNDS);
	const float min_bounds_y = pState->form->height * CAMERA_BOUNDS;
	const float max_bounds_y = pState->form->height * (1 - CAMERA_BOUNDS);

	Camera* camera = &pState->camera;
	if (pCharacter->position.X < camera->position.X + min_bounds_x)
	{
		camera->position.X = pCharacter->position.X - min_bounds_x;
	}
	else if (pCharacter->position.X > camera->position.X + max_bounds_x)
	{
		camera->position.X = pCharacter->position.X - max_bounds_x;
	}

	if (pCharacter->position.Y < camera->position.Y + min_bounds_y)
	{
		camera->position.Y = pCharacter->position.Y - min_bounds_y;
	}
	else if (pCharacter->position.Y > camera->position.Y + max_bounds_y)
	{
		camera->position.Y = pCharacter->position.Y - max_bounds_y;
	}
	
	
	camera->position.X = clamp(0.0F, camera->position.X, (float)pState->map->width - pState->form->width);
	camera->position.Y = clamp(0.0F, camera->position.Y, (float)pState->map->height - pState->form->height);
}

static inline v2 WorldSpaceToCameraSpace(Camera* pCamera, v2 pPosition)
{
	return pPosition - pCamera->position;
}

static inline mat4 GetOrthoMatrix(Camera* pCamera)
{
	return HMM_Orthographic(pCamera->position.X, pCamera->position.X + g_state.form->width, pCamera->position.Y + g_state.form->height, pCamera->position.Y, -Z_LAYER_MAX * Z_INDEX_DEPTH, Z_LAYER_MAX * Z_INDEX_DEPTH);
}