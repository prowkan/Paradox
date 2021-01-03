#pragma once

class StaticMeshComponent;

class RenderScene
{
	public:

		void RegisterStaticMeshComponent(StaticMeshComponent* staticMeshComponent) { StaticMeshComponents.push_back(staticMeshComponent); }

		vector<StaticMeshComponent*>& GetStaticMeshComponents() { return StaticMeshComponents; }

	private:

		vector<StaticMeshComponent*> StaticMeshComponents;
};