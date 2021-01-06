#include "TransformComponent.h"

#include "../../MetaClass.h"

MetaClass *TransformComponent::TransformComponentMetaClass;

void TransformComponent::InitComponentDefaultProperties()
{
	Location = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
	PivotPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);
}