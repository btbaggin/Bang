#pragma once
struct PlatformWorkQueue;
struct PlatformWindow;
struct GameState;
struct Platform;
struct PlatformInputMessage;
enum ERROR_TYPE;

#define WORK_QUEUE_CALLBACK(name)void name(PlatformWorkQueue* pQueue, void* pData)
typedef WORK_QUEUE_CALLBACK(work_queue_callback);

static bool QueueUserWorkItem(PlatformWorkQueue* pQueue, work_queue_callback* pCallback, void* pData);

static void GetFullPath(const char* pPath, char* pBuffer);

static void DisplayErrorMessage(const char* pError, ERROR_TYPE pType, ...);

static void SwapBuffers(PlatformWindow* pWindow);

#define MAX_PATH 260 //Is this smart?
