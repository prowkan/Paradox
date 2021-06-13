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
		RECT WindowRect;
		BOOL Result = GetWindowRect(Application::GetMainWindowHandle(), &WindowRect);
		POINT WindowCenter;
		WindowCenter.x = (WindowRect.left + WindowRect.right) / 2;
		WindowCenter.y = (WindowRect.top + WindowRect.bottom) / 2;
		Result = SetCursorPos(WindowCenter.x, WindowCenter.y);
		PreviousCursorPosition.x = WindowCenter.x;
		PreviousCursorPosition.y = WindowCenter.y;
	}

	for (int i = 0; i < 255; i++)
	{
		KeyStates[i] = KeyState::Released;
	}

	OnPressedKeyBindings[VK_F2] = Delegate<void>(this, &InputSystem::ToggleOcclusionBuffer);
	OnPressedKeyBindings[VK_F3] = Delegate<void>(this, &InputSystem::ToggleBoundingBoxes);
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

	Camera& camera = Engine::GetEngine().GetGameFramework().GetCamera();

	XMFLOAT3 CameraLocation = camera.GetCameraLocation();
	XMFLOAT3 CameraRotation = camera.GetCameraRotation();

	if (GetAsyncKeyState(VK_UP) & 0x8000) CameraRotation.x -= 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) CameraRotation.x += 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) CameraRotation.y -= 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) CameraRotation.y += 1.0f * DeltaTime;

	BOOL Result = GetCursorPos(&CurrentCursorPosition);

	float MouseSensetivity = 0.25f;

	CameraRotation.x += MouseSensetivity * (CurrentCursorPosition.y - PreviousCursorPosition.y) * DeltaTime;
	CameraRotation.y += MouseSensetivity * (CurrentCursorPosition.x - PreviousCursorPosition.x) * DeltaTime;

#if WITH_EDITOR
	if (!Application::IsEditor())
#endif
	{
		RECT WindowRect;
		Result = GetWindowRect(Application::GetMainWindowHandle(), &WindowRect);
		POINT WindowCenter;
		WindowCenter.x = (WindowRect.left + WindowRect.right) / 2;
		WindowCenter.y = (WindowRect.top + WindowRect.bottom) / 2;
		Result = SetCursorPos(WindowCenter.x, WindowCenter.y);
		PreviousCursorPosition.x = WindowCenter.x;
		PreviousCursorPosition.y = WindowCenter.y;
	}

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
}