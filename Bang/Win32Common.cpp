#define CONFIG_FILE_LOCATION ".\\bang.conf"

static void GetFullPath(const char* pPath, char* pBuffer)
{
	GetFullPathNameA(pPath, MAX_PATH, pBuffer, 0);
}

static void ProcessTaskCallbacks(TaskCallbackQueue* pQueue)
{
	u32 array = (u32)(pQueue->i >> 32);
	u64 newArray = ((u64)array ^ 1) << 32;

	u64 event = InterlockedExchange(&pQueue->i, newArray);
	u32 count = (u32)event;

	for (u32 i = 0; i < count; i++)
	{
		TaskCallback* cb = &pQueue->callbacks[array][i];
		cb->callback(cb->data);
	}
}

static void AddTaskCallback(TaskCallbackQueue* pQueue, TaskCompleteCallback* pFunc, void* pData)
{
	u64 arrayIndex = InterlockedIncrement(&pQueue->i);
	arrayIndex--;

	u32 array = (u32)(arrayIndex >> 32);
	u32 index = (u32)arrayIndex;
	assert(index < 32);

	TaskCallback cb;
	cb.callback = pFunc;
	cb.data = pData;
	pQueue->callbacks[array][index] = cb;
}


static bool QueueUserWorkItem(PlatformWorkQueue* pQueue, work_queue_callback* pCallback, void* pData)
{
	u32 newnext = (pQueue->NextEntryToWrite + 1) % QUEUE_ENTRIES;
	if (newnext == pQueue->NextEntryToRead) return false;

	WorkQueueEntry* entry = pQueue->entries + pQueue->NextEntryToWrite;
	entry->data = pData;
	entry->callback = pCallback;

	pQueue->CompletionTarget++;
	_WriteBarrier();
	_mm_sfence();

	pQueue->NextEntryToWrite = newnext;
	ReleaseSemaphore(pQueue->Semaphore, 1, 0);

	return true;
}

bool Win32DoNextWorkQueueEntry(PlatformWorkQueue* pQueue)
{
	u32 oldnext = pQueue->NextEntryToRead;
	u32 newnext = (oldnext + 1) % QUEUE_ENTRIES;
	if (oldnext != pQueue->NextEntryToWrite)
	{
		u32 index = InterlockedCompareExchange((LONG volatile*)&pQueue->NextEntryToRead, newnext, oldnext);
		if (index == oldnext)
		{
			WorkQueueEntry entry = pQueue->entries[index];
			entry.callback(pQueue, entry.data);

			InterlockedIncrement((LONG volatile*)&pQueue->CompletionCount);
		}
	}
	else
	{
		return true;
	}

	return false;
}

void Win32CompleteAllWork(PlatformWorkQueue* pQueue)
{
	while (pQueue->CompletionTarget != pQueue->CompletionCount)
	{
		Win32DoNextWorkQueueEntry(pQueue);
	}

	pQueue->CompletionCount = 0;
	pQueue->CompletionTarget = 0;
}

DWORD WINAPI ThreadProc(LPVOID pParameter)
{
	win32_thread_info* thread = (win32_thread_info*)pParameter;

	while (true)
	{
		if (Win32DoNextWorkQueueEntry(thread->queue))
		{
			WaitForSingleObjectEx(thread->queue->Semaphore, INFINITE, false);
		}
	}
}

static void InitWorkQueue(PlatformWorkQueue* pQueue, win32_thread_info* pThreads, GameState* pState)
{
	//Set up work queue
	pQueue->Semaphore = CreateSemaphoreEx(0, 0, THREAD_COUNT, 0, 0, SEMAPHORE_ALL_ACCESS);
	for (u32 i = 0; i < THREAD_COUNT; i++)
	{
		win32_thread_info* thread = pThreads + i;
		thread->ThreadIndex = i;
		thread->queue = pQueue;

		DWORD id;
		HANDLE t = CreateThread(0, 0, ThreadProc, thread, 0, &id);
		CloseHandle(t);
	}
	pState->work_queue = pQueue;
}

static void DisplayErrorMessage(const char* pError, ERROR_TYPE pType, ...)
{
	va_list args;
	va_start(args, pType);

	char buffer[1000];
	vsprintf(buffer, pError, args);

#ifdef _SERVER
	printf(buffer);
#else
	if (pType == ERROR_TYPE_Warning) DisplayModalWindow(&g_interface, new ErrorModal(buffer), MODAL_SIZE_Half);
	else MessageBoxA(g_state.form->handle, buffer, "Error", MB_OK);
#endif
	if (pType == ERROR_TYPE_Error)
	{
		g_state.is_running = false;
	}

	va_end(args);
}