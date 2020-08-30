#pragma once
#include <winsock2.h>
#pragma comment(lib, "ws2_32")

//https://www.codersblock.org/blog/multiplayer-fps-part-3
const float CLIENT_TIMEOUT = 150.0F;
const u32 SOCKET_BUFFER_SIZE = 1024;
const u32 PORT = 11000;
const u32 MAX_PREDICTION_BUFFER = 128; //128 is just over 2 seconds for latency assuming 60fps
const u32 PREDICTION_BUFFER_MASK = MAX_PREDICTION_BUFFER - 1;

enum CLIENT_MESSAGES : u8
{
	CLIENT_MESSAGE_Join,
	CLIENT_MESSAGE_Input,
	CLIENT_MESSAGE_Ping,
	CLIENT_MESSAGE_Leave,
	CLIENT_MESSAGE_GameStart
};

enum SERVER_MESSAGES : u8
{
	SERVER_MESSAGE_JoinResult,
	SERVER_MESSAGE_JoinAnnouncement,
	SERVER_MESSAGE_GameStartAnnoucement,
	SERVER_MESSAGE_State,
	SERVER_MESSAGE_LeaveAnnoucement,
};

enum JOIN_STATUS_CODES
{
	JOIN_STATUS_Ok,
	JOIN_STATUS_GameStarted,
	JOIN_STATUS_LobbyFull,
	JOIN_STATUS_ServerNotFound,
};

struct IPEndpoint
{
	u32 ip;
	u16 port;
};

struct PlatformSocket
{
	SOCKET handle;
};


struct SyncedPlayerState
{
	u8 animation;
	u8 health;
	s8 team_attack_choice;
};
struct LocalPlayerState
{
	v2 velocity;
};
struct PredictedMove
{
	float dt;
	u32 input;
};
struct PredictedMoveResult
{
	v2 position;
	LocalPlayerState state;
};

struct Client
{
	u32 input_flags;
	float dt;
	float time_since_last_packet;
	char name[NAME_LENGTH];
	u32 current_prediction_id;

#ifdef _SERVER
	IPEndpoint endpoint;
#endif
};

struct GameNetState
{
	//Universal data
	u32 initialized;
	PlatformSocket send_socket;
	u8 buffer[SOCKET_BUFFER_SIZE];
	Client clients[MAX_PLAYERS];

	PredictedMove moves[MAX_PREDICTION_BUFFER];
	PredictedMoveResult results[MAX_PREDICTION_BUFFER];

#ifdef _SERVER
	PlatformSocket listen_socket;
	bool game_started;
#else
	u8 client_id;
	IPEndpoint server_ip;
	u32 latency_ms[UPDATE_FREQUENCY];
#endif
};

static JOIN_STATUS_CODES AttemptJoinServer(GameNetState* pState, const char* pName);