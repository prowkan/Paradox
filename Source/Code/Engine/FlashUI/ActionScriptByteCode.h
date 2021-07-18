#pragma once

#include <Containers/DynamicArray.h>
#include <Containers/String.h>

class ActionScriptByteCode
{
	public:

		enum class NamespaceKind : uint8_t
		{
			Namespace = 0x80,
			PackageNamespace = 0x16,
			PackageInternalNamespace = 0x17,
			ProtectedNamespace = 0x18,
			ExplicitNamespace = 0x19,
			StaticProtectedNamespace = 0x1A,
			PrivateNamespace = 0x05
		};

		enum class MultiNameKind : uint8_t
		{
			QName = 0x07,
			QNameA = 0x0D,
			RTQName = 0x0F,
			RTQNameA = 0x10,
			RTQNameL = 0x11,
			RTQNameLA = 0x12,
			MultiName = 0x09,
			MultiNameA = 0x0E,
			MultiNameL = 0x1B,
			MultiNameLA = 0x1C
		};

		enum class TraitType : uint8_t
		{
			TraitSlot = 0,
			TraitMethod = 1,
			TraitGetter = 2,
			TraitSetter = 3,
			TraitClass = 4,
			TraitFunction = 5,
			TraitConst = 6
		};

		struct ActionScriptNamespace
		{
			NamespaceKind Kind;
			char *Name;
		};

		struct ActionScriptNamespaceSet
		{
			DynamicArray<ActionScriptNamespace*> Namespaces;
		};
		
		struct ActionScriptMultiName
		{
			MultiNameKind Kind;
			union
			{
				struct
				{
					ActionScriptNamespace *Namespace;
					char *Name;
				} QName;
				struct
				{
					char *Name;
				} RTQName;
				struct
				{
					char *Name;
					ActionScriptNamespaceSet *NamespaceSet;
				} MultiName;
				struct
				{
					ActionScriptNamespaceSet *NamespaceSet;
				} MultiNameL;
			};
		};

		struct ActionScriptConstantPool
		{
			DynamicArray<String> Strings;
			DynamicArray<int32_t> SignedIntegers;
			DynamicArray<uint32_t> UnsignedIntegers;
			DynamicArray<double> Doubles;
			DynamicArray<ActionScriptNamespace> Namespaces;
			DynamicArray<ActionScriptNamespaceSet> NamespaceSets;
			DynamicArray<ActionScriptMultiName> MultiNames;
		};

		struct ActionScriptMethod
		{
			uint32_t ParamCount;
			uint32_t ReturnType;
			DynamicArray<uint32_t> ParamTypes;
			uint32_t Name;
			uint8_t Flags;
			uint32_t MethodCodeLength;
			BYTE *MethodCode;
		};

		struct ActionScriptTrait
		{
			TraitType Type;
		};

		struct ActionScriptClassInstanceInfo
		{
			char *ClassName;
			char *SuperClassName;
			uint8_t Flags;
			DynamicArray<ActionScriptTrait> InstanceTraits;
		};

		struct ActionScriptClassInfo
		{
			ActionScriptMethod *InitializationMethod;
		};

		struct ActionScriptClass
		{
			ActionScriptClassInstanceInfo InstanceInfo;
			ActionScriptClassInfo ClassInfo;
		};

		struct ActionScriptScript
		{

		};

		ActionScriptByteCode(BYTE *ByteCodeData, const SIZE_T ByteCodeLength);

	private:

		ActionScriptConstantPool ConstantPool;

		DynamicArray<ActionScriptMethod> Methods;
		DynamicArray<ActionScriptClass> Classes;
		DynamicArray<ActionScriptScript> Scripts;

		enum class OpCode : uint8_t
		{
			Nop = 0x02,
			Throw = 0x03,
			GetSuper = 0x04,
			SetSuper = 0x05,
			DXNS = 0x06,
			DXNSLate = 0x07,
			Kill = 0x08,
			Label = 0x09,
			IfNlt = 0x0C,
			IfNle = 0x0D,
			IfNgt = 0x0E,
			IfNge = 0x0F,
			Jump = 0x10,
			IfTrue = 0x11,
			IfFalse = 0x12,
			IfEq = 0x13,
			IfNe = 0x14,
			IfLt = 0x15,
			IfLe = 0x16,
			IfGt = 0x17,
			IfGe = 0x18,
			IfStrictEq = 0x19,
			IfStrictNe = 0x1A,
			LookUpSwitch = 0x1B,
			PopWith = 0x1C,
			PopScope = 0x1D,
			NextName = 0x1E,
			HastNext = 0x1F,
			PushNull = 0x20,
			PushUndefined = 0x21,
			NextValue = 0x23,
			PushByte = 0x24,
			PushShort = 0x25,
			PushTrue = 0x26,
			PushFalse = 0x27,
			PushNaN = 0x28,
			Pop = 0x29,
			Dup = 0x2A,
			Swap = 0x2B,
			PushString = 0x2C,
			PushInt = 0x2D,
			PushUInt = 0x2E,
			PushDouble = 0x2F,
			PushScope = 0x30,
			PushNamespace = 0x31,
			HasNext2 = 0x32,
			NewFunction = 0x40,
			Call = 0x41,
			Construct = 0x42,
			CallMethod = 0x43,
			CallStatic = 0x44,
			CallProperty = 0x46,
			ReturnVoid = 0x47,
			ReturnValue = 0x48,
			ConstructSuper = 0x49,
			ConstructProp = 0x4A,
			CallPropLex = 0x4C,
			CallSuper = 0x4E,
			CallPropVoid = 0x4F,
			NewObject = 0x55,
			NewArray = 0x56,
			NewActivation = 0x57,
			NewClass = 0x58,
			GetDescendants = 0x59,
			NewCatch = 0x5A,
			FindPropStrict = 0x5D,
			FindProperty = 0x5E,
			GetLex = 0x60,
			SetProperty = 0x61,
			GetLocal = 0x62,
			SetLocal = 0x63,
			GetGlobalScope = 0x64,
			GetScopeObject = 0x65,
			GetProperty = 0x66,
			InitProperty = 0x68,
			DeleteProperty = 0x6A,
			GetSlot = 0x6C,
			SetSlot = 0x6D,
			GetGlobalSlot = 0x6E,
			SetGlobalSlot = 0x6F,
			Convert_s = 0x70,
			EscXElem = 0x71,
			EscXAttr = 0x72,
			Convert_i = 0x73,
			Convert_u = 0x74,
			Convert_d = 0x75,
			Convert_b = 0x76,
			Convert_o = 0x77,
			CheckFilter = 0x78,
			Coerce = 0x80,
			Coerce_a = 0x82,
			Coerce_s = 0x85,
			AsType = 0x86,
			AsTypeLate = 0x87,
			Increment = 0xC1,
			Negate = 0x90,
			InLocal = 0x92,
			Decrement = 0x93,
			DecLocal = 0x94,
			TypeOf = 0x95,
			Not = 0x96,
			BitNot = 0x97,
			Add = 0xA0,
			Subtract = 0xA1,
			Multiply = 0xA2,
			Divide = 0xA3,
			Modulo = 0xA4,
			LShift = 0xA5,
			RShift = 0xA6,
			URShift = 0xA7,
			BitAnd = 0xA8,
			BitOr = 0xA9,
			BitXor = 0xAA,
			Equals = 0xAB,
			StrictEquals = 0xAC,
			LessThan = 0xAD,
			LessEquals = 0xAE,
			GreaterThan = 0xAF,
			GreaterEquals = 0xB0,
			InstanceOf = 0xB1,
			IsType = 0xB2,
			IsTypeLate = 0xB3,
			In = 0xB4,
			Increment_i = 0xC0,
			Decrement_i = 0xC1,
			InLocal_i = 0xC2,
			DecLocal_i = 0xC3,
			Negate_i = 0xC4,
			Add_i = 0xC5,
			Subtract_i = 0xC6,
			Multiply_i = 0xC7,
			GetLocal_0 = 0xD0,
			GetLocal_1 = 0xD1,
			GetLocal_2 = 0xD2,
			GetLocal_3 = 0xD3,
			SetLocal_0 = 0xD4,
			SetLocal_1 = 0xD5,
			SetLocal_2 = 0xD6,
			SetLocal_3 = 0xD7,
			Debug = 0xEF,
			DebugLine = 0xF0,
			DebugFile = 0xF1
		};

		const char* OpCodeNames[255] = 
		{
			nullptr,
			nullptr,
			"nop",
			"throw",
			"getsuper",
			"setsuper",
			"dxns",
			"dxnslate",
			"kill",
			"label",
			nullptr,
			nullptr,
			"ifnlt",
			"ifnle",
			"ifngt",
			"ifnge",
			"jump",
			"iftrue",
			"iffalse",
			"ifeq",
			"ifne",
			"iflt",
			"ifle",
			"ifgt",
			"ifge",
			"ifstricteq",
			"ifstrictne",
			"lookupswitch",
			"popwith",
			"popscope",
			"nextname",
			"hastnext",
			"pushnull",
			"pushundefined",
			nullptr,
			"nextvalue",
			"pushbyte",
			"pushshort",
			"pushtrue",
			"pushfalse",
			"pushnan",
			"pop",
			"dup",
			"swap",
			"pushstring",
			"pushint",
			"pushuInt",
			"pushdouble",
			"pushscope",
			"pushnamespace",
			"hasnext2",
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
			"newfunction",
			"call",
			"construct",
			"callmethod",
			"callstatic",
			nullptr,
			"callproperty",
			"returnvoid",
			"returnvalue",
			"constructsuper",
			"constructprop",
			nullptr,
			"callproplex",
			nullptr,
			"callsuper",
			"callpropvoid",
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			"newobject",
			"newarray",
			"newactivation",
			"newclass",
			"getdescendants",
			"newcatch",
			nullptr,
			nullptr,
			"findpropstrict",
			"findproperty",
			nullptr,
			"getlex",
			"setproperty",
			"getlocal",
			"setlocal",
			"getglobalscope",
			"getscopeobject",
			"getproperty",
			nullptr,
			"initproperty",
			nullptr,
			"deleteproperty",
			nullptr,
			"getslot",
			"setslot",
			"getglobalslot",
			"setglobalslot",
			"convert_s",
			"escxelem",
			"escxattr",
			"convert_i",
			"convert_u",
			"convert_d",
			"convert_b",
			"convert_o",
			"checkfilter",
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			"coerce",
			nullptr,
			"coerce_a",
			nullptr,
			nullptr,
			"coerce_s",
			"astype",
			"astypelate",
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			"negate",
			"increment",
			"inlocal",
			"decrement",
			"declocal",
			"typeof",
			"not",
			"bitnot",
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			"add",
			"subtract",
			"multiply",
			"divide",
			"modulo",
			"lshift",
			"rshift",
			"urshift",
			"bitand",
			"bitor",
			"bitxor",
			"equals",
			"strictequals",
			"lessthan",
			"lessequals",
			"greaterthan",
			"greaterequals",
			"instanceof",
			"istype",
			"istypelate",
			"in",
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
			"increment_i",
			"decrement_i",
			"inlocal_i",
			"declocal_i",
			"negate_i",
			"add_i",
			"subtract_i",
			"multiply_i",
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			"getlocal_0",
			"getlocal_1",
			"getlocal_2",
			"getlocal_3",
			"setlocal_0",
			"setlocal_1",
			"setlocal_2",
			"setlocal_3",
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
			"debug",
			"debugline",
			"debugfile"
		};
};