// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ConfigSystem.h"

string Trim(const string& InputString)
{
	string OutputString = InputString;

	size_t i = 0;

	while (true)
	{
		if (OutputString[i] == ' ')
		{
			OutputString = OutputString.substr(1);
			i++;
		}
		else break;
	}

	i = OutputString.length();

	while (true)
	{
		if (OutputString[i] == ' ')
		{
			OutputString = OutputString.substr(0, OutputString.length() - 1);
			i--;
		}
		else break;
	}

	return OutputString;
}

void ConfigSystem::InitSystem()
{
	HANDLE RenderConfigFileHandle = CreateFile((const wchar_t*)u"Config/Render.cfg", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER RenderConfigFileSize;
	BOOL Result = GetFileSizeEx(RenderConfigFileHandle, &RenderConfigFileSize);
	BYTE *RenderConfigFileData = (BYTE*)malloc(RenderConfigFileSize.QuadPart);
	Result = ReadFile(RenderConfigFileHandle, RenderConfigFileData, (DWORD)RenderConfigFileSize.QuadPart, NULL, NULL);
	Result = CloseHandle(RenderConfigFileHandle);

	size_t FilePointer = 0;

	vector<string> ConfigFileLines;

	while (true)
	{
		if (FilePointer >= RenderConfigFileSize.QuadPart) break;

		string CurrentLine = "";

		while (true)
		{
			if ((RenderConfigFileData[FilePointer] == '\r' && RenderConfigFileData[FilePointer + 1] == '\n') || (FilePointer >= RenderConfigFileSize.QuadPart))
			{
				FilePointer += 2;
				break;
			}

			CurrentLine += RenderConfigFileData[FilePointer];
			++FilePointer;
		}

		CurrentLine = Trim(CurrentLine);

		ConfigFileLines.push_back(CurrentLine);

		//cout << CurrentLine << endl;
	}

	string CurrentSection;

	for (const string& ConfigLine : ConfigFileLines)
	{
		if (ConfigLine[0] == '[')
		{
			string SectionName = ConfigLine;
			SectionName = SectionName.substr(1);
			SectionName = SectionName.substr(0, SectionName.length() - 1);

			SectionName = Trim(SectionName);

			RenderConfig.emplace(SectionName, map<string, string>());

			CurrentSection = SectionName;
		}
		else
		{
			string ParamAndValueString = ConfigLine;
			string Param = Trim(ParamAndValueString.substr(0, ParamAndValueString.find('=')));
			string Value = Trim(ParamAndValueString.substr(ParamAndValueString.find('=') + 1));

			RenderConfig[CurrentSection].emplace(Param, Value);
		}
	}
}

void ConfigSystem::ShutdownSystem()
{

}

string ConfigSystem::GetRenderConfigValueString(const string& Section, const string& Param)
{
	return RenderConfig[Section][Param];
}

int ConfigSystem::GetRenderConfigValueInt(const string& Section, const string& Param)
{
	string StrValue = RenderConfig[Section][Param];

	int Value = 0;

	for (int i = 0; i < StrValue.length(); i++)
	{
		Value = Value * 10 + (StrValue[i] - '0');
	}

	return Value;
}