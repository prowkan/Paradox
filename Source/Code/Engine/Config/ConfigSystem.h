#pragma once

class ConfigSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();

		string GetRenderConfigValueString(const string& Section, const string& Param);
		int GetRenderConfigValueInt(const string& Section, const string& Param);

	private:

		map<string, map<string, string>> RenderConfig;
};