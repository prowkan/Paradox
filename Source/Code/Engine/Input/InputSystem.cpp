// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "InputSystem.h"

#include <Core/Application.h>
#include <Engine/Engine.h>

void InputSystem::InitSystem()
{
	RECT WindowRect;
	BOOL Result = GetWindowRect(Application::GetMainWindowHandle(), &WindowRect);
	POINT WindowCenter;
	WindowCenter.x = (WindowRect.left + WindowRect.right) / 2;
	WindowCenter.y = (WindowRect.top + WindowRect.bottom) / 2;
	Result = SetCursorPos(WindowCenter.x, WindowCenter.y);
	//PreviousCursorPosition.x = WindowCenter.x;
	//PreviousCursorPosition.y = WindowCenter.y;

	//Result = ClipCursor(&WindowRect);
	//ShowCursor(FALSE);

	RAWINPUTDEVICE RawInputDevice;
	RawInputDevice.dwFlags = 0;
	RawInputDevice.hwndTarget = NULL;
	RawInputDevice.usUsage = 0x0002;
	RawInputDevice.usUsagePage = 0x0001;

	Result = RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RAWINPUTDEVICE));
}

void InputSystem::ShutdownSystem()
{

}

void InputSystem::TickSystem(float DeltaTime)
{
	if (GetForegroundWindow() != Application::GetMainWindowHandle()) return;

	Camera& camera = Engine::GetEngine().GetGameFramework().GetCamera();

	XMFLOAT3 CameraLocation = camera.GetCameraLocation();
	XMFLOAT3 CameraRotation = camera.GetCameraRotation();

	if (GetAsyncKeyState(VK_UP) & 0x8000) CameraRotation.x -= 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) CameraRotation.x += 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) CameraRotation.y -= 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) CameraRotation.y += 1.0f * DeltaTime;

	//BOOL Result = GetCursorPos(&CurrentCursorPosition);

	float MouseSensetivity = 0.25f;

	//CameraRotation.x += MouseSensetivity * (CurrentCursorPosition.y - PreviousCursorPosition.y) * DeltaTime;
	CameraRotation.x += MouseSensetivity * MouseDeltaY * DeltaTime;
	//CameraRotation.y += MouseSensetivity * (CurrentCursorPosition.x - PreviousCursorPosition.x) * DeltaTime;
	CameraRotation.y += MouseSensetivity * MouseDeltaX * DeltaTime;

	/*RECT WindowRect;
	Result = GetWindowRect(Application::GetMainWindowHandle(), &WindowRect);
	POINT WindowCenter;
	WindowCenter.x = (WindowRect.left + WindowRect.right) / 2;
	WindowCenter.y = (WindowRect.top + WindowRect.bottom) / 2;
	Result = SetCursorPos(WindowCenter.x, WindowCenter.y);
	PreviousCursorPosition.x = WindowCenter.x;
	PreviousCursorPosition.y = WindowCenter.y;*/

	XMFLOAT3 CameraOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);

	float Acceleration = ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_RSHIFT) & 0x8000)) ? 5.0f : 1.0f;

	if (GetAsyncKeyState('S') & 0x8000) CameraOffset.z -= 2.5f * Acceleration * DeltaTime;
	if (GetAsyncKeyState('W') & 0x8000) CameraOffset.z += 2.5f * Acceleration * DeltaTime;
	if (GetAsyncKeyState('A') & 0x8000) CameraOffset.x -= 2.5f * Acceleration * DeltaTime;
	if (GetAsyncKeyState('D') & 0x8000) CameraOffset.x += 2.5f * Acceleration * DeltaTime;

	XMVECTOR CameraOffsetVector = XMLoadFloat3(&CameraOffset);

	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(CameraRotation.x, CameraRotation.y, CameraRotation.z);

	CameraOffsetVector = XMVector4Transform(CameraOffsetVector, RotationMatrix);

	XMStoreFloat3(&CameraOffset, CameraOffsetVector);

	CameraLocation.x += CameraOffset.x;
	CameraLocation.y += CameraOffset.y;
	CameraLocation.z += CameraOffset.z;

	camera.SetCameraLocation(CameraLocation);
	camera.SetCameraRotation(CameraRotation);

	//MouseDeltaX = 0;
	//MouseDeltaY = 0;
}

void InputSystem::ProcessRawMouseInput(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	UINT RawInputDataSize;

	UINT Result = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &RawInputDataSize, sizeof(RAWINPUTHEADER));

	BYTE *RawInputData = new BYTE[RawInputDataSize];

	Result = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, RawInputData, &RawInputDataSize, sizeof(RAWINPUTHEADER));

	RAWINPUT& RawInput = *(RAWINPUT*)RawInputData;

	if (RawInput.header.dwType == RIM_TYPEMOUSE)
	{
		cout << RawInput.data.mouse.lLastX << " " << RawInput.data.mouse.lLastY << endl;

		MouseDeltaX = RawInput.data.mouse.lLastX;
		MouseDeltaY = RawInput.data.mouse.lLastY;
	}	

	delete[] RawInputData;
}