#include "Component.h"

#include "MetaClass.h"

MetaClass *ComponentMetaClass = new MetaClass(&CallObjectConstructor<Component>, sizeof(Component), "Component");