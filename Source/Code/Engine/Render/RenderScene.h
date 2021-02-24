#pragma once

#include <Containers/DynamicArray.h>

class StaticMeshComponent;

class RenderScene
{
	public:

		void RegisterStaticMeshComponent(StaticMeshComponent* staticMeshComponent) { StaticMeshComponents.Add(staticMeshComponent); }

		DynamicArray<StaticMeshComponent*>& GetStaticMeshComponents() { return StaticMeshComponents; }

	private:

		DynamicArray<StaticMeshComponent*> StaticMeshComponents;
};