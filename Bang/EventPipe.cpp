enum GAME_EVENTS : u8
{
	GAME_EVENT_SpawnBeer,
};

struct Event
{
	GAME_EVENTS type;
};

struct SpawnBeerEvent : Event
{
	v2 position;
};

#define PushGameEvent(pPipe, type, evnt) (type*)PushEvent(pPipe, PushStruct(g_transstate.trans_arena, type), evnt)
static Event* PushEvent(EventPipe* pPipe, Event* pEvent, GAME_EVENTS pType)
{
	assert(pPipe->events.count < MAX_GAME_EVENTS);
	pPipe->events.AddItem(pEvent);

	pEvent->type = pType;

	LogInfo("Generating event: %d", pType);

	return pEvent;
}

static u32 SerializeEvents(u8** pBuffer, EventPipe* pPipe)
{
	u8 size = Serialize(pBuffer, &pPipe->events.count, u8);
	for (u32 i = 0; i < pPipe->events.count; i++)
	{
		Event* e = pPipe->events.items[i];
		switch (e->type)
		{
			case GAME_EVENT_SpawnBeer:
				size += Serialize(pBuffer, e, SpawnBeerEvent);
				break;

			default:
				assert(false);
		}
	}

	return size;
}

static void DeserializeEvents(u8** pBuffer, EventPipe* pPipe)
{
	u8 count;
	Deserialize(pBuffer, &count, u8);
	for (u32 i = 0; i < count; i++)
	{
		GAME_EVENTS e = (GAME_EVENTS)*pBuffer[0];
		switch (e)
		{
			case GAME_EVENT_SpawnBeer:
			{
				SpawnBeerEvent* ev = PushStruct(g_transstate.trans_arena, SpawnBeerEvent);
				Deserialize(pBuffer, ev, SpawnBeerEvent);
				pPipe->events.AddItem(ev);
			}
			break;

			default:
				assert(false);
		}
	}
}

static void ProcessEvents(GameState* pState, EventPipe* pPipe)
{
	for (u32 i = 0; i < pPipe->events.count; i++)
	{
		Event* e = pPipe->events.items[i];
		switch (e->type)
		{
			case GAME_EVENT_SpawnBeer:
			{
				SpawnBeerEvent* beer = (SpawnBeerEvent*)e;
				Beer* b = CreateEntity(&pState->entities, Beer);
				b->position = beer->position;
				b->original_pos = beer->position;
			}
			break;

			default:
				assert(false);
		}
	}
	pPipe->events.count = 0;

}