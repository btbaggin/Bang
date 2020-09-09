//Base structs
struct ServerMessage
{
	SERVER_MESSAGES message;
};
struct ClientMessage
{
	CLIENT_MESSAGES message;
	u8 client_id;
};

//Server->client messages
struct JoinResult : public ServerMessage
{
	JOIN_STATUS_CODES result;
	u8 slot;
};
struct GameStartAnnoucement : public ServerMessage
{
	u32 time;
	PLAYER_ROLES roles[MAX_PLAYERS];
	PLAYER_TEAMS teams[MAX_PLAYERS];
};
struct JoinAnnoucement : public ServerMessage
{
	char names[MAX_PLAYERS][NAME_LENGTH];
};
struct GameStateMessage : public ServerMessage
{
	u32 prediction_id;
	LocalPlayerState local_state;
	v2 positions[MAX_PLAYERS];
	SyncedPlayerState state[MAX_PLAYERS];
};
struct PlayerDiedMessage : public ServerMessage
{
	u8 client_id;
	PLAYER_ROLES role;
};
struct GameOverMessage : public ServerMessage
{
	PLAYER_ROLES winner;
};

//Client -> server messages
struct ClientLeave : public ClientMessage { };
struct GameStart : public ClientMessage { };
struct Join : public ClientMessage
{
	char name[NAME_LENGTH];
};
struct Ping : public ClientMessage { };
struct ClientInput : public ClientMessage
{
	u32 prediction_id;
	s8 attack_choice;
	float dt;
	u32 flags;
};

#define WriteMessage(pBuffer, data, type, message) WriteStructMessage(pBuffer, data, sizeof(type), message)
#define ReadMessage(pBuffer, name, type) type name; ReadStructMessage(pBuffer, &name, sizeof(type))
static u32 WriteStructMessage(u8* pBuffer, void* pStruct, u32 pSize, u8 pMessage)
{
	//Message needs to be the first field in all messages (should be fine since they all inherit the message struct)
	((u8*)pStruct)[0] = pMessage;

	return SerializeStruct(&pBuffer, pStruct, pSize);
}
static void ReadStructMessage(u8* pBuffer, void* pStruct, u32 pSize)
{
	DeserializeStruct(&pBuffer, pStruct, pSize);
}

#ifdef _SERVER
static void SendCurrentGameState(GameNetState* pNet, GameState* pState)
{
	u8* buffer = pNet->buffer;
	u8 message = SERVER_MESSAGE_State;
	Serialize(&buffer, &message, u8);

	//Set positions of all entities
	u8 player_mask = 0;
	for (u32 i = 0; i < MAX_PLAYERS; i++)
	{
		Player* p = pState->players[i];
		if (IsEntityValid(&pState->entities, p))
		{
			player_mask |= (1 << i);
		}
	}
	Serialize(&buffer, &player_mask, u8);


	for (u32 i = 0; i < MAX_PLAYERS; i++)
	{
		Client* client = pNet->clients + i;
		if (IsClientConnected(client))
		{
			Player* p = pState->players[i];
			Serialize(&buffer, &p->position, v2);
			Serialize(&buffer, &p->state, SyncedPlayerState);
		}
	}

	u8* buffer_save = buffer;
	for (u32 i = 0; i < MAX_PLAYERS; i++)
	{
		buffer = buffer_save;
		Client* client = pNet->clients + i;
		if (IsClientConnected(client))
		{
			Player* p = pState->players[i];
			Serialize(&buffer, &p->local_state, LocalPlayerState);
			Serialize(&buffer, &client->current_prediction_id, u32);
			SerializeEvents(&buffer, &pState->events);
			SocketSend(&pNet->listen_socket, client->endpoint, pNet->buffer, (u32)(buffer - pNet->buffer));
		}
	}

}
#else
static GameStateMessage ReadCurrentGameState(GameNetState* pNet, GameState* pState)
{
	GameStateMessage m = {};
	u8* buffer = pNet->buffer;

	Deserialize(&buffer, &m.message, u8);

	u8 player_mask;
	Deserialize(&buffer, &player_mask, u8);

	for (u32 i = 0; i < MAX_PLAYERS; i++)
	{
		Client* client = pNet->clients + i;
		if (HasFlag((1 << i), player_mask))
		{
			Deserialize(&buffer, &m.positions[i], v2);
			Deserialize(&buffer, &m.state[i], SyncedPlayerState);
		}
	}
	Deserialize(&buffer, &m.local_state, LocalPlayerState);
	Deserialize(&buffer, &m.prediction_id, u32);
	DeserializeEvents(&buffer, &pState->events);

	return m;
}
#endif