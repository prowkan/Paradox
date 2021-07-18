#pragma once

#include <Containers/DynamicArray.h>

#include "SWFStream.h"

class SWFStream;

class SWFMovie
{
	public:

		enum class FillStyleType : uint8_t
		{
			SolidFill = 0x00,
			LinearGradientFill = 0x10,
			RadialGradientFill = 0x12,
			FocalRadialGradientFill = 0x13,
			RepeatingBitmapFill = 0x40,
			ClippedBitmapFill = 0x41,
			NonSmoothedRepeatingBitmapFill = 0x42,
			NonSmoothedClippedBitmapFill = 0x43
		};

		struct SWFFillStyle
		{
			FillStyleType FillType;
			union
			{
				struct
				{
					SWFRGBA Color;
				} Solid;
				struct
				{
					SWFMatrix GradientMatrix;
				} Gradient;
				struct
				{
					uint16_t BitmapId;
					SWFMatrix BitmapMatrix;
				};
			};
		};

		struct SWFLineStyle
		{

		};

		struct SWFShape
		{
			uint16_t ShapeId;
			SWFRect ShapeRect;
		};

		struct SWFShapeWithStyle : SWFShape
		{
			DynamicArray<SWFFillStyle> FillStyles;
			DynamicArray<SWFLineStyle> LineStyles;
		};

		struct SWFImage
		{
			uint16_t CharacterId;
			uint16_t ImageWidth;
			uint16_t ImageHeight;
			BYTE *ImageData;
		};

		struct SWFFont
		{
			uint16_t FontId;
			uint16_t NumGliphs;
		};

		struct SWFSprite
		{

		};

		SWFMovie(SWFStream& Stream);

	private:

		DynamicArray<SWFShape> Shapes;
		DynamicArray<SWFImage> Images;
		DynamicArray<SWFFont> Fonts;
		DynamicArray<SWFSprite> Sprite;

		enum class SWFTag : uint16_t
		{
			TAG_END = 0,
			TAG_SHOW_FRAME = 1,
			TAG_DEFINE_SHAPE = 2,
			TAG_PLACE_OBJECT = 4,
			TAG_REMOVE_OBJECT = 5,
			TAG_DEFINE_BITS = 6,
			TAG_DEFINE_BUTTON = 7,
			TAG_JPEG_TABLES = 8,
			TAG_SET_BACKGROUND_COLOR = 9,
			TAG_DEFINE_FONT = 10,
			TAG_DEFINE_TEXT = 11,
			TAG_DO_ACTION = 12,
			TAG_DEFINE_FONT_INFO = 13,
			TAG_DEFINE_SOUND = 14,
			TAG_START_SOUND = 15,
			TAG_DEFINE_BUTTON_SOUND = 17,
			TAG_SOUND_STREAM_HEAD = 18,
			TAG_SOUND_STREAM_BLOCK = 19,
			TAG_DEFINE_BITS_LOSELESS = 20,
			TAG_DEFINE_BITS_JPEG_2 = 21,
			TAG_DEFINE_SHAPE_2 = 22,
			TAG_PROTECT = 24,
			TAG_PLACE_OBJECT_2 = 26,
			TAG_REMOVE_OBJECT_2 = 28,
			TAG_DEFINE_SHAPE_3 = 32,
			TAG_DEFINE_TEXT_2 = 33,
			TAG_DEFINE_BUTTON_2 = 34,
			TAG_DEFINE_BITS_JPEG_3 = 35,
			TAG_DEFINE_BITS_LOSELESS_2 = 36,
			TAG_DEFINE_EDIT_TEXT = 37,
			TAG_DEFINE_SPRITE = 39,
			TAG_FRAME_LABEL = 43,
			TAG_SOUND_STREAM_HEAD_2 = 45,
			TAG_DEFINE_MORPH_SHAPE = 46,
			TAG_DEFINE_FONT_2 = 48,
			TAG_EXPORT_ASSETS = 56,
			TAG_IMPORT_ASSETS = 57,
			TAG_ENABLE_DEBUGGER = 58,
			TAG_DO_INIT_ACTION = 59,
			TAG_DEFINE_VIDEO_STREAM = 60,
			TAG_VIDEO_FRAME = 61,
			TAG_DEFINE_FONT_INFO_2 = 62,
			TAG_ENABLE_DEBUGGER_2 = 64,
			TAG_SCRIPT_LIMITS = 65,
			TAG_SET_TAB_INDEX = 66,
			TAG_FILE_ATTRIBUTES = 69,
			TAG_PLACE_OBJECT_3 = 70,
			TAG_IMPORT_ASSETS_2 = 71,
			TAG_DEFINE_FONT_ALIGN_ZONES = 73,
			TAG_CSM_TEXT_SETTINGS = 74,
			TAG_DEFINE_FONT_3 = 75,
			TAG_SYMBOL_CLASS = 76,
			TAG_METADATA = 77,
			TAG_DEFINE_SCALING_GRID = 78,
			TAG_DO_ABC = 82,
			TAG_DEFINE_SHAPE_4 = 83,
			TAG_DEFINE_MORPH_SHAPE_2 = 84,
			TAG_DEFINE_SCENE_AND_FRAME_LABEL_DATA = 86,
			TAG_DEFINE_BINARY_DATA = 87,
			TAG_DEFINE_FONT_NAME = 88,
			TAG_START_SOUND_2 = 89,
			TAG_DEFINE_BITS_JPEG_4 = 90,
			TAG_DEFINE_FONT_4 = 91,
			TAG_ENABLE_TELEMETRY = 93
		};

		void ProcessTag(SWFStream& Stream, const SWFTag TagCode, const uint32_t TagLength);

		void ProcessEndTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessShowFrameTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessDefineShapeTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessSetBackgroundColorTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessDefineTextTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessPlaceObject2Tag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessRemoveObject2Tag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessDefineShape3Tag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessDefineBitsLoseLess2Tag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessDefineEditTextTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessDefineSpriteTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessFrameLabelTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessFileAttributesTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessPlaceObject3Tag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessDefineFongAlignZonesTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessCSMTextSettingsTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessDefineFont3Tag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessSymbolClassTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessDoABCTag(SWFStream& Stream, uint32_t TagLength);
		void ProcessDefineSceneAndFrameLabelDataTag(SWFStream& Stream, const uint32_t TagLength);
		void ProcessDefineFontNameTag(SWFStream& Stream, const uint32_t TagLength);

		using TagProcessFunction = void(SWFMovie::*)(SWFStream& Stream, const uint32_t TagLength);

		TagProcessFunction TagProcessFunctionTable[100] =
		{
			&SWFMovie::ProcessEndTag,
			&SWFMovie::ProcessShowFrameTag,
			&SWFMovie::ProcessDefineShapeTag,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			&SWFMovie::ProcessSetBackgroundColorTag,
			nullptr,
			&SWFMovie::ProcessDefineTextTag,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			&SWFMovie::ProcessPlaceObject2Tag,
			nullptr,
			&SWFMovie::ProcessRemoveObject2Tag,
			nullptr,
			nullptr,
			nullptr,
			&SWFMovie::ProcessDefineShape3Tag,
			nullptr,
			nullptr,
			nullptr,
			&SWFMovie::ProcessDefineBitsLoseLess2Tag,
			&SWFMovie::ProcessDefineEditTextTag,
			nullptr,
			&SWFMovie::ProcessDefineSpriteTag,
			nullptr,
			nullptr,
			nullptr,
			&SWFMovie::ProcessFrameLabelTag,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			&SWFMovie::ProcessFileAttributesTag,
			&SWFMovie::ProcessPlaceObject3Tag,
			nullptr,
			nullptr,
			&SWFMovie::ProcessDefineFongAlignZonesTag,
			&SWFMovie::ProcessCSMTextSettingsTag,
			&SWFMovie::ProcessDefineFont3Tag,
			&SWFMovie::ProcessSymbolClassTag,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			&SWFMovie::ProcessDoABCTag,
			nullptr,
			nullptr,
			nullptr,
			&SWFMovie::ProcessDefineSceneAndFrameLabelDataTag,
			nullptr,
			&SWFMovie::ProcessDefineFontNameTag
		};

		const char *TagNames[100] =
		{
			"End tag.",
			"ShowFrame tag.",
			"DefineShape tag.",
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			"SetBackgroundColor tag.",
			nullptr,
			"DefineText tag.",
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			"PlaceObject2 tag.",
			nullptr,
			"RemoveObject2 tag.",
			nullptr,
			nullptr,
			nullptr,
			"DefineShape3 tag.",
			nullptr,
			nullptr,
			nullptr,
			"DefineBitsLoseLess2 tag.",
			"DefineEditText tag.",
			nullptr,
			"DefineSprite tag.",
			nullptr,
			nullptr,
			nullptr,
			"FrameLabel tag.",
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			"FileAttributes tag.",
			"PlaceObject3 tag.",
			nullptr,
			nullptr,
			"DefineFongAlignZones tag.",
			"CSMTextSettings tag.",
			"DefineFont3 tag.",
			"SymbolClass tag.",
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			"DoABC tag.",
			nullptr,
			nullptr,
			nullptr,
			"DefineSceneAndFrameLabelData tag.",
			nullptr,
			"DefineFontName Tag."
		};
};