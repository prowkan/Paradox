#include "GameObject.h"

#include "MetaClass.h"
#include "Component.h"

MetaClass *GameObjectMetaClass = new MetaClass(&CallObjectConstructor<GameObject>, sizeof(GameObject));

Component* GameObject::CreateDefaultComponent(MetaClass* metaClass)
{
	void *componentPtr = malloc(metaClass->GetClassSize());
	metaClass->ObjectConstructorFunc(componentPtr);
	Component *component = (Component*)componentPtr;
	component->InitComponentDefaultProperties();
	Components.push_back(component);
	return component;
}
