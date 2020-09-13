const float CAMERA_BOUNDS = 0.2F;

static void UpdateCamera(GameState* pState, Entity* pCharacter)
{
	Camera* camera = &pState->camera;
	float zoom = GetSetting(&pState->config, "camera_zoom")->f;
	camera->viewport = g_state.game_started ? g_state.map->tile_size * zoom : V2(g_state.form->width, g_state.form->height);
	const float min_bounds_x = camera->viewport.Width * CAMERA_BOUNDS;
	const float max_bounds_x = camera->viewport.Width * (1 - CAMERA_BOUNDS);
	const float min_bounds_y = camera->viewport.Height * CAMERA_BOUNDS;
	const float max_bounds_y = camera->viewport.Height * (1 - CAMERA_BOUNDS);

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
		
	camera->position.X = clamp(0.0F, camera->position.X, (float)pState->map->width - camera->viewport.Width);
	camera->position.Y = clamp(0.0F, camera->position.Y, (float)pState->map->height - camera->viewport.Height);
	camera->matrix = HMM_Orthographic(camera->position.X,
									  camera->position.X + camera->viewport.Width,
									  camera->position.Y + camera->viewport.Height,
									  camera->position.Y,
									  -Z_LAYER_MAX * Z_INDEX_DEPTH, Z_LAYER_MAX * Z_INDEX_DEPTH);
}

static inline v2 ToWorldSpace(Camera* pCamera, v2 pPosition)
{
	v2 aspect = V2(pCamera->viewport.Width / g_state.form->width, pCamera->viewport.Height / g_state.form->height);
	return pCamera->position + pPosition * aspect;
}