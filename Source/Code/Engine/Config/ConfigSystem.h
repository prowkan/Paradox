#pragma once

class ConfigSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();

	private:

		map<string, map<string, string>> RenderConfig;
};