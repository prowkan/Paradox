// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Entity.h"

#include "MetaClass.h"
#include "Component.h"

#include <Engine/Engine.h>

#include <MemoryManager/SystemMemoryAllocator.h>

DEFINE_METACLASS_VARIABLE(Entity)

Component* Entity::CreateDefaultComponent(MetaClass* metaClass)
{
	//void *componentPtr = Engine::GetEngine().GetMemoryManager().AllocateComponent(metaClass);
	void *componentPtr = SystemMemoryAllocator::AllocateMemory(metaClass->GetClassSize());
	metaClass->ObjectConstructorFunc(componentPtr);
	Component *component = (Component*)componentPtr;
	component->SetMetaClass(metaClass);
	component->SetOwner(this);
	component->InitComponentDefaultProperties();
	component->RegisterComponent();
	Components.Add(component);
	return component;
}
