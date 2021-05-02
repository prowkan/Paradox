// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "TransformComponent.h"

#include <FileSystem/LevelFile.h>

DEFINE_METACLASS_VARIABLE(TransformComponent)

void TransformComponent::InitComponentDefaultProperties()
{
	Location = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
	PivotPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void TransformComponent::LoadFromFile(LevelFile& File)
{
	File.Read(Location);
	File.Read(Rotation);
	File.Read(Scale);
	File.Read(PivotPoint);
}
