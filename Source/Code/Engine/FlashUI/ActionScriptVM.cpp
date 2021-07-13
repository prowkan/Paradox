// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ActionScriptVM.h"

#include <Containers/DynamicArray.h>

BYTE* ActionScriptVM::ByteCodeData;

SIZE_T ActionScriptVM::CurrentByte;
SIZE_T ActionScriptVM::CurrentBit;

uint32_t ActionScriptVM::ReadEncodedU30()
{
	/*uint32_t Value = 0;

	for (int i = 0; i < 5; i++)
	{
		uint8_t Byte = Read<uint8_t>();

		Value |= ((Byte & (0b01111111)) << (i * 7));

		if (((Byte & 0b10000000) >> 7) == 0) break;
	}

	return Value;*/

	return ReadEncodedU32() & 0b00111111111111111111111111111111;
}

uint32_t ActionScriptVM::ReadEncodedU32()
{
	uint32_t Value = 0;

	for (int i = 0; i < 5; i++)
	{
		uint8_t Byte = Read<uint8_t>();

		Value |= ((Byte & (0b01111111)) << (i * 7));

		if (((Byte & 0b10000000) >> 7) == 0) break;
	}

	return Value;
}

void ActionScriptVM::ParseASByteCode(BYTE* ByteCodeData, SIZE_T ByteCodeLength)
{
	ActionScriptVM::ByteCodeData = ByteCodeData;

	CurrentByte = 0;
	CurrentBit = 0;

	uint16_t MinorVersion = Read<uint16_t>();
	uint16_t MajorVersion = Read<uint16_t>();

	uint32_t IntCount = ReadEncodedU30();

	uint32_t UIntCount = ReadEncodedU30();

	uint32_t DoubleCount = ReadEncodedU30();

	uint32_t StringCount = ReadEncodedU30();

	DynamicArray<char*> StringsArray;

	for (uint32_t i = 0; i < StringCount - 1; i++)
	{
		uint32_t StringLength = ReadEncodedU30();

		//char* String = (char*)(ByteCodeData + CurrentByte);

		char* String = new char[StringLength + 1];
		memcpy(String, ByteCodeData + CurrentByte, StringLength);
		String[StringLength] = 0;

		StringsArray.Add(String);

		CurrentByte += StringLength;
	}

	uint32_t NamespaceCount = ReadEncodedU30();

	struct ASNamespace
	{
		uint8_t Kind;
		uint32_t Name;
	};

	DynamicArray<ASNamespace> Namespaces;

	for (uint32_t i = 0; i < NamespaceCount - 1; i++)
	{
		uint8_t Kind = Read<uint8_t>();
		uint32_t Name = ReadEncodedU30();

		cout << "Namespace: " << StringsArray[Name - 1] << endl;

		Namespaces.Add({ Kind, Name });
	}

	uint32_t NsSetCount = ReadEncodedU30();

	for (uint32_t i = 0; i < NsSetCount - 1; i++)
	{
		uint32_t Count = ReadEncodedU30();

		cout << "Namespace set:" << endl;

		for (uint32_t j = 0; j < Count; j++)
		{
			uint32_t ns = ReadEncodedU30();
			//cout << ns << endl;
			cout << StringsArray[Namespaces[ns - 1].Name - 1] << endl;
		}

		cout << "============" << endl;
	}

	uint32_t MultiNameCount = ReadEncodedU30();

	struct ASMultiName
	{
		uint32_t Kind;
		uint32_t u1;
		uint32_t u2;
	};

	DynamicArray<ASMultiName> MultiNames;

	for (uint32_t i = 0; i < MultiNameCount - 1; i++)
	{
		uint8_t Kind = Read<uint8_t>();
		cout << +Kind << " ";

		uint32_t ns;
		uint32_t Name;
		uint32_t ns_set;

		switch (Kind)
		{
			case 0x07:
			case 0x0D:
				ns = ReadEncodedU30();
				Name = ReadEncodedU30();
				cout << StringsArray[Namespaces[ns - 1].Name - 1] << "." << StringsArray[Name - 1] << endl;
				MultiNames.Add({ Kind, ns, Name });
				break;
			case 0x0F:
			case 0x1D:
				Name = ReadEncodedU30();
				break;
			case 0x11:
			case 0x12:
				break;
			case 0x09:
			case 0x0E:
				Name = ReadEncodedU30();
				ns_set = ReadEncodedU30();
				cout << StringsArray[Name - 1] << endl;
				MultiNames.Add({ Kind, Name, ns_set });
				break;
			case 0x1B:
			case 0x1C:
				ns_set = ReadEncodedU30();
				break;
		}
	}

	uint32_t MethodCount = ReadEncodedU30();

	for (uint32_t i = 0; i < MethodCount; i++)
	{
		uint32_t ParamCount = ReadEncodedU30();
		uint32_t ReturnType = ReadEncodedU30();
		for (uint32_t j = 0; j < ParamCount; j++)
		{
			uint32_t ParamType = ReadEncodedU30();
		}
		uint32_t Name = ReadEncodedU30();
		uint8_t Flags = Read<uint8_t>();
	}

	uint32_t MetaDataCount = ReadEncodedU30();

	for (uint32_t i = 0; i < MetaDataCount; i++)
	{
		uint32_t Name = ReadEncodedU30();
		uint32_t ItemCount = ReadEncodedU30();

		for (uint32_t j = 0; j < ItemCount; j++)
		{
			uint32_t Key = ReadEncodedU30();
			uint32_t Value = ReadEncodedU30();
		}
	}

	uint32_t ClassCount = ReadEncodedU30();

	for (uint32_t i = 0; i < ClassCount; i++)
	{
		uint32_t Name = ReadEncodedU30();
		uint32_t SuperName = ReadEncodedU30();
		uint8_t Flags = Read<uint8_t>();
		uint32_t ProtectedNameSpace = ReadEncodedU30();
		uint32_t InterfacesCount = ReadEncodedU30();
		for (uint32_t i = 0; i < InterfacesCount; i++)
		{

		}
		uint32_t InstanceInitializer = ReadEncodedU30();
		uint32_t TraitCount = ReadEncodedU30();
		for (uint32_t i = 0; i < TraitCount; i++)
		{
			uint32_t TraitName = ReadEncodedU30();
			uint8_t TraitKind = Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			switch (TraitType)
			{
				case 0:
				case 6:
					ReadEncodedU30();
					ReadEncodedU30();
					if (ReadEncodedU30() > 0)
					{
						Read<uint8_t>();
					}
					break;
				case 5:
					ReadEncodedU30();
					ReadEncodedU30();
					break;
				case 1:
				case 2:
				case 3:
					ReadEncodedU30();
					ReadEncodedU30();
					break;
			}
			if (((TraitKind & 0b00001111) >> 4) == 0) continue;
			uint32_t MetaDataCount = ReadEncodedU30();
			for (uint32_t j = 0; j < MetaDataCount; j++)
			{

			}
		}
	}
	for (uint32_t i = 0; i < ClassCount; i++)
	{
		uint32_t ClassInitializer = ReadEncodedU30();
		uint32_t TraitCount = ReadEncodedU30();
		for (uint32_t i = 0; i < TraitCount; i++)
		{
			uint32_t TraitName = ReadEncodedU30();
			uint8_t TraitKind = Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			switch (TraitType)
			{
			case 0:
			case 6:

				break;
			case 5:
				ReadEncodedU30();
				ReadEncodedU30();
				break;
			case 1:
			case 2:
			case 3:
				ReadEncodedU30();
				ReadEncodedU30();
				break;
			}
			if (((TraitKind & 0b00001111) >> 4) == 0) continue;
			uint32_t MetaDataCount = ReadEncodedU30();
			for (uint32_t j = 0; j < MetaDataCount; j++)
			{

			}
		}
	}
	uint32_t ScriptCount = ReadEncodedU30();
	for (uint32_t i = 0; i < ScriptCount; i++)
	{
		uint32_t ScriptInitializer = ReadEncodedU30();
		uint32_t TraitCount = ReadEncodedU30();
		for (uint32_t i = 0; i < TraitCount; i++)
		{
			uint32_t TraitName = ReadEncodedU30();
			uint8_t TraitKind = Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			switch (TraitType)
			{
			case 0:
			case 6:

				break;
			case 4:
				ReadEncodedU30();
				ReadEncodedU30();
				break;
			case 5:
				ReadEncodedU30();
				ReadEncodedU30();
				break;
			case 1:
			case 2:
			case 3:
				ReadEncodedU30();
				ReadEncodedU30();
				break;
			}
			if (((TraitKind & 0b00001111) >> 4) == 0) continue;
			uint32_t MetaDataCount = ReadEncodedU30();
			for (uint32_t j = 0; j < MetaDataCount; j++)
			{

			}
		}
	}
	uint32_t MethodBodyCount = ReadEncodedU30();
	for (uint32_t i = 0; i < MethodBodyCount; i++)
	{
		uint32_t Method = ReadEncodedU30();
		uint32_t MaxStack = ReadEncodedU30();
		uint32_t LocalCount = ReadEncodedU30();
		uint32_t InitScopeDepth = ReadEncodedU30();
		uint32_t MaxScopeDepth = ReadEncodedU30();
		uint32_t CodeLength = ReadEncodedU30();
		
		uint8_t* Code = ByteCodeData + CurrentByte;

		uint32_t CodePointer = 0;

		cout << "Begin method " << Method << endl;

		while (true)
		{
			if (CodePointer >= CodeLength) break;

			uint8_t OpCode = Code[CodePointer];

			CodePointer++;

			switch (OpCode)
			{
				case 0xD0:
					cout << "getlocal_0" << endl;
					break;
				case 0x30:
					cout << "pushscope" << endl;
					break;
				case 0x47:
					cout << "returnvoid" << endl;
					break;
				case 0x5D:
					cout << "findpropstrict" << endl;
					break;
				case 0x60:
					cout << "getlex" << endl;
					break;
				case 0x1D:
					cout << "popscope" << endl;
					break;
				case 0x2C:
					cout << "pushstring" << endl;
					break;
				case 0x4F:
					cout << "callpropvoid" << endl;
					break;
				case 0x57:
					cout << "newactivation" << endl;
					break;
				case 0x2A:
					cout << "dup" << endl;
					break;
				case 0xD5:
					cout << "setlocal_1" << endl;
					break;
				case 0x49:
					cout << "constructsuper" << endl;
					break;
				case 0x24:
					cout << "pushbyte" << endl;
					break;
				case 0x66:
					cout << "getproperty" << endl;
					break;
				case 0x40:
					cout << "newfunction" << endl;
					break;
				case 0x65:
					cout << "getscopeobject" << endl;
					break;
				case 0x58:
					cout << "newclass" << endl;
					break;
				case 0x68:
					cout << "initproperty" << endl;
					break;
				case 0x46:
					cout << "callproperty" << endl;
					break;
				case 0x61:
					cout << "setproperty" << endl;
					break;
				default:
					cout << (uint32_t)OpCode << endl;
					break;
			}

			if (OpCode == 0x24)
			{
				CodePointer++;
			}
			if (OpCode == 0x5D || OpCode == 0x60 || OpCode == 0x2C || OpCode == 0x49 || OpCode == 0x66 || OpCode == 0x40 || OpCode == 0x65 || OpCode == 0x58 || OpCode == 0x68 || OpCode == 0x61)
			{
				uint8_t Operand = Code[CodePointer];
				CodePointer++;
				if (Operand & 0b1000000)
				{
					Operand = Code[CodePointer];
					CodePointer++;
					if (Operand & 0b1000000)
					{
						Operand = Code[CodePointer];
						CodePointer++;
						if (Operand & 0b1000000)
						{
							Operand = Code[CodePointer];
							CodePointer++;
							if (Operand & 0b1000000)
							{
								Operand = Code[CodePointer];
								CodePointer++;
							}
						}
					}
				}
			}
			if (OpCode == 0x4F || OpCode == 0x46)
			{
				uint8_t Operand = Code[CodePointer];
				CodePointer++;
				if (Operand & 0b1000000)
				{
					Operand = Code[CodePointer];
					CodePointer++;
					if (Operand & 0b1000000)
					{
						Operand = Code[CodePointer];
						CodePointer++;
						if (Operand & 0b1000000)
						{
							Operand = Code[CodePointer];
							CodePointer++;
							if (Operand & 0b1000000)
							{
								Operand = Code[CodePointer];
								CodePointer++;
							}
						}
					}
				}
				Operand = Code[CodePointer];
				CodePointer++;
				if (Operand & 0b1000000)
				{
					Operand = Code[CodePointer];
					CodePointer++;
					if (Operand & 0b1000000)
					{
						Operand = Code[CodePointer];
						CodePointer++;
						if (Operand & 0b1000000)
						{
							Operand = Code[CodePointer];
							CodePointer++;
							if (Operand & 0b1000000)
							{
								Operand = Code[CodePointer];
								CodePointer++;
							}
						}
					}
				}
			}
		}
		
		cout << "End method " << Method << endl;

		CurrentByte += CodeLength;
		uint32_t ExceptionCount = ReadEncodedU30();
		uint32_t TraitCount = ReadEncodedU30();
		for (uint32_t j = 0; j < TraitCount; j++)
		{
			uint32_t TraitName = ReadEncodedU30();
			uint8_t TraitKind = Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			switch (TraitType)
			{
			case 0:
			case 6:

				break;
			case 4:
				ReadEncodedU30();
				ReadEncodedU30();
				break;
			case 5:
				ReadEncodedU30();
				ReadEncodedU30();
				break;
			case 1:
			case 2:
			case 3:
				ReadEncodedU30();
				ReadEncodedU30();
				break;
			}
			if (((TraitKind & 0b00001111) >> 4) == 0) continue;
			uint32_t MetaDataCount = ReadEncodedU30();
			for (uint32_t j = 0; j < MetaDataCount; j++)
			{

			}
		}
	}
}