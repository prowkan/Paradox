// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Camera.h"

void Camera::InitCamera()
{
	CameraLocation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	CameraRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void Camera::ShutdownCamera()
{

}

void Camera::TickCamera(float DeltaTime)
{
	XMVECTOR ForwardVector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR UpVector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(CameraRotation.x, CameraRotation.y, CameraRotation.z);

	ForwardVector = XMVector4Transform(ForwardVector, RotationMatrix);
	UpVector = XMVector4Transform(UpVector, RotationMatrix);

	ViewProjMatrix = XMMatrixLookToLH(XMVectorSet(CameraLocation.x, CameraLocation.y, CameraLocation.z, 1.0f), ForwardVector, UpVector) * XMMatrixPerspectiveFovLH(3.14f / 2.0f, 16.0f / 9.0f, 0.01f, 1000.0f);
}