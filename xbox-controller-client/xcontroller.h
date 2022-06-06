#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <XInput.h>
#include <nlohmann/json.hpp>

#pragma comment(lib, "XInput.lib")

class xcontroller
{
private:
	XINPUT_STATE _controllerState;
	int _controllerNum;
public:
	int vibration_val;
	xcontroller(int playernumber);
	XINPUT_STATE GetState();
	bool IsConnected();
	void Vibrate(int leftVal = 0, int rightVal = 0);
};

struct controller_data {
	// xinput object
	xcontroller* controller_1;
	// stored and filtered values
	float normalizedLX;
	float normalizedLY;
	float normalizedMagnitude;
	// timers
	HANDLE hTimer;
	// semaphore
	HANDLE ghSemaphore;
	// mutex
	HANDLE ghMutex;
	// success
	int xSuccess;
	// json serialization information
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(controller_data, normalizedLX, normalizedLY, normalizedMagnitude);
};