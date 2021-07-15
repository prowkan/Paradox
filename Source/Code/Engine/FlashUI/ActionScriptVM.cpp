// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ActionScriptVM.h"

#include <Containers/DynamicArray.h>

BYTE* ActionScriptVM::ByteCodeData;

SIZE_T ActionScriptVM::CurrentByte;
SIZE_T ActionScriptVM::CurrentBit;

uint32_t ActionScriptVM::ReadEncodedU30()
{
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
	SetConsoleOutputCP(CP_UTF8);

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
		cout << "Class instance: " << endl;
		cout << StringsArray[Namespaces[MultiNames[Name - 1].u1 - 1].Name - 1] << "." << StringsArray[MultiNames[Name - 1].u2 - 1] << endl;
		cout << StringsArray[Namespaces[MultiNames[SuperName - 1].u1 - 1].Name - 1] << "." << StringsArray[MultiNames[SuperName - 1].u2 - 1] << endl;
		uint8_t Flags = Read<uint8_t>();
		uint32_t ProtectedNameSpace = ReadEncodedU30();
		uint32_t InterfacesCount = ReadEncodedU30();
		for (uint32_t i = 0; i < InterfacesCount; i++)
		{

		}
		uint32_t InstanceInitializer = ReadEncodedU30();
		uint32_t TraitCount = ReadEncodedU30();
		cout << "Traits count: " << TraitCount << endl;
		for (uint32_t i = 0; i < TraitCount; i++)
		{
			uint32_t TraitName = ReadEncodedU30();
			cout << "Trait name: " << StringsArray[Namespaces[MultiNames[TraitName - 1].u1 - 1].Name - 1] << "." << StringsArray[MultiNames[TraitName - 1].u2 - 1] << endl;
			uint8_t TraitKind = Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			cout << "Trait type: " << +TraitType << endl;
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
		cout << "================================================" << endl;
	}

	for (uint32_t i = 0; i < ClassCount; i++)
	{
		cout << "Class: " << endl;
		uint32_t ClassInitializer = ReadEncodedU30();
		uint32_t TraitCount = ReadEncodedU30();
		cout << "Traits count: " << TraitCount << endl;
		for (uint32_t i = 0; i < TraitCount; i++)
		{
			uint32_t TraitName = ReadEncodedU30();
			cout << "Trait name: " << StringsArray[Namespaces[MultiNames[TraitName - 1].u1 - 1].Name - 1] << "." << StringsArray[MultiNames[TraitName - 1].u2 - 1] << endl;
			uint8_t TraitKind = Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			cout << "Trait type: " << +TraitType << endl;
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
		cout << "================================================" << endl;
	}

	uint32_t ScriptCount = ReadEncodedU30();

	for (uint32_t i = 0; i < ScriptCount; i++)
	{
		cout << "Script: " << endl;
		uint32_t ScriptInitializer = ReadEncodedU30();
		uint32_t TraitCount = ReadEncodedU30();
		cout << "Traits count: " << TraitCount << endl;
		for (uint32_t i = 0; i < TraitCount; i++)
		{
			uint32_t TraitName = ReadEncodedU30();
			cout << "Trait name: " << StringsArray[Namespaces[MultiNames[TraitName - 1].u1 - 1].Name - 1] << "." << StringsArray[MultiNames[TraitName - 1].u2 - 1] << endl;
			uint8_t TraitKind = Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			cout << "Trait type: " << +TraitType << endl;
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
		cout << "================================================" << endl;
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
					cout << "\tgetlocal_0";
					break;
				case 0xD1:
					cout << "\tgetlocal_1";
					break;
				case 0xD2:
					cout << "\tgetlocal_2";
					break;
				case 0xD3:
					cout << "\tgetlocal_3";
					break;
				case 0x30:
					cout << "\tpushscope";
					break;
				case 0x47:
					cout << "\treturnvoid";
					break;
				case 0x5D:
					cout << "\tfindpropstrict";
					break;
				case 0x60:
					cout << "\tgetlex";
					break;
				case 0x1D:
					cout << "\tpopscope";
					break;
				case 0x2C:
					cout << "\tpushstring";
					break;
				case 0x4F:
					cout << "\tcallpropvoid";
					break;
				case 0x57:
					cout << "\tnewactivation";
					break;
				case 0x2A:
					cout << "\tdup";
					break;
				case 0xD5:
					cout << "\tsetlocal_1";
					break;
				case 0x49:
					cout << "\tconstructsuper";
					break;
				case 0x24:
					cout << "\tpushbyte";
					break;
				case 0x66:
					cout << "\tgetproperty";
					break;
				case 0x40:
					cout << "\tnewfunction";
					break;
				case 0x65:
					cout << "\tgetscopeobject";
					break;
				case 0x58:
					cout << "\tnewclass";
					break;
				case 0x68:
					cout << "\tinitproperty";
					break;
				case 0x46:
					cout << "\tcallproperty";
					break;
				case 0x61:
					cout << "\tsetproperty";
					break;
				default:
					cout << (uint32_t)OpCode;
					break;
			}

			if (OpCode == 0x24)
			{
				cout << " " << +Code[CodePointer];
				CodePointer++;
			}
			if (OpCode == 0x2C)
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

				cout << " \"" << StringsArray[Operand - 1] << "\"";
			}
			if (OpCode == 0x46 || OpCode == 0x4F)
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

				cout << " \"" << StringsArray[MultiNames[Operand - 1].u2  - 1] << "\"";

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
			if (OpCode == 0x61 || OpCode == 0x66 || OpCode == 0x68 || OpCode == 0x5D || OpCode == 0x60)
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

				cout << " \"" << StringsArray[MultiNames[Operand - 1].u2 - 1] << "\"";
			}
			if (OpCode == 0x40)
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

				cout << " " << +Operand;
			}
			if (OpCode == 0x49 || OpCode == 0x65 || OpCode == 0x58 || OpCode == 0x61)
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

			cout << endl;
		}
		
		CurrentByte += CodeLength;
		uint32_t ExceptionCount = ReadEncodedU30();
		uint32_t TraitCount = ReadEncodedU30();
		cout << "Traits count: " << TraitCount << endl;
		for (uint32_t j = 0; j < TraitCount; j++)
		{
			uint32_t TraitName = ReadEncodedU30();
			cout << "Trait name: " << StringsArray[Namespaces[MultiNames[TraitName - 1].u1 - 1].Name - 1] << "." << StringsArray[MultiNames[TraitName - 1].u2 - 1] << endl;
			uint8_t TraitKind = Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			cout << "Trait type: " << +TraitType << endl;
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
		cout << "End method " << Method << endl;
	}
}