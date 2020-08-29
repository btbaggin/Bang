#pragma once

#include <map>
#include "string.h"
struct ConfigSetting
{
	char name[20];
	union
	{
		v3 V3;
		v4 V4;
		float f;
		u32 i;
	};
};

struct ConfigFile
{
	char path[MAX_PATH];
	time_t last_write;

	ConfigSetting def;
	List<ConfigSetting> settings;
};