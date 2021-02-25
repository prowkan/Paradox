#pragma once

class StaticMeshComponent;
class PointLightComponent;

class RenderScene
{
	public:

		void RegisterStaticMeshComponent(StaticMeshComponent* staticMeshComponent) { StaticMeshComponents.push_back(staticMeshComponent); }
		void RegisterPointLightComponent(PointLightComponent* pointLightComponent) { PointLightComponents.push_back(pointLightComponent); }

		vector<StaticMeshComponent*>& GetStaticMeshComponents() { return StaticMeshComponents; }
		vector<PointLightComponent*>& GetPointLightComponents() { return PointLightComponents; }

	private:

		vector<StaticMeshComponent*> StaticMeshComponents;
		vector<PointLightComponent*> PointLightComponents;
};