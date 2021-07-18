// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ActionScriptByteCode.h"

#include "ABCStream.h"

ActionScriptByteCode::ActionScriptByteCode(BYTE *ByteCodeData, const SIZE_T ByteCodeLength)
{
	ABCStream Stream(ByteCodeData, ByteCodeLength);

	SetConsoleOutputCP(CP_UTF8);

	uint16_t MinorVersion = Stream.Read<uint16_t>();
	uint16_t MajorVersion = Stream.Read<uint16_t>();

	uint32_t IntCount = Stream.ReadEncodedU30();

	if (IntCount > 0)
	{
		for (uint32_t i = 0; i < IntCount - 1; i++)
		{
			ConstantPool.SignedIntegers.Add(Stream.ReadEncodedS32());
		}
	}

	uint32_t UIntCount = Stream.ReadEncodedU30();

	uint32_t DoubleCount = Stream.ReadEncodedU30();

	uint32_t StringCount = Stream.ReadEncodedU30();

	for (uint32_t i = 0; i < StringCount - 1; i++)
	{
		uint32_t StringLength = Stream.ReadEncodedU30();

		ConstantPool.Strings.Add(String((char*)Stream.GetData() + Stream.GetPosition(), StringLength));

		Stream.Advane(StringLength);
	}

	uint32_t NamespaceCount = Stream.ReadEncodedU30();

	for (uint32_t i = 0; i < NamespaceCount - 1; i++)
	{
		uint8_t Kind = Stream.Read<uint8_t>();
		uint32_t Name = Stream.ReadEncodedU30();

		ConstantPool.Namespaces.Add(ActionScriptNamespace{ (NamespaceKind)Kind, Name > 0 ? (char*)ConstantPool.Strings[Name - 1].GetData() : nullptr });
	}

	uint32_t NsSetCount = Stream.ReadEncodedU30();

	for (uint32_t i = 0; i < NsSetCount - 1; i++)
	{
		uint32_t Count = Stream.ReadEncodedU30();

		ActionScriptNamespaceSet NamespaceSet;

		for (uint32_t j = 0; j < Count; j++)
		{
			uint32_t ns = Stream.ReadEncodedU30();

			NamespaceSet.Namespaces.Add(&ConstantPool.Namespaces[ns - 1]);
		}

		ConstantPool.NamespaceSets.Add(NamespaceSet);
	}

	uint32_t MultiNameCount = Stream.ReadEncodedU30();

	for (uint32_t i = 0; i < MultiNameCount - 1; i++)
	{
		MultiNameKind Kind = (MultiNameKind)Stream.Read<uint8_t>();

		uint32_t Namespace;
		uint32_t Name;
		uint32_t NamespaceSet;

		switch (Kind)
		{
			case MultiNameKind::QName:
			case MultiNameKind::QNameA:
				Namespace = Stream.ReadEncodedU30();
				Name = Stream.ReadEncodedU30();
				if (ConstantPool.Namespaces[Namespace - 1].Name)
					cout << ConstantPool.Namespaces[Namespace - 1].Name << ".";
				cout << ConstantPool.Strings[Name - 1] << endl;
				ConstantPool.MultiNames.Add(ActionScriptMultiName());
				ConstantPool.MultiNames[ConstantPool.MultiNames.GetLength() - 1].Kind = Kind;
				ConstantPool.MultiNames[ConstantPool.MultiNames.GetLength() - 1].QName.Name = (char*)ConstantPool.Strings[Name - 1].GetData();
				ConstantPool.MultiNames[ConstantPool.MultiNames.GetLength() - 1].QName.Namespace = &ConstantPool.Namespaces[Namespace - 1];
				break;
			case MultiNameKind::RTQName:
			case MultiNameKind::RTQNameA:
				Name = Stream.ReadEncodedU30();
				ConstantPool.MultiNames.Add(ActionScriptMultiName());
				ConstantPool.MultiNames[ConstantPool.MultiNames.GetLength() - 1].Kind = Kind;
				ConstantPool.MultiNames[ConstantPool.MultiNames.GetLength() - 1].RTQName.Name = (char*)ConstantPool.Strings[Name - 1].GetData();
				break;
			case MultiNameKind::RTQNameL:
			case MultiNameKind::RTQNameLA:
				break;
			case MultiNameKind::MultiName:
			case MultiNameKind::MultiNameA:
				Name = Stream.ReadEncodedU30();
				NamespaceSet = Stream.ReadEncodedU30();
				cout << ConstantPool.Strings[Name - 1] << endl;
				ConstantPool.MultiNames.Add(ActionScriptMultiName());
				ConstantPool.MultiNames[ConstantPool.MultiNames.GetLength() - 1].Kind = Kind;
				ConstantPool.MultiNames[ConstantPool.MultiNames.GetLength() - 1].MultiName.Name = (char*)ConstantPool.Strings[Name - 1].GetData();
				ConstantPool.MultiNames[ConstantPool.MultiNames.GetLength() - 1].MultiName.NamespaceSet = &ConstantPool.NamespaceSets[NamespaceSet - 1];
				break;
			case MultiNameKind::MultiNameL:
			case MultiNameKind::MultiNameLA:
				NamespaceSet = Stream.ReadEncodedU30();
				ConstantPool.MultiNames.Add(ActionScriptMultiName());
				ConstantPool.MultiNames[ConstantPool.MultiNames.GetLength() - 1].Kind = Kind;
				ConstantPool.MultiNames[ConstantPool.MultiNames.GetLength() - 1].MultiNameL.NamespaceSet = &ConstantPool.NamespaceSets[NamespaceSet - 1];
				break;
		}
	}

	uint32_t MethodCount = Stream.ReadEncodedU30();

	for (uint32_t i = 0; i < MethodCount; i++)
	{
		ActionScriptMethod ASMethod;

		ASMethod.ParamCount = Stream.ReadEncodedU30();
		ASMethod.ReturnType = Stream.ReadEncodedU30();
		for (uint32_t j = 0; j < ASMethod.ParamCount; j++)
		{
			ASMethod.ParamTypes.Add(Stream.ReadEncodedU30());
		}
		ASMethod.Name = Stream.ReadEncodedU30();
		ASMethod.Flags = Stream.Read<uint8_t>();

		Methods.Add(ASMethod);
	}

	uint32_t MetaDataCount = Stream.ReadEncodedU30();

	for (uint32_t i = 0; i < MetaDataCount; i++)
	{
		uint32_t Name = Stream.ReadEncodedU30();
		uint32_t ItemCount = Stream.ReadEncodedU30();

		for (uint32_t j = 0; j < ItemCount; j++)
		{
			uint32_t Key = Stream.ReadEncodedU30();
			uint32_t Value = Stream.ReadEncodedU30();
		}
	}

	uint32_t ClassCount = Stream.ReadEncodedU30();

	for (uint32_t i = 0; i < ClassCount; i++)
	{
		uint32_t Name = Stream.ReadEncodedU30();
		uint32_t SuperName = Stream.ReadEncodedU30();
		cout << "Class instance: " << endl;
		cout << ConstantPool.MultiNames[Name - 1].QName.Namespace->Name << "." << ConstantPool.MultiNames[Name - 1].QName.Name << endl;
		cout << ConstantPool.MultiNames[SuperName - 1].QName.Namespace->Name << "." << ConstantPool.MultiNames[SuperName - 1].QName.Name << endl;
		uint8_t Flags = Stream.Read<uint8_t>();
		uint32_t ProtectedNameSpace = Stream.ReadEncodedU30();
		uint32_t InterfacesCount = Stream.ReadEncodedU30();
		for (uint32_t i = 0; i < InterfacesCount; i++)
		{

		}
		uint32_t InstanceInitializer = Stream.ReadEncodedU30();
		uint32_t TraitCount = Stream.ReadEncodedU30();
		cout << "Traits count: " << TraitCount << endl;
		for (uint32_t i = 0; i < TraitCount; i++)
		{
			uint32_t TraitName = Stream.ReadEncodedU30();
			if (ConstantPool.MultiNames[TraitName - 1].QName.Namespace->Name)
				cout << ConstantPool.MultiNames[TraitName - 1].QName.Namespace->Name << ".";
			cout << ConstantPool.MultiNames[TraitName - 1].QName.Name << endl;
			uint8_t TraitKind = Stream.Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			cout << "Trait type: " << +TraitType << endl;
			switch (TraitType)
			{
				case 0:
				case 6:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					if (Stream.ReadEncodedU30() > 0)
					{
						Stream.Read<uint8_t>();
					}
					break;
				case 5:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					break;
				case 1:
				case 2:
				case 3:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					break;
			}
			if (((TraitKind & 0b00001111) >> 4) == 0) continue;
			uint32_t MetaDataCount = Stream.ReadEncodedU30();
			for (uint32_t j = 0; j < MetaDataCount; j++)
			{

			}
		}
		cout << "================================================" << endl;
	}

	for (uint32_t i = 0; i < ClassCount; i++)
	{
		cout << "Class: " << endl;
		uint32_t ClassInitializer = Stream.ReadEncodedU30();
		uint32_t TraitCount = Stream.ReadEncodedU30();
		cout << "Traits count: " << TraitCount << endl;
		for (uint32_t i = 0; i < TraitCount; i++)
		{
			uint32_t TraitName = Stream.ReadEncodedU30();
			cout << ConstantPool.MultiNames[TraitName - 1].QName.Namespace->Name << "." << ConstantPool.MultiNames[TraitName - 1].QName.Name << endl;
			uint8_t TraitKind = Stream.Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			cout << "Trait type: " << +TraitType << endl;
			switch (TraitType)
			{
				case 0:
				case 6:

					break;
				case 5:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					break;
				case 1:
				case 2:
				case 3:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					break;
			}
			if (((TraitKind & 0b00001111) >> 4) == 0) continue;
			uint32_t MetaDataCount = Stream.ReadEncodedU30();
			for (uint32_t j = 0; j < MetaDataCount; j++)
			{

			}
		}
		cout << "================================================" << endl;
	}

	uint32_t ScriptCount = Stream.ReadEncodedU30();

	for (uint32_t i = 0; i < ScriptCount; i++)
	{
		cout << "Script: " << endl;
		uint32_t ScriptInitializer = Stream.ReadEncodedU30();
		uint32_t TraitCount = Stream.ReadEncodedU30();
		cout << "Traits count: " << TraitCount << endl;
		for (uint32_t i = 0; i < TraitCount; i++)
		{
			uint32_t TraitName = Stream.ReadEncodedU30();
			cout << ConstantPool.MultiNames[TraitName - 1].QName.Namespace->Name << "." << ConstantPool.MultiNames[TraitName - 1].QName.Name << endl;
			uint8_t TraitKind = Stream.Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			cout << "Trait type: " << +TraitType << endl;
			switch (TraitType)
			{
				case 0:
				case 6:

					break;
				case 4:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					break;
				case 5:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					break;
				case 1:
				case 2:
				case 3:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					break;
			}
			if (((TraitKind & 0b00001111) >> 4) == 0) continue;
			uint32_t MetaDataCount = Stream.ReadEncodedU30();
			for (uint32_t j = 0; j < MetaDataCount; j++)
			{

			}
		}
		cout << "================================================" << endl;
	}
	uint32_t MethodBodyCount = Stream.ReadEncodedU30();
	for (uint32_t i = 0; i < MethodBodyCount; i++)
	{
		uint32_t Method = Stream.ReadEncodedU30();
		uint32_t MaxStack = Stream.ReadEncodedU30();
		uint32_t LocalCount = Stream.ReadEncodedU30();
		uint32_t InitScopeDepth = Stream.ReadEncodedU30();
		uint32_t MaxScopeDepth = Stream.ReadEncodedU30();
		uint32_t CodeLength = Stream.ReadEncodedU30();

		ABCStream MethodCodeStream(Stream.GetData() + Stream.GetPosition(), CodeLength);

		cout << "Begin method " << Method << endl;

		while (true)
		{
			if (MethodCodeStream.IsEndOfStream()) break;
			
			OpCode opCode = (OpCode)MethodCodeStream.Read<uint8_t>();

			cout << "\t";

			if (OpCodeNames[(uint8_t)opCode] != nullptr)
			{
				cout << OpCodeNames[(uint8_t)opCode];
			}
			else
			{
				cout << +(uint8_t)opCode;
			}

			if (opCode == OpCode::PushByte)
			{
				cout << " " << +MethodCodeStream.Read<uint8_t>();
			}
			if (opCode == OpCode::PushString)
			{
				uint32_t Operand = MethodCodeStream.ReadEncodedU30();

				cout << " \"" << ConstantPool.Strings[Operand - 1] << "\"";
			}
			if (opCode == OpCode::CallProperty || opCode == OpCode::CallPropVoid)
			{
				uint32_t Operand = MethodCodeStream.ReadEncodedU30();

				cout << " \"" << ConstantPool.MultiNames[Operand - 1].QName.Namespace->Name << "." << ConstantPool.MultiNames[Operand - 1].QName.Name << "\"";

				Operand = MethodCodeStream.ReadEncodedU30();
			}
			if (opCode == OpCode::SetProperty || opCode == OpCode::GetProperty || opCode == OpCode::InitProperty || opCode == OpCode::FindPropStrict || opCode == OpCode::GetLex)
			{
				uint32_t Operand = MethodCodeStream.ReadEncodedU30();

				if (ConstantPool.MultiNames[Operand - 1].Kind == MultiNameKind::QName)
				{
					if (ConstantPool.MultiNames[Operand - 1].QName.Namespace->Name)
						cout << " \"" << ConstantPool.MultiNames[Operand - 1].QName.Namespace->Name << ".";
					cout << ConstantPool.MultiNames[Operand - 1].QName.Name << "\"";
				}
			}
			if (opCode == OpCode::NewFunction || opCode == OpCode::NewCatch || opCode == OpCode::SetSlot)
			{
				uint32_t Operand = MethodCodeStream.ReadEncodedU30();

				cout << " " << Operand;
			}
			if (opCode == OpCode::ConstructSuper || opCode == OpCode::GetScopeObject || opCode == OpCode::NewClass)
			{
				uint32_t Operand = MethodCodeStream.ReadEncodedU30();
			}
			if (opCode == OpCode::IfFalse || opCode == OpCode::IfTrue || opCode == OpCode::IfNgt)
			{
				int32_t Operand = MethodCodeStream.ReadS24();
			}

			cout << endl;
		}

		Stream.Advane(CodeLength);

		uint32_t ExceptionCount = Stream.ReadEncodedU30();

		for (uint32_t j = 0; j < ExceptionCount; j++)
		{
			Stream.ReadEncodedU30();
			Stream.ReadEncodedU30();
			Stream.ReadEncodedU30();
			Stream.ReadEncodedU30();
			Stream.ReadEncodedU30();
		}

		uint32_t TraitCount = Stream.ReadEncodedU30();
		cout << "Traits count: " << TraitCount << endl;
		for (uint32_t j = 0; j < TraitCount; j++)
		{
			uint32_t TraitName = Stream.ReadEncodedU30();
			if (ConstantPool.MultiNames[TraitName - 1].QName.Namespace && ConstantPool.MultiNames[TraitName - 1].QName.Namespace->Name)
				cout << "Trait name: " << ConstantPool.MultiNames[TraitName - 1].QName.Namespace->Name << ".";
			cout << ConstantPool.MultiNames[TraitName - 1].QName.Name << endl;
			uint8_t TraitKind = Stream.Read<uint8_t>();
			uint8_t TraitType = TraitKind & 0b1111;
			cout << "Trait type: " << +TraitType << endl;
			switch (TraitType)
			{
				case 0:
				case 6:

					break;
				case 4:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					break;
				case 5:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					break;
				case 1:
				case 2:
				case 3:
					Stream.ReadEncodedU30();
					Stream.ReadEncodedU30();
					break;
			}
			if (((TraitKind & 0b00001111) >> 4) == 0) continue;
			uint32_t MetaDataCount = Stream.ReadEncodedU30();
			for (uint32_t j = 0; j < MetaDataCount; j++)
			{

			}
		}
		cout << "End method " << Method << endl;
	}
}