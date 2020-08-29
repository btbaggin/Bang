#define DEBUG_LEVEL_None 0
#define DEBUG_LEVEL_Error 1
#define DEBUG_LEVEL_All 2

//Modify this define to control the level of logging
#define DEBUG_LEVEL DEBUG_LEVEL_All

#if DEBUG_LEVEL > DEBUG_LEVEL_Error
#define LogInfo Log
#else
#define LogInfo(message, ...)
#endif

#if DEBUG_LEVEL != DEBUG_LEVEL_None
#include <stdarg.h>
#include <cctype>
#include <locale>
#define LogError Log
#define InitializeLogger() InitLog()
#define CloseLogger() 
#else
#define LogError(message, ...)
#define InitializeLogger()
#define CloseLogger()
#endif


static void GetTime(char* pBuffer, u32 pBufferSize, const char* pFormat = "%I:%M%p")
{
	time_t t;
	struct tm* timeinfo;
	time(&t);
	timeinfo = localtime(&t);
	strftime(pBuffer, pBufferSize, pFormat, timeinfo);
}

#ifdef _SERVER
static void Log(const char* pMessage, ...)
{
	va_list args;
	va_start(args, pMessage);

	char buffer[1000];
	vsprintf(buffer, pMessage, args);

	char time[100];
	GetTime(time, 100, "{%x %X} ");

	printf(time);
	printf(buffer);
	printf("\n");

	va_end(args);
}

static void InitLog() { }

static void CloseLog() { }
#else
FILE* log_handle = nullptr;
static void Log(const char* pMessage, ...)
{
	va_list args;
	va_start(args, pMessage);

	char buffer[1000];
	vsprintf(buffer, pMessage, args);

	char time[100];
	GetTime(time, 100, "{%x %X} ");
	fputs(time, log_handle);

	fputs(buffer, log_handle);
	fputc('\n', log_handle);

	fflush(log_handle);

	va_end(args);
}

static void InitLog()
{
	log_handle = fopen("log.txt", "w");
}

static void CloseLog()
{
	fclose(log_handle);
}
#endif