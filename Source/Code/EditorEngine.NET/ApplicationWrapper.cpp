// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <../Engine/Core/Application.h>
#include <../Engine/Engine/Engine.h>
#include <../Engine/Game/Entity.h>
#include <../Engine/Game/Component.h>

extern "C" __declspec(dllexport) void StartApplication()
{
	Application::EditorStartApplication();
}

extern "C" __declspec(dllexport) void StopApplication()
{
	Application::EditorStopApplication();
}

extern "C" __declspec(dllexport) void RunMainLoop()
{
	Application::EditorRunMainLoop();
}

extern "C" __declspec(dllexport) void SetLevelRenderCanvasHandle(HWND LevelRenderCanvasHandle)
{
	Application::SetLevelRenderCanvasHandle(LevelRenderCanvasHandle);
}

extern "C" __declspec(dllexport) void SetAppExitFlag(bool Value)
{
	Application::SetAppExitFlag(Value);
}

extern "C" __declspec(dllexport) void SetEditorViewportSize(UINT Width, UINT Height)
{
	//Engine::GetEngine().GetRenderSystem().SetEditorViewportSize(Width, Height);
	Application::EditorViewportWidth = Width;
	Application::EditorViewportHeight = Height;
}

extern "C" __declspec(dllexport) void RotateCamera(int MouseDeltaX, int MouseDeltaY)
{
	Camera& camera = Engine::GetEngine().GetGameFramework().GetCamera();
	XMFLOAT3 CameraRotation = camera.GetCameraRotation();
	CameraRotation.x += MouseDeltaY * 0.01f;
	CameraRotation.y += MouseDeltaX * 0.01f;
	camera.SetCameraRotation(CameraRotation);
}

extern "C" __declspec(dllexport) void MoveCamera(bool bForward, bool bBackward, bool bLeft, bool bRight)
{
	Camera& camera = Engine::GetEngine().GetGameFramework().GetCamera();
	XMFLOAT3 CameraLocation = camera.GetCameraLocation();
	XMFLOAT3 CameraRotation = camera.GetCameraRotation();

	XMFLOAT3 CameraOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (bForward) CameraOffset.z += 0.1f;
	if (bBackward) CameraOffset.z -= 0.1f;
	if (bRight) CameraOffset.x += 0.1f;
	if (bLeft) CameraOffset.x -= 0.1f;

	XMVECTOR CameraOffsetVector = XMLoadFloat3(&CameraOffset);

	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(CameraRotation.x, CameraRotation.y, CameraRotation.z);

	CameraOffsetVector = XMVector4Transform(CameraOffsetVector, RotationMatrix);

	XMStoreFloat3(&CameraOffset, CameraOffsetVector);

	CameraLocation.x += CameraOffset.x;
	CameraLocation.y += CameraOffset.y;
	CameraLocation.z += CameraOffset.z;

	camera.SetCameraLocation(CameraLocation);
}

extern "C" __declspec(dllexport) const char* GetEntityClassName(const char* EntityName)
{
	return Engine::GetEngine().GetGameFramework().GetWorld().FindEntityByName(EntityName)->GetMetaClass()->GetClassName();
}

extern "C" __declspec(dllexport) uint32_t GetEntityPropertiesCount(const char* EntityName)
{
	return Engine::GetEngine().GetGameFramework().GetWorld().FindEntityByName(EntityName)->GetPropertiesCount();
}

extern "C" __declspec(dllexport) const char* GetEntityPropertyName(const char* EntityName, uint32_t PropertyIndex)
{
	return Engine::GetEngine().GetGameFramework().GetWorld().FindEntityByName(EntityName)->GetPropertyName(PropertyIndex);
}

extern "C" __declspec(dllexport) ClassPropertyType GetEntityPropertyType(const char* EntityName, uint32_t PropertyIndex)
{
	return Engine::GetEngine().GetGameFramework().GetWorld().FindEntityByName(EntityName)->GetPropertyType(PropertyIndex);
}

extern "C" __declspec(dllexport) void* GetEntityComponentReferenceProperty(const char* EntityName, const char* PropertyName)
{
	return Engine::GetEngine().GetGameFramework().GetWorld().FindEntityByName(EntityName)->GetComponentReferenceProperty(PropertyName);
}

extern "C" __declspec(dllexport) const char* GetComponentClassName(Component* component)
{
	return component->GetMetaClass()->GetClassName();
}

extern "C" __declspec(dllexport) uint32_t GetComponentPropertiesCount(Component* component)
{
	return component->GetPropertiesCount();
}

extern "C" __declspec(dllexport) const char* GetComponentPropertyName(Component* component, uint32_t PropertyIndex)
{
	return component->GetPropertyName(PropertyIndex);
}

extern "C" __declspec(dllexport) ClassPropertyType GetComponentPropertyType(Component* component, uint32_t PropertyIndex)
{
	return component->GetPropertyType(PropertyIndex);
}

extern "C"
{
	struct Vector3
	{
		float X, Y, Z;
	};
}

extern "C" __declspec(dllexport) Vector3 GetComponentVectorProperty(Component* component, const char* PropertyName)
{
	XMFLOAT3 Value = component->GetVectorProperty(PropertyName);

	Vector3 Vector;
	Vector.X = Value.x;
	Vector.Y = Value.y;
	Vector.Z = Value.z;

	return Vector;
}

extern "C" __declspec(dllexport) Vector3 GetComponentRotatorProperty(Component* component, const char* PropertyName)
{
	XMFLOAT3 Value = component->GetRotatorProperty(PropertyName);

	Vector3 Vector;
	Vector.X = Value.x;
	Vector.Y = Value.y;
	Vector.Z = Value.z;

	return Vector;
}

extern "C" __declspec(dllexport) Vector3 GetComponentColorProperty(Component* component, const char* PropertyName)
{
	XMFLOAT3 Value = component->GetColorProperty(PropertyName);

	Vector3 Vector;
	Vector.X = Value.x;
	Vector.Y = Value.y;
	Vector.Z = Value.z;

	return Vector;
}

extern "C" __declspec(dllexport) float GetComponentFloatProperty(Component* component, const char* PropertyName)
{
	return component->GetFloatProperty(PropertyName);
}

extern "C" __declspec(dllexport) void* GetComponentResourceReferenceProperty(Component* component, const char* PropertyName)
{
	return component->GetResourceReferenceProperty(PropertyName);
}

extern "C" __declspec(dllexport) const char* GetResourceName(Resource* resource)
{
	return resource->GetResourceName().c_str();
}