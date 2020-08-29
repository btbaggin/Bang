static void SendFontToGraphicsCard(void* pData)
{
	if (pData)
	{
		FontInfo* font = (FontInfo*)pData;

		glGenTextures(1, &font->texture);
		glBindTexture(GL_TEXTURE_2D, font->texture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, font->atlasWidth, font->atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, font->data);
		glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
		glGenerateMipmap(GL_TEXTURE_2D);

		delete font->data;
		font->data = nullptr;
	}
}

static FontInfo* LoadFontAsset(Assets* pAssets, const char* pPath, float pSize)
{
	//https://github.com/0xc0dec/demos/blob/master/src/stb-truetype/StbTrueType.cpp
	const u32 firstChar = ' ';
	const u32 charCount = '~' - ' ';
	const u32 ATLAS_SIZE = 2048;

	//Alloc in one big block so we can free in one big block
	void* data = Alloc(pAssets, (sizeof(stbtt_packedchar) * charCount) + sizeof(FontInfo));

	FontInfo* font = (FontInfo*)data;
	font->size = pSize;
	font->atlasWidth = ATLAS_SIZE;
	font->atlasHeight = ATLAS_SIZE;
	font->charInfo = (stbtt_packedchar*)((char*)data + sizeof(FontInfo));
	font->data = new u8[ATLAS_SIZE * ATLAS_SIZE];

	std::ifstream file(pPath, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		DisplayErrorMessage("Unable to read font file", ERROR_TYPE_Warning);
		return nullptr;
	}
	const auto size = file.tellg();
	file.seekg(0, std::ios::beg);
	u8* bytes = (u8*)Alloc(pAssets, size);
	file.read(reinterpret_cast<char*>(&bytes[0]), size);
	file.close();

	stbtt_fontinfo info;
	stbtt_InitFont(&info, bytes, stbtt_GetFontOffsetForIndex(bytes, 0));

	stbtt_pack_context context;
	if (!stbtt_PackBegin(&context, font->data, font->atlasWidth, font->atlasHeight, 0, 1, nullptr))
	{
		DisplayErrorMessage("Unable to pack font", ERROR_TYPE_Warning);
		return nullptr;
	}

	stbtt_PackSetOversampling(&context, 5, 5);
	if (!stbtt_PackFontRange(&context, bytes, 0, font->size, firstChar, charCount, font->charInfo))
	{
		DisplayErrorMessage("Unable pack font range", ERROR_TYPE_Warning);
		return nullptr;
	}

	stbtt_PackEnd(&context);
	Free(bytes);
	file.close();

	float scale = stbtt_ScaleForPixelHeight(&info, font->size);
	int ascent;
	stbtt_GetFontVMetrics(&info, &ascent, 0, 0);
	font->baseline = ascent * scale;

	return font;
}

static FontInfo* GetFont(Assets* pAssets, AssetSlot* pSlot)
{
	RequestAsset(pAssets, pSlot, ASSET_TYPE_Font);
	return pSlot->font;
}

static FontInfo* GetFont(Assets* pAssets, FONTS pFont)
{
	return GetFont(pAssets, pAssets->fonts + pFont);
}

static v2 MeasureString(FONTS pFont, const char* pText)
{
	v2 size = {};
	float x = 0;
	FontInfo* font = GetFont(g_transstate.assets, pFont);
	if (font)
	{
		size.Height = font->size;
		float y = 0.0F;
		stbtt_aligned_quad quad;
		for (u32 i = 0; pText[i] != 0; i++)
		{
			const unsigned char c = pText[i];
			if (c == '\n')
			{
				size.Height += font->size;
				x = 0;
			}
			else
			{
				assert(c - ' ' >= 0);
				stbtt_GetPackedQuad(font->charInfo, font->atlasWidth, font->atlasHeight, c - ' ', &x, &y, &quad, 1);
				size.Width = max(size.Width, x);
			}
		}
	}
	return size;
}

inline static v2 CenterText(FONTS pFont, const char* pText, v2 pBounds)
{
	v2 size = MeasureString(pFont, pText);
	return (pBounds - size) / 2.0F;
}
inline static float CenterTextHorizontally(FONTS pFont, const char* pText, float pWidth)
{
	v2 size = MeasureString(pFont, pText);
	return (pWidth - size.Width) / 2.0F;
}

static float CharWidth(FONTS pFont, char pChar)
{
	FontInfo* font = GetFont(g_transstate.assets, pFont);
	if (font)
	{
		if (pChar == '\n' || pChar - ' ' < 0)
		{
			return 0;
		}
		else
		{
			stbtt_aligned_quad quad;
			float y = 0.0F;
			float x = 0.0F;

			stbtt_GetPackedQuad(font->charInfo, font->atlasWidth, font->atlasHeight, pChar - ' ', &x, &y, &quad, 1);
			return x;
		}
	}
	return 0;
}

static float GetFontSize(FONTS pFont)
{
	FontInfo* font = GetFont(g_transstate.assets, pFont);
	if (font) return font->size;
	return 0;
}

static void WrapText(char* pText, u32 pLength, FONTS pFont, float pWidth)
{
	u32 last_index = 0;
	float total_size = 0;
	float word_size = 0;
	for (u32 i = 0; i <= pLength; i++)
	{
		if (pText[i] == '\n')
		{
			total_size = 0;
			last_index = i;
		}
		else if (i == pLength || (pText[i] > 0 && isspace(pText[i])))
		{
			word_size = 0;
			for (u32 j = last_index; j < i; j++)
			{
				word_size += CharWidth(pFont, pText[j]);
			}

			total_size += word_size;
			if (total_size > pWidth)
			{
				pText[last_index] = '\n';
				total_size = word_size;
			}
			last_index = i;
		}
	}
}