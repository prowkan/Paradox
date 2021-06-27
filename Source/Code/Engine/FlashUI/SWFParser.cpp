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

	float FrameRate = (float)FrameRateParts[1] + ((float)FrameRateParts[0] / 256.0f);

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
		case TAG_DEFINE_SHAPE:
			cout << "DefineShape tag." << endl;
			ProcessDefineShapeTag(File);
			break;
		case TAG_SET_BACKGROUND_COLOR:
			cout << "SetBackgroundColor tag." << endl;
			ProcessSetBackgroundColorTag(File);
			break;
		case TAG_PLACE_OBJECT_2:
			cout << "PlaceObject2 tag." << endl;
			ProcessPlaceObject2Tag(File);
			break;
		case TAG_FILE_ATTRIBUTES:
			cout << "FileAttributes tag." << endl;
			ProcessFileAttributesTag(File);
			break;
		case TAG_DEFINE_SCENE_AND_FRAME_LABEL_DATA:
			cout << "DefineSceneAndFrameLabelData tag." << endl;
			ProcessDefineSceneAndFrameLabelDataTag(File);
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
	uint16_t ShapeId = File.Read<uint16_t>();
	SWFRect ShapeRect = File.ReadRect();

	uint8_t FillStyleCount = File.Read<uint8_t>();

	for (int i = 0; i < FillStyleCount; i++)
	{
		uint8_t FillStyleType = File.Read<uint8_t>();

		if (FillStyleType == 0x00)
		{
			SWFRGB Color = File.ReadRGB();
		}
	}

	uint8_t LineStyleCount = File.Read<uint8_t>();

	for (int i = 0; i < LineStyleCount; i++)
	{
		float Width = File.Read<uint16_t>() / (float)SWFFile::TWIPS_IN_PIXEL;
		SWFRGB Color = File.ReadRGB();
	}

	uint8_t NumFillBits = (uint8_t)File.ReadUnsignedBits(4);
	uint8_t NumLineBits = (uint8_t)File.ReadUnsignedBits(4);

	while (true)
	{
		uint8_t TypeFlag = (uint8_t)File.ReadUnsignedBits(1);

		if (TypeFlag == 0)
		{
			uint8_t StateNewStyles = (uint8_t)File.ReadUnsignedBits(1);
			uint8_t StateLineStyle = (uint8_t)File.ReadUnsignedBits(1);
			uint8_t StateFillStyle1 = (uint8_t)File.ReadUnsignedBits(1);
			uint8_t StateFillStyle0 = (uint8_t)File.ReadUnsignedBits(1);
			uint8_t StateMoveTo = (uint8_t)File.ReadUnsignedBits(1);

			if ((StateNewStyles == 0) && (StateLineStyle == 0) && (StateFillStyle1 == 0) && (StateFillStyle0 == 0) && (StateMoveTo == 0))
			{
				break;
			}
			else
			{
				if (StateMoveTo)
				{
					uint8_t MoveBitsCount = (uint8_t)File.ReadUnsignedBits(5);
					int64_t MoveDeltaX = File.ReadSignedBits(MoveBitsCount) / SWFFile::TWIPS_IN_PIXEL;
					int64_t MoveDeltaY = File.ReadSignedBits(MoveBitsCount) / SWFFile::TWIPS_IN_PIXEL;
				}

				if (StateFillStyle0)
				{
					uint64_t FillStyle0 = File.ReadUnsignedBits(NumFillBits);
				}

				if (StateFillStyle1)
				{
					uint64_t FillStyle1 = File.ReadUnsignedBits(NumFillBits);
				}

				if (StateLineStyle)
				{
					uint64_t LineStyle = File.ReadUnsignedBits(NumLineBits);
				}
			}
		}
		else
		{
			uint8_t StraightFlag = (uint8_t)File.ReadUnsignedBits(1);

			if (StraightFlag)
			{
				uint8_t NumBits = (uint8_t)File.ReadUnsignedBits(4);

				uint8_t GeneralLineFlag = (uint8_t)File.ReadUnsignedBits(1);
				uint8_t VerticalLineFlag = 0;

				if (!GeneralLineFlag)
				{
					//VerticalLineFlag = (uint8_t)File.ReadSignedBits(1);
					VerticalLineFlag = (uint8_t)File.ReadUnsignedBits(1); // Согласно спецификации SWF должно быть unsigned, но т. к. бит один, то если он будет единичным, он будет распространен во все биты слева
				}

				if (GeneralLineFlag == 1 || VerticalLineFlag == 0)
				{
					int64_t DeltaX = File.ReadSignedBits(NumBits + 2) / SWFFile::TWIPS_IN_PIXEL;
				}

				if (GeneralLineFlag == 1 || VerticalLineFlag == 1)
				{
					int64_t DeltaY = File.ReadSignedBits(NumBits + 2) / SWFFile::TWIPS_IN_PIXEL;
				}
			}
			else
			{

			}
		}
	}
}

void SWFParser::ProcessSetBackgroundColorTag(SWFFile& File)
{
	SWFRGB BackgroundColor = File.ReadRGB();

	cout << "R = " << (uint32_t)BackgroundColor.R << " G = " << (uint32_t)BackgroundColor.G << " B = " << (uint32_t)BackgroundColor.B << endl;
}

void SWFParser::ProcessPlaceObject2Tag(SWFFile& File)
{
	uint8_t HasClipActions = (uint8_t)File.ReadUnsignedBits(1);
	uint8_t HasClipDepth = (uint8_t)File.ReadUnsignedBits(1);
	uint8_t HasName = (uint8_t)File.ReadUnsignedBits(1);
	uint8_t HasRatio = (uint8_t)File.ReadUnsignedBits(1);
	uint8_t HasColorTransform = (uint8_t)File.ReadUnsignedBits(1);
	uint8_t HasMatrix = (uint8_t)File.ReadUnsignedBits(1);
	uint8_t HasCharacter = (uint8_t)File.ReadUnsignedBits(1);
	uint8_t HasMove = (uint8_t)File.ReadUnsignedBits(1);

	uint16_t Depth = File.Read<uint16_t>();

	if (HasCharacter)
	{
		uint16_t CharacterId = File.Read<uint16_t>();
	}

	if (HasMatrix)
	{
		SWFMatrix Matrix = File.ReadMatrix();
	}
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

void SWFParser::ProcessDefineSceneAndFrameLabelDataTag(SWFFile& File)
{
	uint32_t SceneCount = File.ReadEncodedU32();

	for (uint32_t i = 0; i < SceneCount; i++)
	{
		uint32_t Offset = File.ReadEncodedU32();
		File.ReadString();
	}

	uint32_t FrameLabelsCount = File.ReadEncodedU32();

	for (uint32_t i = 0; i < FrameLabelsCount; i++)
	{
		uint32_t FrameLabelNum = File.ReadEncodedU32();
		File.ReadString();
	}
}