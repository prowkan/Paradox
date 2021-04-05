// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SWFParser.h"

#include "SWFFile.h"

void SWFParser::ParseFile(SWFFile& File)
{
	uint8_t SWFSignature[3];
	SWFSignature[0] = File.Read<uint8_t>();
	SWFSignature[1] = File.Read<uint8_t>();
	SWFSignature[2] = File.Read<uint8_t>();

	uint8_t SWFVersion = File.Read<uint8_t>();

	uint32_t SWFFileLength = File.Read<uint32_t>();

	SWFRect FrameSize = File.ReadRect();

	cout << FrameSize.XMin << " " << FrameSize.XMax << " " << FrameSize.YMin << " " << FrameSize.YMax << endl;

	uint8_t FrameRateParts[2];
	FrameRateParts[0] = File.Read<uint8_t>();
	FrameRateParts[1] = File.Read<uint8_t>();

	float FrameRate = (float)FrameRateParts[1];

	uint16_t FramesCount = File.Read<uint16_t>();

	uint32_t TagCode;
	uint32_t TagLength;
	void *TagData;

	while (File.ReadTag(TagCode, TagLength, TagData))
	{
		ProcessTag(TagCode, TagLength, TagData);
		free(TagData);
	}
}

void SWFParser::ProcessTag(uint32_t TagCode, uint32_t TagLength, void* TagData)
{
	cout << TagCode << " " << TagLength << endl;
}