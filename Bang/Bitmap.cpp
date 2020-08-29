static void SendTextureToGraphicsCard(void* pData)
{
	if (pData)
	{
		Bitmap* bitmap = (Bitmap*)pData;

		GLenum format;
		if (bitmap->channels == 1) format = GL_RED;
		else if (bitmap->channels == 3) format = GL_RGB;
		else if (bitmap->channels == 4) format = GL_RGBA;

		glGenTextures(1, &bitmap->texture);
		glBindTexture(GL_TEXTURE_2D, bitmap->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, format, bitmap->width, bitmap->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(bitmap->data);
		bitmap->data = nullptr;
	}
}

static Bitmap* GetBitmap(Assets* pAssets, AssetSlot* pSlot)
{
	RequestAsset(pAssets, pSlot, ASSET_TYPE_Bitmap);
	return pSlot->bitmap;
}

static Bitmap* GetBitmap(Assets* pAssets, BITMAPS pAsset)
{
	return GetBitmap(pAssets, pAssets->bitmaps + pAsset);
}

static void UpdateParalaxBitmap(ParalaxBitmap* pBitmap, float pDeltaTime, float pWrapSize)
{
	//Farthest back doesnt update
	for (u32 i = 1; i < pBitmap->layers; i++)
	{
		pBitmap->position[i].X += pBitmap->speed * (i * i * pDeltaTime);
		if (pBitmap->position[i].X > pWrapSize)
		{
			pBitmap->position[i].X = 0;
		}
	}
}

static inline void SetZLayer(RenderState* pState, float pLayer);
static void PushSizedQuad(RenderState* pState, v2 pMin, v2 pSize, Bitmap* pTexture);
static void PushSizedQuad(RenderState* pState, v2 pMin, v2 pSize, v4 pColor, Bitmap* pTexture);
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

static AnimatedBitmap CreateAnimatedBitmap(BITMAPS pBitmap, u32 pAnimationCount, u32* pAnimationLengths, v2 pFrameSize)
{
	AnimatedBitmap bitmap;
	bitmap.bitmap = pBitmap;
	bitmap.animation_count = pAnimationCount;
	bitmap.current_animation = 0;
	bitmap.current_index = 0;
	bitmap.frame_size = pFrameSize;
	bitmap.animation_lengths = pAnimationLengths;
	bitmap.frame_time = 0;
	return bitmap;
}

static void UpdateAnimation(AnimatedBitmap* pBitmap, float pDeltaTime)
{
	if ((pBitmap->frame_time += pDeltaTime) >= pBitmap->frame_duration)
	{
		pBitmap->frame_time = 0;
		pBitmap->current_index++;
		if (pBitmap->current_index >= *(pBitmap->animation_lengths + pBitmap->current_animation))
		{
			pBitmap->current_index = 0;
		}
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

static bool AnimationIsComplete(AnimatedBitmap* pBitmap)
{
	return pBitmap->current_index == pBitmap->animation_lengths[pBitmap->current_animation] - 1;
}