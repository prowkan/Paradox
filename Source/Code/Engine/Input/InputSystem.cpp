#include "InputSystem.h"

#include <Engine/Engine.h>

void InputSystem::InitSystem()
{

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

	XMFLOAT3 CameraOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (GetAsyncKeyState('S') & 0x8000) CameraOffset.z -= 2.5f * DeltaTime;
	if (GetAsyncKeyState('W') & 0x8000) CameraOffset.z += 2.5f * DeltaTime;
	if (GetAsyncKeyState('A') & 0x8000) CameraOffset.x -= 2.5f * DeltaTime;
	if (GetAsyncKeyState('D') & 0x8000) CameraOffset.x += 2.5f * DeltaTime;

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