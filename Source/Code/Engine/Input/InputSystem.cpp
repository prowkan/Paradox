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
	PreviousCursorPosition.x = WindowCenter.x;
	PreviousCursorPosition.y = WindowCenter.y;
}

void InputSystem::ShutdownSystem()
{

}

void InputSystem::TickSystem(float DeltaTime)
{
	XMFLOAT3 CameraLocation = Engine::GetEngine().GetGameFramework().GetCamera().GetCameraLocation();
	XMFLOAT3 CameraRotation = Engine::GetEngine().GetGameFramework().GetCamera().GetCameraRotation();

	if (GetAsyncKeyState(VK_UP) & 0x8000) CameraRotation.x -= 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) CameraRotation.x += 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) CameraRotation.y -= 1.0f * DeltaTime;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) CameraRotation.y += 1.0f * DeltaTime;

	BOOL Result = GetCursorPos(&CurrentCursorPosition);

	float MouseSensetivity = 1.0f;

	CameraRotation.x += MouseSensetivity * (CurrentCursorPosition.y - PreviousCursorPosition.y) * DeltaTime;
	CameraRotation.y += MouseSensetivity * (CurrentCursorPosition.x - PreviousCursorPosition.x) * DeltaTime;

	RECT WindowRect;
	Result = GetWindowRect(Application::GetMainWindowHandle(), &WindowRect);
	POINT WindowCenter;
	WindowCenter.x = (WindowRect.left + WindowRect.right) / 2;
	WindowCenter.y = (WindowRect.top + WindowRect.bottom) / 2;
	Result = SetCursorPos(WindowCenter.x, WindowCenter.y);
	PreviousCursorPosition.x = WindowCenter.x;
	PreviousCursorPosition.y = WindowCenter.y;

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

	Engine::GetEngine().GetGameFramework().GetCamera().SetCameraLocation(CameraLocation);
	Engine::GetEngine().GetGameFramework().GetCamera().SetCameraRotation(CameraRotation);
}