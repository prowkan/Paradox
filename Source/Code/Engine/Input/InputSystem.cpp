// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "InputSystem.h"

#include <Core/Application.h>
#include <Engine/Engine.h>

void InputSystem::InitSystem()
{
#if WITH_EDITOR
	if (!Application::IsEditor())
#endif
	{
		RAWINPUTDEVICE RawInputDevice;
		RawInputDevice.dwFlags = 0;
		RawInputDevice.hwndTarget = NULL;
		RawInputDevice.usUsage = 0x0002;
		RawInputDevice.usUsagePage = 0x0001;

		BOOL Result = RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RAWINPUTDEVICE));
	}

	for (int i = 0; i < 255; i++)
	{
		KeyStates[i] = KeyState::Released;
	}

	OnPressedKeyBindings[VK_F2] = Delegate<void>(this, &InputSystem::ToggleOcclusionBuffer);
	OnPressedKeyBindings[VK_F3] = Delegate<void>(this, &InputSystem::ToggleBoundingBoxes);
	
	OnReleasedKeyBindings[VK_ESCAPE] = Delegate<void>(this, &InputSystem::EscapeKeyHandler);

	IsMouseCaptured = false;
}

void InputSystem::ShutdownSystem()
{

}

void InputSystem::ToggleOcclusionBuffer()
{
	Engine::GetEngine().GetRenderSystem().ToggleOcclusionBuffer();
}

void InputSystem::ToggleBoundingBoxes()
{
	Engine::GetEngine().GetRenderSystem().ToggleBoundingBoxes();
}

void InputSystem::EscapeKeyHandler()
{
	if (IsMouseCaptured)
	{
		IsMouseCaptured = false;
		int Result = ShowCursor(TRUE);
	}
	else
	{
		Application::SetAppExitFlag(true);
	}
}

void InputSystem::TickSystem(float DeltaTime)
{
	if (GetForegroundWindow() != Application::GetMainWindowHandle()) return;

	bool WasKeyPressed[255];
	bool WasKeyReleased[255];

	for (int i = 0; i < 255; i++)
	{
		WasKeyPressed[i] = false;
		WasKeyReleased[i] = false;
	}

	for (int i = 0; i < 255; i++)
	{
		if (GetAsyncKeyState(i) & 0x8000)
		{
			if (KeyStates[i] == KeyState::Released)
			{
				WasKeyPressed[i] = true;
				KeyStates[i] = KeyState::Pressed;
			}
		}
		else
		{
			if (KeyStates[i] == KeyState::Pressed)
			{
				WasKeyReleased[i] = true;
				KeyStates[i] = KeyState::Released;
			}
		}
	}

	for (int i = 0; i < 255; i++)
	{
		if (WasKeyPressed[i] && OnPressedKeyBindings[i] != nullptr)
		{
			OnPressedKeyBindings[i]();
		}

		if (WasKeyReleased[i] && OnReleasedKeyBindings[i] != nullptr)
		{
			OnReleasedKeyBindings[i]();
		}
	}

	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
	{
		if (!IsMouseCaptured)
		{
			IsMouseCaptured = true;
			int Result = ShowCursor(FALSE);

			RECT MainWindowRect;
			BOOL bResult = GetWindowRect(Application::GetMainWindowHandle(), &MainWindowRect);
			bResult = ClipCursor(&MainWindowRect);
		}
	}	

	Camera& camera = Engine::GetEngine().GetGameFramework().GetCamera();

	XMFLOAT3 CameraLocation = camera.GetCameraLocation();
	XMFLOAT3 CameraRotation = camera.GetCameraRotation();

	if (GetAsyncKeyState(VK_UP) & 0x8000) CameraRotation.x -= 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) CameraRotation.x += 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) CameraRotation.y -= 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) CameraRotation.y += 1.0f * DeltaTime;

	float MouseSensetivity = 0.25f;

	CameraRotation.x += MouseSensetivity * MouseDeltaY * DeltaTime;
	CameraRotation.y += MouseSensetivity * MouseDeltaX * DeltaTime;

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

	MouseDeltaX = 0;
	MouseDeltaY = 0;
}

void InputSystem::ProcessRawMouseInput(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (!IsMouseCaptured) return;

	UINT RawInputDataSize;

	UINT Result = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &RawInputDataSize, sizeof(RAWINPUTHEADER));

	BYTE *RawInputData = new BYTE[RawInputDataSize];

	Result = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, RawInputData, &RawInputDataSize, sizeof(RAWINPUTHEADER));

	RAWINPUT& RawInput = *(RAWINPUT*)RawInputData;

	if (RawInput.header.dwType == RIM_TYPEMOUSE)
	{
		MouseDeltaX += RawInput.data.mouse.lLastX;
		MouseDeltaY += RawInput.data.mouse.lLastY;
	}

	delete[] RawInputData;
}