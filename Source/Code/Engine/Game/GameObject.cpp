#include "GameObject.h"

#include "MetaClass.h"
#include "Component.h"

MetaClass *GameObject::GameObjectMetaClass;

Component* GameObject::CreateDefaultComponent(MetaClass* metaClass)
{
	void *componentPtr = malloc(metaClass->GetClassSize());
	metaClass->ObjectConstructorFunc(componentPtr);
	Component *component = (Component*)componentPtr;
	component->SetMetaClass(metaClass);
	component->SetOwner(this);
	component->InitComponentDefaultProperties();
	component->RegisterComponent();
	Components.push_back(component);
	return component;
}
