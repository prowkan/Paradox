#pragma once

class SWFFile;

class SWFParser
{
	public:

		static void ParseFile(SWFFile& File);

	private:

		static void ProcessTag(SWFFile& File, const uint32_t TagCode, const uint32_t TagLength);

		static void ProcessEndTag(SWFFile& File);
		static void ProcessShowFrameTag(SWFFile& File);
		static void ProcessDefineShapeTag(SWFFile& File);
		static void ProcessSetBackgroundColorTag(SWFFile& File);
		static void ProcessPlaceObject2Tag(SWFFile& File);
		static void ProcessDefineEditTextTag(SWFFile& File);
		static void ProcessFileAttributesTag(SWFFile& File);
		static void ProcessDefineFont3Tag(SWFFile& File);
		static void ProcessDefineSceneAndFrameLabelDataTag(SWFFile& File);
		static void ProcessDefineFontNameTag(SWFFile& File);

		static const uint32_t TAG_END = 0;
		static const uint32_t TAG_SHOW_FRAME = 1;
		static const uint32_t TAG_DEFINE_SHAPE = 2;

		static const uint32_t TAG_SET_BACKGROUND_COLOR = 9;

		static const uint32_t TAG_PLACE_OBJECT_2 = 26;

		static const uint32_t TAG_DEFINE_EDIT_TEXT = 37;

		static const uint32_t TAG_FILE_ATTRIBUTES = 69;

		static const uint32_t TAG_DEFINE_FONT_ALIGN_ZONES = 73;
		static const uint32_t TAG_CSM_TEXT_SETTINGS = 74;
		static const uint32_t TAG_DEFINE_FONT_3 = 75;

		static const uint32_t TAG_DEFINE_SCENE_AND_FRAME_LABEL_DATA = 86;

		static const uint32_t TAG_DEFINE_FONT_NAME = 88;
};