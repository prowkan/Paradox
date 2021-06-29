#pragma once

#include <Containers/DynamicArray.h>

class StaticMeshComponent;
class PointLightComponent;

class RenderScene
{
	public:

		void RegisterStaticMeshComponent(StaticMeshComponent* staticMeshComponent) { StaticMeshComponents.Add(staticMeshComponent); }
		void RegisterPointLightComponent(PointLightComponent* pointLightComponent) { PointLightComponents.Add(pointLightComponent); }

		DynamicArray<StaticMeshComponent*>& GetStaticMeshComponents() { return StaticMeshComponents; }
		DynamicArray<PointLightComponent*>& GetPointLightComponents() { return PointLightComponents; }

	private:

		DynamicArray<StaticMeshComponent*> StaticMeshComponents;
		DynamicArray<PointLightComponent*> PointLightComponents;
};