//https://gist.github.com/pelya/4babc0bab224bd22e6f30ce17d784c07
struct Pair {
	int x;
	int y;
	Pair(int a, int b) : x(a), y(b) {}
};

static void UpdateCache(std::vector<int> *c, int n, int M, int rowWidth, const u8* input) {
	for (int m = 0; m != M; ++m) {
		if (input[n * rowWidth + m] != 0) {
			(*c)[m]++;
		}
		else {
			(*c)[m] = 0;
		}
	}
}

static Rect FindBiggest(const u8* pInput, int pWidth, int pHeight, int pRowWidth) {

	Pair best_ll(0, 0); /* Lower-left corner */
	Pair best_ur(-1, -1); /* Upper-right corner */
	int best_area = 0;
	int best_perimeter = 0;

	std::vector<int> c(pWidth + 1, 0); /* Cache */
	std::vector<Pair> s; /* Stack */
	s.reserve(pWidth + 1);

	int m, n;

	/* Main algorithm: */
	for (n = 0; n != pHeight; ++n) {
		int open_width = 0;
		UpdateCache(&c, n, pWidth, pRowWidth, pInput);
		for (m = 0; m != pWidth + 1; ++m) {
			if (c[m] > open_width) { /* Open new rectangle? */
				s.push_back(Pair(m, open_width));
				open_width = c[m];
			}
			else if (c[m] < open_width) { /* Close rectangle(s)? */
				int m0, w0, area, perimeter;
				do {
					m0 = s.back().x;
					w0 = s.back().y;
					s.pop_back();
					area = open_width * (m - m0);
					perimeter = open_width + (m - m0);
					/* If the area is the same, prefer squares over long narrow rectangles,
						it finds more rectangles this way when calling findAll() with minLength == 2 or more */
					if (area > best_area || (area == best_area && perimeter < best_perimeter)) {
						best_area = area;
						best_perimeter = perimeter;
						best_ll.x = m0;
						best_ll.y = n;
						best_ur.x = m - 1;
						best_ur.y = n - open_width + 1;
					}
					open_width = w0;
				} while (c[m] < open_width);
				open_width = c[m];
				if (open_width != 0) {
					s.push_back(Pair(m0, w0));
				}
			}
		}
	}
	return { best_ll.x, max(0, best_ur.y), 1 + best_ur.x - best_ll.x, 1 + best_ll.y - best_ur.y };
}

Rect FindBiggest(const u8* pInput, int pWidth, int pHeight) {
	return FindBiggest(pInput, pWidth, pHeight, pWidth);
}

/** Find biggest rectangle, then recursively search area to the left/right and to the top/bottom of that rectangle
	for smaller rectangles, and choose the one that covers biggest area.
	@return biggest area size, covered by rectangles with side length = maxLength or bigger,
	@param search: limit searching for the following area
	@param output: may be NULL, then the function will only return the area size
*/
static long long FindRectsInArea(const u8* pInput, int pRowWidth, u16* pOutput, Rect pSearch) {
	if (pSearch.w < 1 || pSearch.h < 1) {
		return 0; // We reached a size limit
	}
	Rect biggest = FindBiggest(pInput + pSearch.y * pRowWidth + pSearch.x, pSearch.w, pSearch.h, pRowWidth);

	if (biggest.w < 1 || biggest.h < 1) {
		return 0; // No rectangles here
	}
	biggest.x += pSearch.x;
	biggest.y += pSearch.y;
	if (biggest.w > USHRT_MAX) biggest.w = USHRT_MAX;
	if (biggest.h > USHRT_MAX) biggest.h = USHRT_MAX;

	if (pOutput != NULL) {
		for (int y = biggest.y; y < biggest.y + biggest.h; y++) {
			for (int x = biggest.x; x < biggest.x + biggest.w; x++) {
				pOutput[(y * pRowWidth + x) * 2] = 0;
				pOutput[(y * pRowWidth + x) * 2 + 1] = 0;
			}
		}
		pOutput[(biggest.y * pRowWidth + biggest.x) * 2] = biggest.w;
		pOutput[(biggest.y * pRowWidth + biggest.x) * 2 + 1] = biggest.h;
	}

		return (long long)biggest.w * biggest.h +
			FindRectsInArea(pInput, pRowWidth, pOutput, { pSearch.x, pSearch.y, biggest.x - pSearch.x, pSearch.h }) +
			FindRectsInArea(pInput, pRowWidth, pOutput, { biggest.x + biggest.w, pSearch.y, pSearch.x + pSearch.w - biggest.x - biggest.w, pSearch.h }) +
			FindRectsInArea(pInput, pRowWidth, pOutput, { biggest.x, pSearch.y, biggest.w, biggest.y - pSearch.y }) +
			FindRectsInArea(pInput, pRowWidth, pOutput, { biggest.x, biggest.y + biggest.h, biggest.w, pSearch.y + pSearch.h - biggest.y - biggest.h });
}

long long FindAll(const u8* pInput, int pWidth, int pHeight, u16* pOutput) {
	return FindRectsInArea(pInput, pWidth, pOutput, { 0, 0, pWidth, pHeight });
}

static void GetLevelCollisionMask(cute_tiled_map_t* pMap, cute_tiled_layer_t* pLayer, MemoryStack* pStack)
{
	u8* map = PushZerodArray(pStack, u8, pMap->width * pMap->height);

	//Mark solid tiles
	cute_tiled_tileset_t* tiles = pMap->tilesets;
	while (tiles)
	{
		cute_tiled_tile_descriptor_t* tile = tiles->tiles;
		while (tile)
		{
			for (int i = 0; i < tile->property_count; i++)
			{
				cute_tiled_property_t* prop = tile->properties + i;
				if (strcmp(prop->name.ptr, "solid") == 0)
				{
					for (u32 i = 0; i < (u32)pLayer->data_count; i++)
					{
						int gid = pLayer->data[i] - tiles->firstgid;
						if (gid == tile->tile_index)
						{
							u32 r = i / pMap->width;
							u32 c = i % pMap->width;
							map[r * pMap->width + c] = 1;
						}
					}
				}
			}
			tile = tile->next;
		}
		tiles = tiles->next;
	}

	//Find the largest rectangles within the collision array
	u16* output = PushZerodArray(pStack, u16, pMap->width * pMap->height * 2);
	FindAll(map, pMap->width, pMap->height, output);

	//Create walls for each rectangle
	for (int y = 0; y < pMap->height; y++) {
		for (int x = 0; x < pMap->width; x++) {
			u16 width = output[(y * pMap->width + x) * 2];
			u16 height = output[(y * pMap->width + x) * 2 + 1];
			if (width > 0)
			{
				float w = (float)pMap->tilewidth;
				float h = (float)pMap->tileheight;
				Wall* e = CreateEntity(&g_state.entities, Wall);
				
				e->position = V2(x * w, y * h);

				RigidBodyCreationOptions o = {};
				o.height = height * h;
				o.width = width * w;
				o.entity = e;
				o.type = SHAPE_Poly;

				AddRigidBody(g_state.world_arena, &g_state.physics, e, &o);
			}
		}
	}
}