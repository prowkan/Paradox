#include "BoundingBoxComponent.h"

DEFINE_METACLASS_VARIABLE(BoundingBoxComponent)

void BoundingBoxComponent::InitComponentDefaultProperties()
{
	Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	HalfSize = XMFLOAT3(1.0f, 1.0f, 1.0f);
}