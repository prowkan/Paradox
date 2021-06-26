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

	while (true)
	{
		uint16_t TagCodeAndlength = File.Read<uint16_t>();

		TagCode = (TagCodeAndlength & (0b1111111111 << 6)) >> 6;
		TagLength = TagCodeAndlength & 0b111111;

		if (TagLength == 63)
		{
			TagLength = File.Read<uint32_t>();
		}

		ProcessTag(File, TagCode, TagLength);

		if (File.IsEndOfFile()) break;
	}
}

void SWFParser::ProcessTag(SWFFile& File, uint32_t TagCode, uint32_t TagLength)
{
	cout << TagCode << " " << TagLength << endl;

	switch (TagCode)
	{
		case TAG_END:
			cout << "End tag." << endl;
			ProcessEndTag(File);
			break;
		case TAG_SHOW_FRAME:
			cout << "ShowFrame tag." << endl;
			ProcessShowFrameTag(File);
			break;
		case TAG_SET_BACKGROUND_COLOR:
			cout << "SetBackgroundColor tag." << endl;
			ProcessSetBackgroundColorTag(File);
			break;
		case TAG_FILE_ATTRIBUTES:
			cout << "FileAttributes tag." << endl;
			ProcessFileAttributesTag(File);
			break;
		default:
			cout << "Unknown tag." << endl;
			File.SkipBytes(TagLength);
			break;
	}
}

void SWFParser::ProcessEndTag(SWFFile& File)
{
	return;
}

void SWFParser::ProcessShowFrameTag(SWFFile& File)
{
	return;
}

void SWFParser::ProcessDefineShapeTag(SWFFile& File)
{

}

void SWFParser::ProcessSetBackgroundColorTag(SWFFile& File)
{
	SWFRGB BackgroundColor = File.ReadRGB();

	cout << "R = " << (uint32_t)BackgroundColor.R << " G = " << (uint32_t)BackgroundColor.G << " B = " << (uint32_t)BackgroundColor.B << endl;

	return;
}

void SWFParser::ProcessFileAttributesTag(SWFFile& File)
{
	uint32_t Reserved = File.ReadUnsignedBits(1);
	uint32_t UseDirectBlit = File.ReadUnsignedBits(1);
	uint32_t UseGPU = File.ReadUnsignedBits(1);
	uint32_t HasMetadata = File.ReadUnsignedBits(1);
	uint32_t ActionScript3 = File.ReadUnsignedBits(1);
	Reserved = File.ReadUnsignedBits(2);
	uint32_t UseNetwork = File.ReadUnsignedBits(1);
	Reserved = File.ReadUnsignedBits(24);
}