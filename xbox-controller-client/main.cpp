#include <iostream>
#include "xcontroller.h"
#include <windows.h>
#include <stdio.h>
#include "tcpclient.h"

#define _SECOND 10000000
#define MAX_SEM_COUNT 10
#define FREQUENCY 2000

//struct controller_data CONTROLLER_DATA;
//struct controller_data * PCONTROLLER_DATA;

VOID CALLBACK TimerAPCProc(
	LPVOID lpArg, // Data value
	DWORD dwTimerLowValue, // Timer low value
	DWORD dwTimerHighValue // Timer high value
)
{
	UNREFERENCED_PARAMETER(dwTimerLowValue);
	UNREFERENCED_PARAMETER(dwTimerHighValue);

	struct controller_data *pController_Data = (controller_data*)lpArg;
	
	DWORD mutexWaitResult;
	mutexWaitResult = WaitForSingleObject(pController_Data->ghMutex, 0L);	
	if (pController_Data->controller_1->IsConnected()) {
		pController_Data->xSuccess = 1;
	}
	else
	{
		pController_Data->xSuccess = 0;
		std::cout << "XBOX Controller not connected...\n";
	}
	if (!ReleaseMutex(pController_Data->ghMutex))
	{
		printf("ReleaseMutex error timer: %d\n", GetLastError());
	}
}

DWORD WINAPI server_main(LPVOID lpParam) {
	struct controller_data* pController_Data = (controller_data*)lpParam;
	
	try
	{
		boost::asio::io_context io_context;

		server server(io_context, 2001, pController_Data);
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	return 0;
}

DWORD WINAPI xcontroller_main(LPVOID lpParam) {
	struct controller_data* pController_Data = (controller_data*)lpParam;
	DWORD mutexWaitResult;

	xcontroller* controller_1 = pController_Data->controller_1;
	
	while (true) {
		if (pController_Data->xSuccess != 0)
		{
			float LY = controller_1->GetState().Gamepad.sThumbRY;
			float LX = controller_1->GetState().Gamepad.sThumbRX;

			float magnitude = sqrt(LX * LX + LY * LY);

			float normalizedMagnitude = magnitude;

			if (normalizedMagnitude > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
				if (normalizedMagnitude > 32767) normalizedMagnitude = 32767;
				normalizedMagnitude -= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
				normalizedMagnitude = normalizedMagnitude / (32767 - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			}
			else {
				normalizedMagnitude = 0;
			}
			// critical section
			mutexWaitResult = WaitForSingleObject(pController_Data->ghMutex, INFINITE);
			switch (mutexWaitResult)
			{
			case WAIT_OBJECT_0:
				__try {
					// update connection success in data structure
					pController_Data->normalizedLX = LX / magnitude;
					pController_Data->normalizedLY = LY / magnitude;

					pController_Data->normalizedMagnitude = normalizedMagnitude;
				}
				__finally {
					if (!ReleaseMutex(pController_Data->ghMutex))
					{
						printf("ReleaseMutex error: %d\n", GetLastError());
					}
				}
				break;
			case WAIT_ABANDONED:
				return FALSE;
			}
			// end of critical section
		}
	}
	return 0;
}

HANDLE hTimer;

int main(int argc, char* argv[])
{
	using namespace std;

	// create the controller struct
	struct controller_data* pController_Data = new controller_data();
	pController_Data->controller_1 = new xcontroller(1);

	// create the semaphore
	pController_Data->ghSemaphore = CreateSemaphore(NULL, MAX_SEM_COUNT, MAX_SEM_COUNT, NULL);
	if (pController_Data->ghSemaphore == NULL)
	{
		printf("CreateSemaphore error: %d\n", GetLastError());
		return 1;
	}
	// create the mutex
	pController_Data->ghMutex = CreateMutex(NULL, FALSE, NULL);
	if (pController_Data->ghMutex == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		return 1;
	}

	__int64 qwDueTime;
	LARGE_INTEGER liDueTime;
	DWORD dwValue = 100;
	BOOL bSuccess;

	// xinput thread
	DWORD xinput_ThreadID;
	HANDLE xinput_Thread = CreateThread(0, 0, xcontroller_main, pController_Data, 0, &xinput_ThreadID);
	// server 
	DWORD server_ThreadID;
	HANDLE server_Thread = CreateThread(0, 0, server_main, pController_Data, 0, &server_ThreadID);

	pController_Data->hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	if (pController_Data->hTimer != NULL)
	{
		try
		{
			qwDueTime = -2 * _SECOND;
			liDueTime.LowPart = (DWORD)(qwDueTime & 0xFFFFFFFF);
			liDueTime.HighPart = (LONG)(qwDueTime >> 32);

			bSuccess = SetWaitableTimer(pController_Data->hTimer, &liDueTime, FREQUENCY, TimerAPCProc, pController_Data, FALSE);
			if (bSuccess) {
				for (;;)
				{
					SleepEx(
						INFINITE,
						TRUE
					);
				}
			}
			else {
				printf("SetWaitableTimer failed with error %d\n", GetLastError());
				throw(1);
			}
		}
		catch (int error_num)
		{
			CloseHandle(pController_Data->hTimer);
		}
	}
	else {
		printf("CreatteWaitableTimer failed with error %d\n", GetLastError());
	}
	
	CloseHandle(xinput_Thread);

	CloseHandle(pController_Data->ghSemaphore);

	return 0;
}