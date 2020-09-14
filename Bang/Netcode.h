#pragma once
#include <winsock2.h>
#pragma comment(lib, "ws2_32")

//https://www.codersblock.org/blog/multiplayer-fps-part-3
const float PING_TIME = 5.0F;
const float CLIENT_TIMEOUT = PING_TIME * 3;
const u32 SOCKET_BUFFER_SIZE = 1024;
const u32 PORT = 11000;
const u32 MAX_PREDICTION_BUFFER = 128; //128 is just over 2 seconds for latency assuming 60fps
const u32 PREDICTION_BUFFER_MASK = MAX_PREDICTION_BUFFER - 1;
#define ALLOW_PARTIAL_GAMES _DEBUG 

enum CLIENT_MESSAGES : u8
{
	//Request to join the server
	CLIENT_MESSAGE_Join,
	//Send current frames input to the server
	CLIENT_MESSAGE_Input,
	//Ping every X seconds so the server knows we are still present
	CLIENT_MESSAGE_Ping,
	//Tell the server we are leaving the game
	CLIENT_MESSAGE_Leave,
	//Request the server to start the game
	CLIENT_MESSAGE_GameStart
};

enum SERVER_MESSAGES : u8
{
	//Reply when a client attempts to join the server
	SERVER_MESSAGE_JoinResult,
	//Announcement to all players that someone joined
	SERVER_MESSAGE_JoinAnnouncement,
	//Annoucement to all players that the game has started
	SERVER_MESSAGE_GameStartAnnoucement,
	//Game state updates
	SERVER_MESSAGE_State,
	//Tell all players that someone has died, reveals their role
	SERVER_MESSAGE_PlayerDied,
	//Tell all players that the game is over
	SERVER_MESSAGE_GameOver,
	//Annoucement to all players that someone has left
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


struct CurrentInput
{
	u32 flags;
	v2 mouse;
};
struct SyncedPlayerState
{
	u8 animation;
	u8 health;
	s8 team_attack_choice;
};
static bool SynedStateHasChanged(SyncedPlayerState* pP1, SyncedPlayerState* pP2)
{
	return memcmp(pP1, pP2, sizeof(SyncedPlayerState));
}
struct LocalPlayerState
{
	v2 velocity;
	u8 beers;
};
struct PredictedMove
{
	float dt;
	CurrentInput input;
};
struct PredictedMoveResult
{
	v2 position;
	LocalPlayerState state;
};

struct Client
{
	CurrentInput input;
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
#else
	u8 client_id;
	IPEndpoint server_ip;
	u32 latency_ms[UPDATE_FREQUENCY];
	Timer ping_timer;
#endif
};

static JOIN_STATUS_CODES AttemptJoinServer(GameNetState* pState, const char* pName, IPEndpoint pEndpoint);