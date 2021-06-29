// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ConfigSystem.h"

#include <Containers/DynamicArray.h>

#include <MemoryManager/SystemMemoryAllocator.h>

String Trim(const String& InputString)
{
	String OutputString = InputString;

	size_t i = 0;

	while (true)
	{
		if (OutputString[i] == ' ')
		{
			OutputString = OutputString.GetSubString(1);
			i++;
		}
		else break;
	}

	i = OutputString.GetLength();

	while (true)
	{
		if (OutputString[i] == ' ')
		{
			OutputString = OutputString.GetSubString(0, OutputString.GetLength() - 1);
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
	BYTE *RenderConfigFileData = (BYTE*)SystemMemoryAllocator::AllocateMemory(RenderConfigFileSize.QuadPart);
	Result = ReadFile(RenderConfigFileHandle, RenderConfigFileData, (DWORD)RenderConfigFileSize.QuadPart, NULL, NULL);
	Result = CloseHandle(RenderConfigFileHandle);

	size_t FilePointer = 0;

	DynamicArray<String> ConfigFileLines;

	while (true)
	{
		if (FilePointer >= (size_t)RenderConfigFileSize.QuadPart) break;

		String CurrentLine = "";

		while (true)
		{
			if ((RenderConfigFileData[FilePointer] == '\r' && RenderConfigFileData[FilePointer + 1] == '\n') || (FilePointer >= (size_t)RenderConfigFileSize.QuadPart))
			{
				FilePointer += 2;
				break;
			}

			CurrentLine += String((char)RenderConfigFileData[FilePointer]);
			++FilePointer;
		}

		CurrentLine = Trim(CurrentLine);

		ConfigFileLines.Add(CurrentLine);
	}

	String CurrentSection;

	for (const String& ConfigLine : ConfigFileLines)
	{
		if (ConfigLine[0] == '[')
		{
			String SectionName = ConfigLine;
			SectionName = SectionName.GetSubString(1);
			SectionName = SectionName.GetSubString(0, SectionName.GetLength() - 1);

			SectionName = Trim(SectionName);

			RenderConfig.Insert(SectionName, HashTable<String, String>());

			CurrentSection = SectionName;
		}
		else
		{
			String ParamAndValueString = ConfigLine;
			String Param = Trim(ParamAndValueString.GetSubString(0, ParamAndValueString.FindFirst('=')));
			String Value = Trim(ParamAndValueString.GetSubString(ParamAndValueString.FindFirst('=') + 1));

			RenderConfig[CurrentSection].Insert(Param, Value);
		}
	}

	SystemMemoryAllocator::FreeMemory(RenderConfigFileData);
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