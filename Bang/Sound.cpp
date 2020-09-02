static Sound* GetSound(Assets* pAssets, AssetSlot* pSlot)
{
	RequestAsset(pAssets, pSlot, ASSET_TYPE_Sound);
	return pSlot->sound;

}
static Sound* GetSound(Assets* pAssets, SOUNDS pSound)
{
	return GetSound(pAssets, pAssets->sounds + pSound);
}

Sound* LoadSoundAsset(Assets* pAssets, const char* pFile)
{
	Sound* sound = nullptr;
	FILE *fin = fopen(pFile, "rb");
	if (fin)
	{
		//Read WAV header
		wav_header_t header;
		fread(&header, sizeof(header), 1, fin);

		chunk_t chunk;
		//go to data chunk
		while (true)
		{
			fread(&chunk, sizeof(chunk), 1, fin);
			if (*(unsigned int *)&chunk.ID == 0x61746164)
				break;
			//skip chunk data bytes
			fseek(fin, chunk.size, SEEK_CUR);
		}

		u32 sample_count = chunk.size * 8 / header.bitsPerSample;

		//Make one big allocation so if we free it, it frees everything
		void* data = Alloc(pAssets, (sizeof(s16) * sample_count) + sizeof(Sound));
		sound = (Sound*)data;
		sound->samples.items = (s16*)((char*)data + sizeof(Sound));

		//Number of samples
		int sample_size = header.bitsPerSample / 8;
		sound->samples.count = sample_count;
		sound->channels = header.numChannels;

		//Reading data
		for (u32 i = 0; i < sound->samples.count; i++)
		{
			fread(&sound->samples.items[i], sample_size, 1, fin);
			if (feof(fin)) DisplayErrorMessage("Unable to read wav file", ERROR_TYPE_Warning);
		}

		//Write data into the file
		fclose(fin);
	}
	else
	{
		LogError("Unable to open sound asset %s", pFile);
	}

	return sound;
}

static PlayingSound* GetAvailablePlayingSound()
{
	PlayingSound* p = g_transstate.available_sounds.Request();
	if (!p) p = PushStruct(g_state.world_arena, PlayingSound);
	return p;
}

static PlayingSound* PlaySound(Assets* pAssets, SOUNDS pSound, float pVolume = 1.0F)
{
	PlayingSound* p = GetAvailablePlayingSound();
	Sound* sound = GetSound(pAssets, pSound);
	if (!sound) p->sound = pSound;
	p->loaded_sound = sound;

	p->samples_played = 0;
	p->status = SOUND_STATUS_Play;
	p->volume = pow(pVolume, 2.0F); //Logarithmic volume scaling

	p->next = g_state.FirstPlaying;
	g_state.FirstPlaying = p;

	return p;
}

static PlayingSound* LoopSound(Assets* pAssets, SOUNDS pSound, float pVolume = 1.0F)
{
	PlayingSound* p = GetAvailablePlayingSound();
	Sound* sound = GetSound(pAssets, pSound);
	if (!sound) p->sound = pSound;
	p->loaded_sound = sound;

	p->samples_played = 0;
	p->status = SOUND_STATUS_Loop;
	p->volume = pow(pVolume, 2.0F); //Logarithmic volume scaling

	p->next = g_state.FirstPlaying;
	g_state.FirstPlaying = p;

	return p;
}

void StopSound(PlayingSound* pSound)
{
	pSound->status = SOUND_STATUS_Stop;
}

void PauseSound(PlayingSound* pSound)
{
	pSound->status = SOUND_STATUS_Pause;
}

void ResumeSound(PlayingSound* pSound)
{
	pSound->status = SOUND_STATUS_Play;
}

void ResumeLoopSound(PlayingSound* pSound)
{
	pSound->status = SOUND_STATUS_Loop;
}

static void GameGetSoundSamples(GameState* pState, GameTransState* pTransState, GameSoundBuffer* pSound)
{
	auto h = BeginTemporaryMemory(pState->world_arena);
	float* sound_buffer0 = PushArray(pState->world_arena, float, pSound->sample_count);
	float* sound_buffer1 = PushArray(pState->world_arena, float, pSound->sample_count);

	float* channel0 = sound_buffer0;
	float* channel1 = sound_buffer1;
	for (u32 i = 0; i < pSound->sample_count; i++)
	{
		*channel0++ = 0;
		*channel1++ = 0;
	}

	PlayingSound* prev = nullptr;
	for (PlayingSound** s = &pState->FirstPlaying; *s;)
	{
		PlayingSound* current = *s;
		if (current)
		{
			bool remove = false;
			if (!current->loaded_sound)
			{
				//Pretend we are playing the sound while its loading
				current->loaded_sound = GetSound(g_transstate.assets, current->sound);
				current->samples_played += pSound->sample_count;
			}
			else if (current->status == SOUND_STATUS_Play || current->status == SOUND_STATUS_Loop)
			{
				Sound* sound = current->loaded_sound;
				channel0 = sound_buffer0;
				channel1 = sound_buffer1;
				u32 samples = pSound->sample_count;
				u32 samples_remaining = sound->samples.count - current->samples_played;
				if (samples > samples_remaining) samples = samples_remaining;

				for (u32 i = 0; i < samples; i++)
				{
					float sample = (float)sound->samples.items[current->samples_played + i];
					*channel0++ += current->volume * sample;
					*channel1++ += current->volume * sample;
				}

				current->samples_played += samples;
				if (current->samples_played >= sound->samples.count)
				{
					if (current->status == SOUND_STATUS_Loop)
					{
						current->samples_played = 0;
					}
					else
					{
						remove = true;
					}
				}
			}
			else if (current->status == SOUND_STATUS_Stop)
			{
				remove = true;
			}

			if (remove)
			{
				if (prev) prev->next = current->next;
				else pState->FirstPlaying = nullptr;

				prev = *s;
				*s = current->next;

				pTransState->available_sounds.Append(current);
			}
			else
			{
				prev = *s;
				s = &current->next;
			}
		}
	}

	channel0 = sound_buffer0;
	channel1 = sound_buffer1;
	s16* out = (s16*)pSound->data;
	for (u32 i = 0; i < pSound->sample_count; i++)
	{
		*channel0 = clamp(INT16_MIN, *channel0, INT16_MAX);
		*channel1 = clamp(INT16_MIN, *channel1, INT16_MAX);
		*out++ = (s16)(*channel0++ + 0.5F);
		*out++ = (s16)(*channel1++ + 0.5F);
	}
	EndTemporaryMemory(h);
}