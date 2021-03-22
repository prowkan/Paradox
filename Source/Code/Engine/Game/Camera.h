#pragma once

class Camera
{
	public:

		void InitCamera();
		void ShutdownCamera();
		void TickCamera(float DeltaTime);

		XMMATRIX GetViewMatrix() { return ViewMatrix; }
		XMMATRIX GetProjMatrix() { return ProjMatrix; }
		XMMATRIX GetViewProjMatrix() { return ViewProjMatrix; }

		XMFLOAT3 GetCameraLocation() { return CameraLocation; }
		void SetCameraLocation(const XMFLOAT3& NewCameraLocation) { CameraLocation = NewCameraLocation; }

		XMFLOAT3 GetCameraRotation() { return CameraRotation; }
		void SetCameraRotation(const XMFLOAT3& NewCameraRotation) { CameraRotation = NewCameraRotation; }

		void SetFOV(const float NewFOV) { FOV = NewFOV; }
		void SetAspectRatio(const float NewAspectRatio) { AspectRatio = NewAspectRatio; }

	private:

		XMFLOAT3 CameraLocation;
		XMFLOAT3 CameraRotation;

		XMMATRIX ViewMatrix;
		XMMATRIX ProjMatrix;
		XMMATRIX ViewProjMatrix;

		float FOV;
		float AspectRatio;
};