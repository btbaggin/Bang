static void ParseFloats(char* pToken, char** pNextToken, float* pData, u32 pCount)
{
	for (u32 i = 0; i < pCount; i++)
	{
		pToken = strtok_s(NULL, ",", pNextToken);
		pData[i] = (float)atof(pToken);
	}
	assert(**pNextToken == 0);
}
static void ParseInts(char* pToken, char** pNextToken, u32* pData, u32 pCount)
{
	for (u32 i = 0; i < pCount; i++)
	{
		pToken = strtok_s(NULL, ",", pNextToken);
		pData[i] = (u32)atoi(pToken);
	}
	assert(**pNextToken == 0);
}

static time_t CheckFileLastWrite(const char* pPath)
{
	struct stat result;
	if (stat(pPath, &result) == 0)
	{
		return result.st_mtime;
	}
	return 0;
}
static u32 LinesInFile(FILE* pFile)
{
	u32 lines = 0;
	int ch;
	while (EOF != (ch = getc(pFile)))
		if ('\n' == ch) ++lines;

	fseek(pFile, 0, SEEK_SET);
	return lines + 1;
}
static void LoadConfigFileSettings(ConfigFile* pFile, FILE* pHandle)
{	
	char line[200];
	while (!feof(pHandle))
	{
		char* next_token;
		char* token;
		fgets(line, 200, pHandle);

		//Check first non-whitespace character
		char* c = line;
		while (*c == ' ' || *c == '\t')  c++;
		//# denotes a comment
		if (*c == '#' || *c == '\n') continue;

		//setting name
		token = strtok_s(line, " ", &next_token);
		std::string name(token);

		//setting type
		token = strtok_s(NULL, " ", &next_token);

		//setting value
		ConfigSetting* setting = pFile->settings.items + pFile->settings.count++;

		strcpy(setting->name, name.c_str());
		if (strcmp(token, "float") == 0)
		{
			ParseFloats(token, &next_token, &setting->f, 1);
		}
		else if (strcmp(token, "v3") == 0)
		{
			ParseFloats(token, &next_token, &setting->f, 3);
		}
		else if (strcmp(token, "v4") == 0)
		{
			ParseFloats(token, &next_token, &setting->f, 4);
		}
		else if (strcmp(token, "u32") == 0)
		{
			ParseInts(token, &next_token, &setting->i, 1);
		}
		else assert(false);
	}

}

static ConfigFile LoadConfigFile(const char* pPath, MemoryStack* pStack)
{
	ConfigFile file = {};
	GetFullPath(pPath, file.path);
	file.def = {};

	file.last_write = CheckFileLastWrite(file.path);

	FILE* f = fopen(file.path, "r");
	if (f)
	{
		u32 lines = LinesInFile(f);
		file.settings.count = 0;
		file.settings.items = PushArray(pStack, ConfigSetting, lines);

		LoadConfigFileSettings(&file, f);

		fclose(f);
	}
	else 
	{
		LogError("Unable to locate config file %s", file.path);
	}

	return file;
}

static void CheckForConfigUpdates(ConfigFile* pFile)
{
#ifdef _DEBUG
	time_t last_write = CheckFileLastWrite(pFile->path);
	if (last_write > pFile->last_write)
	{
		FILE* f = fopen(pFile->path, "r");
		if (!f) return;

		pFile->last_write = last_write;
		pFile->settings.count = 0;
		LoadConfigFileSettings(pFile, f);

		fclose(f);

	}
#endif
}

static ConfigSetting* GetSetting(ConfigFile* pFile, const char* pSetting)
{
	for (u32 i = 0; i < pFile->settings.count; i++)
	{
		ConfigSetting* setting = pFile->settings.items + i;
		if (strcmp(pSetting, setting->name) == 0)
		{
			return setting;
		}
	}

	LogError("Unable to locate setting %s in config file", pSetting);
	return &pFile->def;
}