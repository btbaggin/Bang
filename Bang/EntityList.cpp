static EntityList GetEntityList(MemoryStack* pStack)
{
	EntityList list = {};
	list.end_index = 0;
	list.entities = PushArray(pStack, Entity*, MAX_ENTITIES);
	list.versions = PushArray(pStack, u16, MAX_ENTITIES);

	return list;
}

//https://stackoverflow.com/questions/222557/what-uses-are-there-for-placement-new
#define CreateEntity(list, struct_type) (struct_type*)AddEntity(list, PushClass(g_state.world_arena, struct_type), ENTITY_TYPE_##struct_type##);
static Entity* AddEntity(EntityList* pList, Entity* pEntity, ENTITY_TYPES pType)
{
	u32 index = pList->free_indices.Request();
	if (!index)
	{
		index = pList->end_index;
		pList->end_index++;
	}

	u16 version = pList->versions[index];

	EntityIndex i;
	i.version = version;
	i.index = index + 1;

	pList->entities[index] = pEntity;
	pEntity->index = i;
	pEntity->type = pType;

	return pEntity;
}

static void RemoveEntity(EntityList* pList, Entity* pEntity)
{
	u32 i = pEntity->index.index - 1;
	pList->free_indices.Append(i);

	u16* version = pList->versions + i;
	++(*version);
}

static Entity* GetEntity(EntityList* pList, EntityIndex pIndex)
{
	if (pIndex.index == 0) return nullptr;

	Entity* e = pList->entities[pIndex.index - 1];
	if (e->index.version == pList->versions[pIndex.index - 1])
	{
		return e;
	}

	return nullptr;
}

static inline bool IsEntityValid(EntityList* pList, Entity* pEntity)
{
	if (!pEntity) return false;

	EntityIndex i = pEntity->index;
	return (i.version == pList->versions[i.index - 1]);
}

static void ClearEntityList(EntityList* pList)
{
	pList->end_index = 0;
	for (u32 i = 0; i < MAX_ENTITIES; i++)
	{
		pList->versions[i]++;
	}
}

static void FindEntitiesWithinRange(EntityList* pList, v2 pPosition, float pRange, Entity** pEntities, u32* pCount, ENTITY_TYPES pType = ENTITY_TYPE_None)
{
	*pCount = 0;
	for (u32 i = 0; i < pList->end_index; i++)
	{
		Entity* e = pList->entities[i];
		if (IsEntityValid(&g_state.entities, e) && HMM_LengthSquared(pPosition - e->position) < pRange * pRange)
		{
			if (pType != ENTITY_TYPE_None && pType == e->type)
			{
				pEntities[(*pCount)++] = e;
			}
		}
	}
}