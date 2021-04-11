// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ConfigSystem.h"

#include <Containers/DynamicArray.h>

String Trim(const String& InputString)
{
	String OutputString = InputString;

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

	i = OutputString.GetLength();

	while (true)
	{
		if (OutputString[i] == ' ')
		{
			OutputString = OutputString.substr(0, OutputString.GetLength() - 1);
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

	DynamicArray<String> ConfigFileLines;

	while (true)
	{
		if (FilePointer >= RenderConfigFileSize.QuadPart) break;

		String CurrentLine = "";

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

		ConfigFileLines.Add(CurrentLine);

		//cout << CurrentLine << endl;
	}

	String CurrentSection;

	//for (const String& ConfigLine : ConfigFileLines)
	for (size_t i = 0; i < ConfigFileLines.GetLength(); i++)
	{
		String& ConfigLine = ConfigFileLines[i];

		if (ConfigLine[0] == '[')
		{
			String SectionName = ConfigLine;
			SectionName = SectionName.substr(1);
			SectionName = SectionName.substr(0, SectionName.GetLength() - 1);

			SectionName = Trim(SectionName);

			RenderConfig.Insert(SectionName, HashTable<String, String>());

			CurrentSection = SectionName;
		}
		else
		{
			String ParamAndValueString = ConfigLine;
			String Param = Trim(ParamAndValueString.substr(0, ParamAndValueString.find('=')));
			String Value = Trim(ParamAndValueString.substr(ParamAndValueString.find('=') + 1));

			RenderConfig[CurrentSection].Insert(Param, Value);
		}
	}
}

void ConfigSystem::ShutdownSystem()
{

}

String ConfigSystem::GetRenderConfigValueString(const String& Section, const String& Param)
{
	return RenderConfig[Section][Param];
}

int ConfigSystem::GetRenderConfigValueInt(const String& Section, const String& Param)
{
	String StrValue = RenderConfig[Section][Param];

	int Value = 0;

	for (int i = 0; i < StrValue.GetLength(); i++)
	{
		Value = Value * 10 + (StrValue[i] - '0');
	}

	return Value;
}