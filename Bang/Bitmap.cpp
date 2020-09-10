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