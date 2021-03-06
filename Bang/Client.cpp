
static JOIN_STATUS_CODES AttemptJoinServer(GameNetState* pState, const char* pName, IPEndpoint pEndpoint)
{
	const u32 MAX_TRIES = 100;

	NetStart(pState, &pState->send_socket);
	pState->server_ip = pEndpoint;

	Join j = {};
	j.client_id = 0;
	strcpy(j.name, pName);

	u32 size = WriteMessage(pState->buffer, &j, Join, CLIENT_MESSAGE_Join);
	SocketSend(&pState->send_socket, pState->server_ip, pState->buffer, size);

	//Wait until we recieve a response
	u32 tries = 0;
	IPEndpoint endpoint;
	while (!SocketReceive(&pState->send_socket, pState->buffer, SOCKET_BUFFER_SIZE, &size, &endpoint) && tries++ < MAX_TRIES)
	{
		Sleep(50);
	}

	if (tries > MAX_TRIES) return JOIN_STATUS_ServerNotFound;

	ReadMessage(pState->buffer, jr, JoinResult);
	pState->client_id = jr.slot;
	return jr.result;
}

static void ProcessServerMessages(GameNetState* pState, u32 pPredictionId, float pDeltaTime)
{
	u32 bytes_received;
	IPEndpoint from;
	if (pState->initialized)
	{
		while (SocketReceive(&pState->send_socket, pState->buffer, SOCKET_BUFFER_SIZE, &bytes_received, &from))
		{
			SERVER_MESSAGES message = (SERVER_MESSAGES)pState->buffer[0];

			switch (message)
			{
			case SERVER_MESSAGE_JoinAnnouncement:
			{
				ReadMessage(pState->buffer, j, JoinAnnoucement);
				for (u32 i = 0; i < MAX_PLAYERS; i++)
				{
					Client* c = pState->clients + i;
					strcpy(c->name, j.names[i]);
				}
			}
			break;

			case SERVER_MESSAGE_GameStartAnnoucement:
				//Sync random number with server so we shouldnt have any desyncs from random events
				ReadMessage(pState->buffer, s, GameStartAnnoucement);
				srand(s.time);

				EndScreen(0);
				for (u32 i = 0; i < MAX_PLAYERS; i++)
				{
					Client* c = pState->clients + i;
					if (IsClientConnected(c))
					{
						Player* p = CreatePlayer(&g_state, pState, c->name);
						p->team = s.teams[i];
						p->role = s.roles[i];
					}
				}
				break;

			case SERVER_MESSAGE_State:
			{
				GameStateMessage m = ReadCurrentGameState(pState, &g_state, &g_transstate);

				for (u32 i = 0; i < MAX_PLAYERS; i++)
				{
					Client* c = pState->clients + i;
					if (IsClientConnected(c))
					{
						Player* p = g_state.players[i];
						v2 received_position = m.positions[i];
						SyncedPlayerState received_state = m.state[i];

						//We only do prediction on our character, all other characters are just synced from the server
						if (pState->client_id == i && m.prediction_id > 0)
						{
							//Figure out how many ticks ahead our simulation is vs the server
							s32 ticks_ahead = pPredictionId - m.prediction_id;
							assert(ticks_ahead > -1);
							assert(ticks_ahead <= MAX_PREDICTION_BUFFER);
							pState->latency_ms[pPredictionId % UPDATE_FREQUENCY] = (u32)((ticks_ahead * ExpectedSecondsPerFrame) / 1000);

							LocalPlayerState received_local_state = m.local_state;
							u32 index = m.prediction_id & PREDICTION_BUFFER_MASK;
							v2 delta_pos = received_position - pState->results[index].position;

							//Check if our local sim mispredicted something 
							if (HMM_LengthSquared(delta_pos) > 0.001f * 0.001f || SynedStateHasChanged(&received_state, &p->state))
							{
								LogInfo("error of(%f, %f) detected at prediction id %d, rewinding and replaying", delta_pos.X, delta_pos.Y, m.prediction_id);

								p->position = received_position;
								p->body->velocity = received_local_state.velocity;;
								p->state = received_state;

								//Replay all moves from when the desync happened so the local sim should be synced again
								for (u32 replaying_prediction_id = m.prediction_id + 1;
									replaying_prediction_id < pPredictionId;
									++replaying_prediction_id)
								{
									u32	replaying_index = replaying_prediction_id & PREDICTION_BUFFER_MASK;

									PredictedMove* replaying_move = &pState->moves[replaying_index];
									PredictedMoveResult* replaying_move_result = &pState->results[replaying_index];

									p->Update(&g_state, replaying_move->dt, replaying_move->input);
									StepPhysics(&g_state.physics, ExpectedSecondsPerFrame);

									replaying_move_result->position = p->position;
									replaying_move_result->state = p->local_state;
								}
							}
						}
						else
						{
							p->position = received_position;
							p->state = received_state;
							SetAnimation(&p->bitmap, p->state.animation, 0.25F);
							UpdateAnimation(&p->bitmap, pDeltaTime);
						}
					}
					c->time_since_last_packet = 0;
				}
			}
			break;

			case SERVER_MESSAGE_PlayerDied:
			{
				ReadMessage(pState->buffer, m, PlayerDiedMessage);
				Player* p = g_state.players[m.client_id];
				p->state.health = 0;
				p->role = m.role;
				RemoveRigidBody(p, &g_state.physics);
			}
			break;

			case SERVER_MESSAGE_GameOver:
			{
				ReadMessage(pState->buffer, m, GameOverMessage);
				EndScreen(3.0F);
				((GameScreen*)g_interface.current_screen)->winner = m.winner;
				g_state.physics.bodies.clear();
			}
			break;

			case SERVER_MESSAGE_LeaveAnnoucement:
				ReadMessage(pState->buffer, l, ClientLeave);
				Client* c = pState->clients + l.client_id;
				c->name[0] = 0;
				if (g_state.game_started)
				{
					Player* p = g_state.players[l.client_id];
					DestroyEntity(&g_state.entities, p);
					DestroyParticleSystem(&p->dust);
				}
				break;
			}
		}


		//We should recieve state packets constantly, if we havent recieved any, connection to the host is lost
		if (g_state.game_started)
		{
			Client* c = pState->clients + pState->client_id;
			c->time_since_last_packet += pDeltaTime;
			if (c->time_since_last_packet > CLIENT_TIMEOUT)
			{
				DisplayErrorMessage("Unable to reach host", ERROR_TYPE_Warning);
				EndScreen(0.0F); //TODO this needs to always go to main menu
				g_state.physics.bodies.clear();
			}
		}
		else
		{
			//Send ping to server so it knows if we disconnect
			//We only need to do this during lobby since we have input messages during the game
			if (TickTimer(&pState->ping_timer, pDeltaTime))
			{
				Ping p = {};
				p.client_id = pState->client_id;

				u32 size = WriteMessage(pState->buffer, &p, Ping, CLIENT_MESSAGE_Ping);
				SocketSend(&pState->send_socket, pState->server_ip, pState->buffer, size);
			}
		}
	}
}
