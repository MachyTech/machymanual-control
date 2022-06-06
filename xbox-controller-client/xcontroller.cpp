#include "xcontroller.h"

xcontroller::xcontroller(int playerNumber)
{
	// Set the Controller Number
	_controllerNum = playerNumber - 1;
	vibration_val = 32767;
}

XINPUT_STATE xcontroller::GetState()
{
	ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));

	XInputGetState(_controllerNum, &_controllerState);

	return _controllerState;
}

bool xcontroller::IsConnected()
{
	ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));
	//DWORD XInputGetState([in] DWORD dwUserIndex, [out] XINPUT_STATE* pState);
	DWORD Result = XInputGetState(_controllerNum, &_controllerState);

	if (Result == ERROR_SUCCESS)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void xcontroller::Vibrate(int leftVal, int rightVal)
{
	XINPUT_VIBRATION Vibration;

	ZeroMemory(&Vibration, sizeof(XINPUT_VIBRATION));

	// Set the Vibration values
	// max is 65535
	Vibration.wLeftMotorSpeed = leftVal;
	Vibration.wRightMotorSpeed = rightVal;

	// Vibrate the controller
	XInputSetState(_controllerNum, &Vibration);
}