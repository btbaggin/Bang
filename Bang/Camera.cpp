const float CAMERA_BOUNDS = 0.2F;

static void UpdateCamera(GameState* pState, Entity* pCharacter)
{
	float zoom = GetSetting(&pState->config, "camera_zoom")->f;
	v2 viewport = g_state.game_started ? g_state.map->tile_size * zoom : V2(g_state.form->width, g_state.form->height);
	const float min_bounds_x = viewport.Width * CAMERA_BOUNDS;
	const float max_bounds_x = viewport.Width * (1 - CAMERA_BOUNDS);
	const float min_bounds_y = viewport.Height * CAMERA_BOUNDS;
	const float max_bounds_y = viewport.Height * (1 - CAMERA_BOUNDS);

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
		
	camera->position.X = clamp(0.0F, camera->position.X, (float)pState->map->width - viewport.Width);
	camera->position.Y = clamp(0.0F, camera->position.Y, (float)pState->map->height - viewport.Height);
}

static inline v2 WorldSpaceToCameraSpace(Camera* pCamera, v2 pPosition)
{
	return pPosition - pCamera->position;
}

static inline mat4 GetOrthoMatrix(Camera* pCamera)
{
	float zoom = GetSetting(&g_state.config, "camera_zoom")->f;
	v2 viewport = g_state.game_started ? g_state.map->tile_size * zoom : V2(g_state.form->width, g_state.form->height);
	return HMM_Orthographic(pCamera->position.X, pCamera->position.X + viewport.Width, pCamera->position.Y + viewport.Height, pCamera->position.Y, -Z_LAYER_MAX * Z_INDEX_DEPTH, Z_LAYER_MAX * Z_INDEX_DEPTH);
}