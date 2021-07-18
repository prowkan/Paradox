// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SWFStream.h"
#include "ActionScriptByteCode.h"

#include "SWFMovie.h"

SWFMovie::SWFMovie(SWFStream& Stream)
{
	uint8_t SWFSignature[3];
	SWFSignature[0] = Stream.Read<uint8_t>();
	SWFSignature[1] = Stream.Read<uint8_t>();
	SWFSignature[2] = Stream.Read<uint8_t>();

	uint8_t SWFVersion = Stream.Read<uint8_t>();

	uint32_t SWFFileLength = Stream.Read<uint32_t>();

	SWFRect FrameSize = Stream.ReadRect();

	cout << FrameSize.XMin << " " << FrameSize.XMax << " " << FrameSize.YMin << " " << FrameSize.YMax << endl;

	uint8_t FrameRateParts[2];
	FrameRateParts[0] = Stream.Read<uint8_t>();
	FrameRateParts[1] = Stream.Read<uint8_t>();

	float FrameRate = (float)FrameRateParts[1] + ((float)FrameRateParts[0] / 256.0f);

	uint16_t FramesCount = Stream.Read<uint16_t>();

	uint32_t TagCode;
	uint32_t TagLength;

	while (true)
	{
		uint16_t TagCodeAndlength = Stream.Read<uint16_t>();

		TagCode = (TagCodeAndlength & (0x3FF << 6)) >> 6;
		TagLength = TagCodeAndlength & 0x3F;

		if (TagLength == 63)
		{
			TagLength = Stream.Read<uint32_t>();
		}

		SWFStream SubStream = SWFStream::CreateSubStreamFromAnother(Stream, TagLength);

		ProcessTag(SubStream, (SWFTag)TagCode, TagLength);

		Stream.SkipBytes(TagLength);

		if (Stream.IsEndOfStream()) break;
	}
}

void SWFMovie::ProcessTag(SWFStream& Stream, SWFTag TagCode, uint32_t TagLength)
{
	cout << (uint32_t)TagCode << " " << TagLength << endl;
	
	if (TagProcessFunctionTable[(uint32_t)TagCode] != nullptr)
	{
		cout << TagNames[(uint32_t)TagCode] << endl;
		(this->*TagProcessFunctionTable[(uint32_t)TagCode])(Stream, TagLength);
	}
	else
	{
		cout << "Unknown tag." << endl;
		Stream.SkipBytes(TagLength);
	}
}

void SWFMovie::ProcessEndTag(SWFStream& Stream, const uint32_t TagLength)
{
	return;
}

void SWFMovie::ProcessShowFrameTag(SWFStream& Stream, const uint32_t TagLength)
{
	return;
}

void SWFMovie::ProcessDefineShapeTag(SWFStream& Stream, const uint32_t TagLength)
{
	SWFShape Shape;

	Shape.ShapeId = Stream.Read<uint16_t>();
	Shape.ShapeRect = Stream.ReadRect();

	uint8_t FillStyleCount = Stream.Read<uint8_t>();

	for (int i = 0; i < FillStyleCount; i++)
	{
		uint8_t FillStyleType = Stream.Read<uint8_t>();

		if (FillStyleType == 0x00)
		{
			SWFRGB Color = Stream.ReadRGB();
		}

		if (FillStyleType == 0x40 || FillStyleType == 0x41 || FillStyleType == 0x42 || FillStyleType == 0x43)
		{
			uint16_t BitmapId = Stream.Read<uint16_t>();
			SWFMatrix BitmapMatrix = Stream.ReadMatrix();
		}
	}

	uint8_t LineStyleCount = Stream.Read<uint8_t>();

	for (int i = 0; i < LineStyleCount; i++)
	{
		float Width = Stream.Read<uint16_t>() / (float)SWFStream::TWIPS_IN_PIXEL;
		SWFRGB Color = Stream.ReadRGB();
	}

	uint8_t NumFillBits = (uint8_t)Stream.ReadUnsignedBits(4);
	uint8_t NumLineBits = (uint8_t)Stream.ReadUnsignedBits(4);

	while (true)
	{
		uint8_t TypeFlag = (uint8_t)Stream.ReadUnsignedBits(1);

		if (TypeFlag == 0)
		{
			uint8_t StateNewStyles = (uint8_t)Stream.ReadUnsignedBits(1);
			uint8_t StateLineStyle = (uint8_t)Stream.ReadUnsignedBits(1);
			uint8_t StateFillStyle1 = (uint8_t)Stream.ReadUnsignedBits(1);
			uint8_t StateFillStyle0 = (uint8_t)Stream.ReadUnsignedBits(1);
			uint8_t StateMoveTo = (uint8_t)Stream.ReadUnsignedBits(1);

			if ((StateNewStyles == 0) && (StateLineStyle == 0) && (StateFillStyle1 == 0) && (StateFillStyle0 == 0) && (StateMoveTo == 0))
			{
				break;
			}
			else
			{
				if (StateMoveTo)
				{
					uint8_t MoveBitsCount = (uint8_t)Stream.ReadUnsignedBits(5);
					int64_t MoveDeltaX = Stream.ReadSignedBits(MoveBitsCount) / SWFStream::TWIPS_IN_PIXEL;
					int64_t MoveDeltaY = Stream.ReadSignedBits(MoveBitsCount) / SWFStream::TWIPS_IN_PIXEL;
				}

				if (StateFillStyle0)
				{
					uint64_t FillStyle0 = Stream.ReadUnsignedBits(NumFillBits);
				}

				if (StateFillStyle1)
				{
					uint64_t FillStyle1 = Stream.ReadUnsignedBits(NumFillBits);
				}

				if (StateLineStyle)
				{
					uint64_t LineStyle = Stream.ReadUnsignedBits(NumLineBits);
				}
			}
		}
		else
		{
			uint8_t StraightFlag = (uint8_t)Stream.ReadUnsignedBits(1);

			if (StraightFlag)
			{
				uint8_t NumBits = (uint8_t)Stream.ReadUnsignedBits(4);

				uint8_t GeneralLineFlag = (uint8_t)Stream.ReadUnsignedBits(1);
				uint8_t VerticalLineFlag = 0;

				if (!GeneralLineFlag)
				{
					//VerticalLineFlag = (uint8_t)File.ReadSignedBits(1);
					VerticalLineFlag = (uint8_t)Stream.ReadUnsignedBits(1); // Согласно спецификации SWF должно быть signed, но т. к. бит один, то если он будет единичным, он будет распространен во все биты слева
				}

				if (GeneralLineFlag == 1 || VerticalLineFlag == 0)
				{
					int64_t DeltaX = Stream.ReadSignedBits(NumBits + 2) / SWFStream::TWIPS_IN_PIXEL;
				}

				if (GeneralLineFlag == 1 || VerticalLineFlag == 1)
				{
					int64_t DeltaY = Stream.ReadSignedBits(NumBits + 2) / SWFStream::TWIPS_IN_PIXEL;
				}
			}
			else
			{

			}
		}
	}

	Shapes.Add(Shape);
}

void SWFMovie::ProcessSetBackgroundColorTag(SWFStream& Stream, const uint32_t TagLength)
{
	SWFRGB BackgroundColor = Stream.ReadRGB();

	cout << "R = " << (uint32_t)BackgroundColor.R << " G = " << (uint32_t)BackgroundColor.G << " B = " << (uint32_t)BackgroundColor.B << endl;
}

void SWFMovie::ProcessDefineTextTag(SWFStream& Stream, const uint32_t TagLength)
{
	uint16_t CharacterId = Stream.Read<uint16_t>();

	SWFRect TextBounds = Stream.ReadRect();

	SWFMatrix TextMatrix = Stream.ReadMatrix();

	uint8_t GlyphBits = Stream.Read<uint8_t>();
	uint8_t AdvanceBits = Stream.Read<uint8_t>();

	while (true)
	{
		uint8_t TextRecordType = (uint8_t)Stream.ReadUnsignedBits(1);

		if (TextRecordType == 0)
		{
			Stream.AlignToByte();
			break;
		}

		uint8_t StyleFlagsReserved = (uint8_t)Stream.ReadUnsignedBits(3);
		uint8_t StyleFlagsHasFont = (uint8_t)Stream.ReadUnsignedBits(1);
		uint8_t StyleFlagsHasColor = (uint8_t)Stream.ReadUnsignedBits(1);
		uint8_t StyleFlagsYOffset = (uint8_t)Stream.ReadUnsignedBits(1);
		uint8_t StyleFlagsXOffset = (uint8_t)Stream.ReadUnsignedBits(1);

		if (StyleFlagsHasFont)
		{
			uint16_t FontId = Stream.Read<uint16_t>();
		}

		if (StyleFlagsHasColor)
		{
			SWFRGB TextColor = Stream.ReadRGB();
		}

		if (StyleFlagsXOffset)
		{
			int16_t XOffset = Stream.Read<int16_t>();
		}

		if (StyleFlagsYOffset)
		{
			int16_t YOffset = Stream.Read<int16_t>();
		}

		if (StyleFlagsHasFont)
		{
			uint16_t TextHeight = Stream.Read<uint16_t>();
		}

		uint8_t GlyphCount = Stream.Read<uint8_t>();

		for (uint8_t i = 0; i < GlyphCount; i++)
		{
			uint64_t GliphId = Stream.ReadUnsignedBits(GlyphBits);
			uint64_t GliphAdvance = Stream.ReadSignedBits(AdvanceBits);
		}

		Stream.AlignToByte();
	}
}

void SWFMovie::ProcessDefineSpriteTag(SWFStream& Stream, const uint32_t TagLength)
{
	uint16_t SpriteID = Stream.Read<uint16_t>();
	uint16_t FrameCount = Stream.Read<uint16_t>();

	SWFStream SpriteTagDataSubStream = SWFStream::CreateSubStreamFromAnother(Stream, TagLength - Stream.GetPosition());

	uint32_t SpriteTagCode;
	uint32_t SpriteTagLength;

	while (true)
	{
		uint16_t SpriteTagCodeAndlength = SpriteTagDataSubStream.Read<uint16_t>();

		SpriteTagCode = (SpriteTagCodeAndlength & (0x3FF << 6)) >> 6;
		SpriteTagLength = SpriteTagCodeAndlength & 0x3F;

		if (SpriteTagLength == 0x3F)
		{
			SpriteTagLength = SpriteTagDataSubStream.Read<uint32_t>();
		}

		SWFStream SubStream = SWFStream::CreateSubStreamFromAnother(SpriteTagDataSubStream, SpriteTagLength);

		ProcessTag(SubStream, (SWFTag)SpriteTagCode, SpriteTagLength);

		SpriteTagDataSubStream.SkipBytes(SpriteTagLength);

		if (SpriteTagDataSubStream.IsEndOfStream()) break;
	}
}

void SWFMovie::ProcessFrameLabelTag(SWFStream& Stream, const uint32_t TagLength)
{
	String Name = Stream.ReadString();
}

void SWFMovie::ProcessPlaceObject2Tag(SWFStream& Stream, const uint32_t TagLength)
{
	uint8_t HasClipActions = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasClipDepth = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasName = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasRatio = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasColorTransform = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasMatrix = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasCharacter = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasMove = (uint8_t)Stream.ReadUnsignedBits(1);

	uint16_t Depth = Stream.Read<uint16_t>();

	if (HasCharacter)
	{
		uint16_t CharacterId = Stream.Read<uint16_t>();
	}

	if (HasMatrix)
	{
		SWFMatrix Matrix = Stream.ReadMatrix();
	}

	if (HasRatio)
	{
		uint16_t Ratio = Stream.Read<uint16_t>();
	}

	if (HasName)
	{
		String Name = Stream.ReadString();
	}
}

void SWFMovie::ProcessRemoveObject2Tag(SWFStream& Stream, const uint32_t TagLength)
{
	uint16_t Depth = Stream.Read<uint16_t>();
}

void SWFMovie::ProcessDefineShape3Tag(SWFStream& Stream, const uint32_t TagLength)
{
	uint16_t ShapeId = Stream.Read<uint16_t>();
	SWFRect ShapeRect = Stream.ReadRect();

	uint8_t FillStyleCount = Stream.Read<uint8_t>();

	for (int i = 0; i < FillStyleCount; i++)
	{
		FillStyleType fillStyleType = (FillStyleType)Stream.Read<uint8_t>();

		if (fillStyleType == FillStyleType::SolidFill)
		{
			SWFRGBA Color = Stream.ReadRGBA();
		}
	}

	uint8_t LineStyleCount = Stream.Read<uint8_t>();

	for (int i = 0; i < LineStyleCount; i++)
	{
		float Width = Stream.Read<uint16_t>() / (float)SWFStream::TWIPS_IN_PIXEL;
		SWFRGBA Color = Stream.ReadRGBA();
	}

	uint8_t NumFillBits = (uint8_t)Stream.ReadUnsignedBits(4);
	uint8_t NumLineBits = (uint8_t)Stream.ReadUnsignedBits(4);

	while (true)
	{
		uint8_t TypeFlag = (uint8_t)Stream.ReadUnsignedBits(1);

		if (TypeFlag == 0)
		{
			uint8_t StateNewStyles = (uint8_t)Stream.ReadUnsignedBits(1);
			uint8_t StateLineStyle = (uint8_t)Stream.ReadUnsignedBits(1);
			uint8_t StateFillStyle1 = (uint8_t)Stream.ReadUnsignedBits(1);
			uint8_t StateFillStyle0 = (uint8_t)Stream.ReadUnsignedBits(1);
			uint8_t StateMoveTo = (uint8_t)Stream.ReadUnsignedBits(1);

			if ((StateNewStyles == 0) && (StateLineStyle == 0) && (StateFillStyle1 == 0) && (StateFillStyle0 == 0) && (StateMoveTo == 0))
			{
				break;
			}
			else
			{
				if (StateMoveTo)
				{
					uint8_t MoveBitsCount = (uint8_t)Stream.ReadUnsignedBits(5);
					int64_t MoveDeltaX = Stream.ReadSignedBits(MoveBitsCount) / SWFStream::TWIPS_IN_PIXEL;
					int64_t MoveDeltaY = Stream.ReadSignedBits(MoveBitsCount) / SWFStream::TWIPS_IN_PIXEL;
				}

				if (StateFillStyle0)
				{
					uint64_t FillStyle0 = Stream.ReadUnsignedBits(NumFillBits);
				}

				if (StateFillStyle1)
				{
					uint64_t FillStyle1 = Stream.ReadUnsignedBits(NumFillBits);
				}

				if (StateLineStyle)
				{
					uint64_t LineStyle = Stream.ReadUnsignedBits(NumLineBits);
				}
			}
		}
		else
		{
			uint8_t StraightFlag = (uint8_t)Stream.ReadUnsignedBits(1);

			if (StraightFlag)
			{
				uint8_t NumBits = (uint8_t)Stream.ReadUnsignedBits(4);

				uint8_t GeneralLineFlag = (uint8_t)Stream.ReadUnsignedBits(1);
				uint8_t VerticalLineFlag = 0;

				if (!GeneralLineFlag)
				{
					//VerticalLineFlag = (uint8_t)Stream.ReadSignedBits(1);
					VerticalLineFlag = (uint8_t)Stream.ReadUnsignedBits(1); // Согласно спецификации SWF должно быть signed, но т. к. бит один, то если он будет единичным, он будет распространен во все биты слева
				}

				if (GeneralLineFlag == 1 || VerticalLineFlag == 0)
				{
					int64_t DeltaX = Stream.ReadSignedBits(NumBits + 2) / SWFStream::TWIPS_IN_PIXEL;
				}

				if (GeneralLineFlag == 1 || VerticalLineFlag == 1)
				{
					int64_t DeltaY = Stream.ReadSignedBits(NumBits + 2) / SWFStream::TWIPS_IN_PIXEL;
				}
			}
			else
			{

			}
		}
	}
}

void SWFMovie::ProcessDefineBitsLoseLess2Tag(SWFStream& Stream, const uint32_t TagLength)
{
	SWFImage Image;

	Image.CharacterId = Stream.Read<uint16_t>();
	uint8_t BitmapFormat = Stream.Read<uint8_t>();
	Image.ImageWidth = Stream.Read<uint16_t>();
	Image.ImageHeight = Stream.Read<uint16_t>();

	SWFStream SubStream = SWFStream::CreateSubStreamFromAnother(Stream, TagLength - Stream.GetPosition());

	SIZE_T DecompressedImageSize = 4 * Image.ImageWidth * Image.ImageHeight;
	Image.ImageData = new BYTE[DecompressedImageSize];
	//ULONG DecompressedDataSize = 0;
	//ULONG CompressedDataSize = SubStream.GetLength();

	//int Result = uncompress(Image.ImageData, &DecompressedDataSize, SubStream.GetData(), CompressedDataSize);

	// Функция uncompress по непонятной причине не работает
	// Код ниже взят отсюда: https://github.com/id-Software/DOOM-3-BFG/blob/master/neo/swf/SWF_Zlib.cpp
	
	z_stream zLibStream;
	ZeroMemory(&zLibStream, sizeof(z_stream));
	zLibStream.zalloc = nullptr;
	zLibStream.zfree = nullptr;
	zLibStream.next_in = SubStream.GetData();
	zLibStream.next_out = Image.ImageData;
	zLibStream.avail_in = SubStream.GetLength();
	zLibStream.avail_out = DecompressedImageSize;

	int Result = inflateInit(&zLibStream);
	Result = inflate(&zLibStream, Z_FINISH);
	Result = inflateEnd(&zLibStream);

	Images.Add(Image);
}

void SWFMovie::ProcessDefineEditTextTag(SWFStream& Stream, const uint32_t TagLength)
{
	uint16_t CharacterId = Stream.Read<uint16_t>();

	SWFRect Bounds = Stream.ReadRect();

	Stream.AlignToByte();

	uint8_t HasText = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t WordWrap = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t Multiline = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t Password = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t ReadOnly = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasTextColor = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasMaxLength = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasFont = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasFontClass = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t AutoSize = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasLayout = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t NoSelect = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t Border = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t WasStatic = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HTML = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t UseOutlines = (uint8_t)Stream.ReadUnsignedBits(1);

	if (HasFont)
	{
		uint16_t FontId = Stream.Read<uint16_t>();
	}

	if (HasFontClass)
	{
		String FontClass = Stream.ReadString();
	}

	if (HasFont)
	{
		uint16_t FontHeight = Stream.Read<uint16_t>();
	}

	if (HasTextColor)
	{
		SWFRGBA TextColor = Stream.ReadRGBA();
	}

	if (HasMaxLength)
	{
		uint16_t MaxLength = Stream.Read<uint16_t>();
	}

	if (HasLayout)
	{
		uint8_t Align = Stream.Read<uint8_t>();
		uint16_t LeftMargin = Stream.Read<uint16_t>();
		uint16_t RightMargin = Stream.Read<uint16_t>();
		uint16_t Indent = Stream.Read<uint16_t>();
		int16_t Leading = Stream.Read<int16_t>();
	}

	String VariableName = Stream.ReadString();

	if (HasText)
	{
		String InitialText = Stream.ReadString();
	}
}

void SWFMovie::ProcessFileAttributesTag(SWFStream& Stream, const uint32_t TagLength)
{
	uint32_t Reserved = Stream.ReadUnsignedBits(1);
	uint32_t UseDirectBlit = Stream.ReadUnsignedBits(1);
	uint32_t UseGPU = Stream.ReadUnsignedBits(1);
	uint32_t HasMetadata = Stream.ReadUnsignedBits(1);
	uint32_t ActionScript3 = Stream.ReadUnsignedBits(1);
	Reserved = Stream.ReadUnsignedBits(2);
	uint32_t UseNetwork = Stream.ReadUnsignedBits(1);
	Reserved = Stream.ReadUnsignedBits(24);
}

void SWFMovie::ProcessPlaceObject3Tag(SWFStream& Stream, const uint32_t TagLength)
{
	uint8_t HasClipActions = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasClipDepth = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasName = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasRatio = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasColorTransform = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasMatrix = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasCharacter = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t Move = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t Reserved = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t OpaqueBackground = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasVisible = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasImage = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasClassName = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasCacheAsBitmap = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasBlendMode = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t HasFilterList = (uint8_t)Stream.ReadUnsignedBits(1);

	uint16_t Depth = Stream.Read<uint16_t>();

	if (HasCharacter)
	{
		uint16_t CharacterId = Stream.Read<uint16_t>();
	}

	if (HasMatrix)
	{
		SWFMatrix Matrix = Stream.ReadMatrix();
	}

	if (HasRatio)
	{
		uint16_t Ratio = Stream.Read<uint16_t>();
	}

	if (HasFilterList)
	{
		uint8_t NumberOfFilters = Stream.Read<uint8_t>();

		for (uint8_t i = 0; i < NumberOfFilters; i++)
		{
			uint8_t FilterId = Stream.Read<uint8_t>();

			if (FilterId == 0)
			{
				SWFRGBA DropShadowColor = Stream.ReadRGBA();
				float BlurX = Stream.ReadFixed();
				float BlurY = Stream.ReadFixed();
				float Angle = Stream.ReadFixed();
				float Distance = Stream.ReadFixed();
				float Strength = Stream.ReadFixed8();
				uint8_t InnerShadow = (uint8_t)Stream.ReadUnsignedBits(1);
				uint8_t Knockout = (uint8_t)Stream.ReadUnsignedBits(1);
				uint8_t CompositeSource = (uint8_t)Stream.ReadUnsignedBits(1);
				uint8_t Passes = (uint8_t)Stream.ReadUnsignedBits(5);
			}
		}
	}
}

void SWFMovie::ProcessDefineFongAlignZonesTag(SWFStream& Stream, const uint32_t TagLength)
{
	uint16_t FontId = Stream.Read<uint16_t>();

	uint8_t CSMTableHint = (uint8_t)Stream.ReadUnsignedBits(2);
	uint8_t Reserved = (uint8_t)Stream.ReadUnsignedBits(6);

	SWFFont *Font = nullptr;

	for (SWFFont& FontIt : Fonts)
	{
		if (FontIt.FontId == FontId)
		{
			Font = &FontIt;
			break;
		}
	}

	for (uint16_t i = 0; i < Font->NumGliphs; i++)
	{
		uint8_t NumZoneData = Stream.Read<uint8_t>();

		for (uint8_t j = 0; j < NumZoneData; j++)
		{
			uint16_t AlignmentCoordinate = Stream.Read<uint16_t>();
			uint16_t Range = Stream.Read<uint16_t>();
		}

		uint8_t Reserved = (uint8_t)Stream.ReadUnsignedBits(6);
		uint8_t ZoneMaskY = (uint8_t)Stream.ReadUnsignedBits(1);
		uint8_t ZoneMaskX = (uint8_t)Stream.ReadUnsignedBits(1);
	}
}

void SWFMovie::ProcessCSMTextSettingsTag(SWFStream& Stream, const uint32_t TagLength)
{
	uint16_t TextId = Stream.Read<uint16_t>();

	uint8_t UseFlashType = (uint8_t)Stream.ReadUnsignedBits(2);
	uint8_t GridFit = (uint8_t)Stream.ReadUnsignedBits(3);
	uint8_t Reserved = (uint8_t)Stream.ReadUnsignedBits(3);

	float Thickness = Stream.Read<float>();
	float Sharpness = Stream.Read<float>();

	Reserved = Stream.Read<uint8_t>();
}

void SWFMovie::ProcessDefineFont3Tag(SWFStream& Stream, const uint32_t TagLength)
{
	uint16_t FontId = Stream.Read<uint16_t>();

	uint8_t HasLayout = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t ShiftJIS = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t SmallText = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t ANSI = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t WideOffsets = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t WideCodes = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t Italic = (uint8_t)Stream.ReadUnsignedBits(1);
	uint8_t Bold = (uint8_t)Stream.ReadUnsignedBits(1);

	uint8_t LanguageCode = Stream.Read<uint8_t>();
	uint8_t FontNameLen = Stream.Read<uint8_t>();

	char *FontName = new char[FontNameLen];

	for (int i = 0; i < FontNameLen; i++)
	{
		FontName[i] = Stream.Read<char>();
	}

	delete FontName;

	uint16_t NumGliphs = Stream.Read<uint16_t>();

	if (WideOffsets)
	{
		for (int i = 0; i < NumGliphs; i++)
		{
			uint32_t Offset = Stream.Read<uint32_t>();
		}
	}
	else
	{
		for (int i = 0; i < NumGliphs; i++)
		{
			uint16_t Offset = Stream.Read<uint16_t>();
		}
	}

	if (WideOffsets)
	{
		uint32_t CodeTableOffset = Stream.Read<uint32_t>();
	}
	else
	{
		uint16_t CodeTableOffset = Stream.Read<uint16_t>();
	}

	for (int i = 0; i < NumGliphs; i++)
	{
		Stream.AlignToByte();

		uint8_t NumFillBits = (uint8_t)Stream.ReadUnsignedBits(4);
		uint8_t NumLineBits = (uint8_t)Stream.ReadUnsignedBits(4);

		//cout << "Glyph Id: " << i << endl;

		int CurrentPointX = 0, CurrentPointY = 0;

		while (true)
		{
			uint8_t TypeFlag = (uint8_t)Stream.ReadUnsignedBits(1);

			if (TypeFlag == 0)
			{
				uint8_t StateNewStyles = (uint8_t)Stream.ReadUnsignedBits(1);
				uint8_t StateLineStyle = (uint8_t)Stream.ReadUnsignedBits(1);
				uint8_t StateFillStyle1 = (uint8_t)Stream.ReadUnsignedBits(1);
				uint8_t StateFillStyle0 = (uint8_t)Stream.ReadUnsignedBits(1);
				uint8_t StateMoveTo = (uint8_t)Stream.ReadUnsignedBits(1);

				if ((StateNewStyles == 0) && (StateLineStyle == 0) && (StateFillStyle1 == 0) && (StateFillStyle0 == 0) && (StateMoveTo == 0))
				{
					break;
				}
				else
				{
					if (StateMoveTo)
					{
						uint8_t MoveBitsCount = (uint8_t)Stream.ReadUnsignedBits(5);
						int64_t MoveDeltaX = Stream.ReadSignedBits(MoveBitsCount) / SWFStream::TWIPS_IN_PIXEL;
						int64_t MoveDeltaY = Stream.ReadSignedBits(MoveBitsCount) / SWFStream::TWIPS_IN_PIXEL;

						//cout << "pf.StartPoint = new Point(" << MoveDeltaX << ", " << MoveDeltaY << ");" << endl;

						CurrentPointX = MoveDeltaX;
						CurrentPointY = MoveDeltaY;
					}

					if (StateFillStyle0)
					{
						uint64_t FillStyle0 = Stream.ReadUnsignedBits(NumFillBits);
					}

					if (StateFillStyle1)
					{
						uint64_t FillStyle1 = Stream.ReadUnsignedBits(NumFillBits);
					}

					if (StateLineStyle)
					{
						uint64_t LineStyle = Stream.ReadUnsignedBits(NumLineBits);
					}
				}
			}
			else
			{
				uint8_t StraightFlag = (uint8_t)Stream.ReadUnsignedBits(1);

				if (StraightFlag)
				{
					uint8_t NumBits = (uint8_t)Stream.ReadUnsignedBits(4);

					uint8_t GeneralLineFlag = (uint8_t)Stream.ReadUnsignedBits(1);
					uint8_t VerticalLineFlag = 0;

					if (!GeneralLineFlag)
					{
						//VerticalLineFlag = (uint8_t)Stream.ReadSignedBits(1);
						VerticalLineFlag = (uint8_t)Stream.ReadUnsignedBits(1); // Согласно спецификации SWF должно быть signed, но т. к. бит один, то если он будет единичным, он будет распространен во все биты слева
					}

					int64_t DeltaX = 0;
					int64_t DeltaY = 0;

					if (GeneralLineFlag == 1 || VerticalLineFlag == 0)
					{
						DeltaX = Stream.ReadSignedBits(NumBits + 2) / SWFStream::TWIPS_IN_PIXEL;
					}

					if (GeneralLineFlag == 1 || VerticalLineFlag == 1)
					{
						DeltaY = Stream.ReadSignedBits(NumBits + 2) / SWFStream::TWIPS_IN_PIXEL;
					}

					//cout << "pf.Segments.Add(new LineSegment(new Point(" << CurrentPointX + DeltaX << ", " << CurrentPointY + DeltaY << "), true));" << endl;

					CurrentPointX += DeltaX;
					CurrentPointY += DeltaY;
				}
				else
				{
					uint8_t NumBits = (uint8_t)Stream.ReadUnsignedBits(4);
					int64_t ControlDeltaX = Stream.ReadSignedBits(NumBits + 2) / SWFStream::TWIPS_IN_PIXEL;
					int64_t ControlDeltaY = Stream.ReadSignedBits(NumBits + 2) / SWFStream::TWIPS_IN_PIXEL;
					int64_t AnchorDeltaX = Stream.ReadSignedBits(NumBits + 2) / SWFStream::TWIPS_IN_PIXEL;
					int64_t AnchorDeltaY = Stream.ReadSignedBits(NumBits + 2) / SWFStream::TWIPS_IN_PIXEL;

					//cout << "pf.Segments.Add(new QuadraticBezierSegment(new Point(" << CurrentPointX + ControlDeltaX << ", " << CurrentPointY + ControlDeltaY << "), new Point(" << CurrentPointX + ControlDeltaX + AnchorDeltaX << ", " << CurrentPointY + ControlDeltaY + AnchorDeltaY << "), true)); " << endl;
				
					CurrentPointX += ControlDeltaX + AnchorDeltaX;
					CurrentPointY += ControlDeltaY + AnchorDeltaY;
				}
			}
		}
	}

	for (int i = 0; i < NumGliphs; i++)
	{
		uint16_t Code = Stream.Read<uint16_t>();
	}

	if (HasLayout)
	{
		uint16_t FontAscent = Stream.Read<uint16_t>();
		uint16_t FontDescent = Stream.Read<uint16_t>();
		int16_t FontLeading = Stream.Read<int16_t>();

		for (int i = 0; i < NumGliphs; i++)
		{
			int16_t FontAdvanceTable = Stream.Read<int16_t>();
		}

		for (int i = 0; i < NumGliphs; i++)
		{
			SWFRect FontBoundsTable = Stream.ReadRect();
		}

		uint16_t KerningCount = Stream.Read<uint16_t>();

		for (int i = 0; i < KerningCount; i++)
		{
			if (WideCodes)
			{
				uint16_t KerningCode1 = Stream.Read<uint16_t>();
				uint16_t KerningCode2 = Stream.Read<uint16_t>();
			}
			else
			{
				uint8_t KerningCode1 = Stream.Read<uint8_t>();
				uint8_t KerningCode2 = Stream.Read<uint8_t>();
			}

			int16_t KerningAdjustment = Stream.Read<int16_t>();
		}
	}

	SWFFont Font;
	Font.FontId = FontId;
	Font.NumGliphs = NumGliphs;

	Fonts.Add(Font);
}

void SWFMovie::ProcessSymbolClassTag(SWFStream& Stream, const uint32_t TagLength)
{
	uint16_t NumSymbols = Stream.Read<uint16_t>();

	for (uint16_t i = 0; i < NumSymbols; i++)
	{
		uint16_t Tag = Stream.Read<uint16_t>();

		String Name = Stream.ReadString();
	}
}

void SWFMovie::ProcessDoABCTag(SWFStream& Stream, const uint32_t TagLength)
{
	uint32_t Flags = Stream.Read<uint32_t>();

	Stream.ReadString();

	ActionScriptByteCode ASByteCode(Stream.GetData() + Stream.GetPosition(), TagLength - Stream.GetPosition());
}

void SWFMovie::ProcessDefineSceneAndFrameLabelDataTag(SWFStream& Stream, const uint32_t TagLength)
{
	uint32_t SceneCount = Stream.ReadEncodedU32();

	for (uint32_t i = 0; i < SceneCount; i++)
	{
		uint32_t Offset = Stream.ReadEncodedU32();
		String Name = Stream.ReadString();
	}

	uint32_t FrameLabelsCount = Stream.ReadEncodedU32();

	for (uint32_t i = 0; i < FrameLabelsCount; i++)
	{
		uint32_t FrameLabelNum = Stream.ReadEncodedU32();
		String FrameLabel = Stream.ReadString();
	}
}

void SWFMovie::ProcessDefineFontNameTag(SWFStream& Stream, const uint32_t TagLength)
{
	uint16_t FontId = Stream.Read<uint16_t>();
	String FontName = Stream.ReadString();
	String FontCopyright = Stream.ReadString();
}