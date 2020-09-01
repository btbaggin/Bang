//https://gist.github.com/pelya/4babc0bab224bd22e6f30ce17d784c07
struct Pair {
	int x;
	int y;
	Pair(int a, int b) : x(a), y(b) {}
};

static void update_cache(std::vector<int> *c, int n, int M, int rowWidth, const u8* input) {
	for (int m = 0; m != M; ++m) {
		if (input[n * rowWidth + m] != 0) {
			(*c)[m]++;
		}
		else {
			(*c)[m] = 0;
		}
	}
}

static Rect findBiggest(const u8* input, int M, int N, int rowWidth) {

	Pair best_ll(0, 0); /* Lower-left corner */
	Pair best_ur(-1, -1); /* Upper-right corner */
	int best_area = 0;
	int best_perimeter = 0;

	std::vector<int> c(M + 1, 0); /* Cache */
	std::vector<Pair> s; /* Stack */
	s.reserve(M + 1);

	int m, n;

	/* Main algorithm: */
	for (n = 0; n != N; ++n) {
		int open_width = 0;
		update_cache(&c, n, M, rowWidth, input);
		for (m = 0; m != M + 1; ++m) {
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

Rect findBiggest(const u8* input, int width, int height) {
	return findBiggest(input, width, height, width);
}

/** Find biggest rectangle, then recursively search area to the left/right and to the top/bottom of that rectangle
	for smaller rectangles, and choose the one that covers biggest area.
	@return biggest area size, covered by rectangles with side length = maxLength or bigger,
	@param search: limit searching for the following area
	@param output: may be NULL, then the function will only return the area size
*/

static long long findRectsInArea(const u8* input, int rowWidth, u16* output, Rect search) {
	if (search.w < 1 || search.h < 1) {
		return 0; // We reached a size limit
	}
	Rect biggest = findBiggest(input + search.y * rowWidth + search.x, search.w, search.h, rowWidth);

	if (biggest.w < 1 || biggest.h < 1) {
		return 0; // No rectangles here
	}
	biggest.x += search.x;
	biggest.y += search.y;
	if (biggest.w > USHRT_MAX) biggest.w = USHRT_MAX;
	if (biggest.h > USHRT_MAX) biggest.h = USHRT_MAX;

	if (output != NULL) {
		for (int y = biggest.y; y < biggest.y + biggest.h; y++) {
			for (int x = biggest.x; x < biggest.x + biggest.w; x++) {
				output[(y * rowWidth + x) * 2] = 0;
				output[(y * rowWidth + x) * 2 + 1] = 0;
			}
		}
		output[(biggest.y * rowWidth + biggest.x) * 2] = biggest.w;
		output[(biggest.y * rowWidth + biggest.x) * 2 + 1] = biggest.h;
	}

		return (long long)biggest.w * biggest.h +
			findRectsInArea(input, rowWidth, output, { search.x, search.y, biggest.x - search.x, search.h }) +
			findRectsInArea(input, rowWidth, output, { biggest.x + biggest.w, search.y, search.x + search.w - biggest.x - biggest.w, search.h }) +
			findRectsInArea(input, rowWidth, output, { biggest.x, search.y, biggest.w, biggest.y - search.y }) +
			findRectsInArea(input, rowWidth, output, { biggest.x, biggest.y + biggest.h, biggest.w, search.y + search.h - biggest.y - biggest.h });
}

long long findAll(const u8* input, int width, int height, u16* output) {
	return findRectsInArea(input, width, output, { 0, 0, width, height });
}

static void GetLevelCollisionMask(cute_tiled_map_t* pMap, cute_tiled_layer_t* pLayer, MemoryStack* pStack)
{
	u8* map = PushZerodArray(pStack, u8, pMap->width * pMap->height);

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

	u16* output = PushZerodArray(pStack, u16, pMap->width * pMap->height * 2);
	findAll(map, pMap->width, pMap->height, output);

	for (int y = 0; y < pMap->height; y++) {
		for (int x = 0; x < pMap->width; x++) {
			u16 width = output[(y * pMap->width + x) * 2];
			u16 heigh = output[(y * pMap->width + x) * 2 + 1];
			if (width > 0)
			{
				//TODO dont hardcode 32
				Wall* e = CreateEntity(&g_state.entities, Wall);
				
				e->position = V2(x * 32.0F, y * 32.0F);

				RigidBodyCreationOptions o = {};
				o.height = heigh * 32.0F;
				o.width = width * 32.0F;
				o.entity = e;
				o.type = SHAPE_Poly;

				AddRigidBody(g_state.world_arena, &g_state.physics, &o);
			}
			//AddRigidBody();
		}
	}
}