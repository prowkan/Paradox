#pragma once

class SWFFile;

class SWFParser
{
	public:

		static void ParseFile(SWFFile& File);

	private:

		static void ProcessTag(uint32_t TagCode, uint32_t TagLength, void* TagData);
};