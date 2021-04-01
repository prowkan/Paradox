// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "TransformComponent.h"

DEFINE_METACLASS_VARIABLE(TransformComponent)

void TransformComponent::InitComponentDefaultProperties()
{
	Location = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
	PivotPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void TransformComponent::LoadFromFile(HANDLE File)
{
	BOOL Result;
	Result = ReadFile(File, &Location, sizeof(XMFLOAT3), NULL, NULL);
	Result = ReadFile(File, &Rotation, sizeof(XMFLOAT3), NULL, NULL);
	Result = ReadFile(File, &Scale, sizeof(XMFLOAT3), NULL, NULL);
	Result = ReadFile(File, &PivotPoint, sizeof(XMFLOAT3), NULL, NULL);
}
