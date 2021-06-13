// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "BoundingBoxComponent.h"

#include <FileSystem/LevelFile.h>

DEFINE_METACLASS_VARIABLE(BoundingBoxComponent)

void BoundingBoxComponent::InitComponentDefaultProperties()
{
	Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	HalfSize = XMFLOAT3(1.0f, 1.0f, 1.0f);
}

void BoundingBoxComponent::LoadFromFile(LevelFile& File)
{
	File.Read(Center);
	File.Read(HalfSize);
}