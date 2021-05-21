#pragma once

#include <Containers/HashTable.h>

class ConfigSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();

		String GetRenderConfigValueString(const String& Section, const String& Param);
		int GetRenderConfigValueInt(const String& Section, const String& Param);

	private:

	HashTable<String, HashTable<String, String>> RenderConfig;
};