#include "../Engine/Containers/COMRCPtr.h"
#include "../Engine/Containers/DynamicArray.h"

extern "C" __declspec(dllexport) void CompileShaders(const char* Action)
{
	if (strcmp(Action, "Compile") == 0)
	{
		const char16_t* FXCompiler = u"C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/fxc.exe";
		const char16_t* DXCompiler = u"C:/VulkanSDK/1.2.170.0/Bin/dxc.exe";

		const char16_t* OutputDirSM1 = u"./../../Build/Shaders/ShaderModel51/";
		const char16_t* OutputDirSPV = u"./../../Build/Shaders/SPIRV/";

		const char16_t* VertexShaders[] =
		{
			u"SkyVertexShader",
			u"SunVertexShader",
			u"FullScreenQuad"
		};

		const char16_t* PixelShaders[] =
		{
			u"SkyPixelShader",
			u"SunPixelShader",
			u"OcclusionBuffer",
			u"MSAADepthResolve",
			u"ShadowResolve",
			u"DeferredLighting",
			u"Fog",
			u"BrightPass",
			u"ImageResample",
			u"HorizontalBlur",
			u"VerticalBlur",
			u"HDRToneMapping"
		};

		const char16_t* ComputeShaders[] =
		{
			u"LuminanceCalc",
			u"LuminanceSum",
			u"LuminanceAvg"
		};

		for (auto VertexShader : VertexShaders)
		{
			char16_t FXCompilerArgs[8192];

			wsprintf((wchar_t*)FXCompilerArgs, (const wchar_t*)u"%s -T vs_5_1 -E VS -Zpr -Fo %s%s.dxbc %s.hlsl", FXCompiler, OutputDirSM1, VertexShader, VertexShader);
			
			STARTUPINFO StartupInfo;
			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
			StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
			StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
			StartupInfo.dwFlags = STARTF_USESTDHANDLES;

			PROCESS_INFORMATION ProcessInformation;
			
			BOOL Result = CreateProcess(NULL, (wchar_t*)FXCompilerArgs, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &StartupInfo, &ProcessInformation);
			
			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

			char16_t DXCompilerArgs[8192];

			wsprintf((wchar_t*)DXCompilerArgs, (const wchar_t*)u"%s -D SPIRV -spirv -fvk-invert-y -T vs_5_1 -E VS -Zpr -Fo %s%s.spv %s.hlsl", DXCompiler, OutputDirSPV, VertexShader, VertexShader);

			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
			StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
			StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
			StartupInfo.dwFlags = STARTF_USESTDHANDLES;

			Result = CreateProcess(NULL, (wchar_t*)DXCompilerArgs, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &StartupInfo, &ProcessInformation);
			
			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
		}

		for (auto PixelShader : PixelShaders)
		{
			char16_t FXCompilerArgs[8192];

			wsprintf((wchar_t*)FXCompilerArgs, (const wchar_t*)u"%s -T ps_5_1 -E PS -Zpr -Fo %s%s.dxbc %s.hlsl", FXCompiler, OutputDirSM1, PixelShader, PixelShader);

			STARTUPINFO StartupInfo;
			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
			StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
			StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
			StartupInfo.dwFlags = STARTF_USESTDHANDLES;

			PROCESS_INFORMATION ProcessInformation;

			BOOL Result = CreateProcess(NULL, (wchar_t*)FXCompilerArgs, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &StartupInfo, &ProcessInformation);

			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

			char16_t DXCompilerArgs[8192];

			wsprintf((wchar_t*)DXCompilerArgs, (const wchar_t*)u"%s -D SPIRV -spirv -fvk-use-dx-position-w -T ps_5_1 -E PS -Zpr -Fo %s%s.spv %s.hlsl", DXCompiler, OutputDirSPV, PixelShader, PixelShader);

			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
			StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
			StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
			StartupInfo.dwFlags = STARTF_USESTDHANDLES;

			Result = CreateProcess(NULL, (wchar_t*)DXCompilerArgs, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &StartupInfo, &ProcessInformation);

			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
		}

		for (auto ComputeShader : ComputeShaders)
		{
			char16_t FXCompilerArgs[8192];

			wsprintf((wchar_t*)FXCompilerArgs, (const wchar_t*)u"%s -T cs_5_1 -E CS -Zpr -Fo %s%s.dxbc %s.hlsl", FXCompiler, OutputDirSM1, ComputeShader, ComputeShader);

			STARTUPINFO StartupInfo;
			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
			StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
			StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
			StartupInfo.dwFlags = STARTF_USESTDHANDLES;
			
			PROCESS_INFORMATION ProcessInformation;

			BOOL Result = CreateProcess(NULL, (wchar_t*)FXCompilerArgs, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &StartupInfo, &ProcessInformation);

			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

			char16_t DXCompilerArgs[8192];

			wsprintf((wchar_t*)DXCompilerArgs, (const wchar_t*)u"%s -D SPIRV -spirv -T cs_5_1 -E CS -Zpr -Fo %s%s.spv %s.hlsl", DXCompiler, OutputDirSPV, ComputeShader, ComputeShader);

			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
			StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
			StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
			StartupInfo.dwFlags = STARTF_USESTDHANDLES;

			Result = CreateProcess(NULL, (wchar_t*)DXCompilerArgs, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &StartupInfo, &ProcessInformation);

			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
		}

		/*STARTUPINFO StartupInfo;
		ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
		StartupInfo.cb = sizeof(STARTUPINFO);
		StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		StartupInfo.dwFlags = STARTF_USESTDHANDLES;

		PROCESS_INFORMATION ProcessInformation;

		char16_t ScriptCommandLine[] = u"C:/Windows/py.exe \"F:/Paradox/BundleContent.py\" \"F:/Paradox/Build/GameContent/Shaders\" \"Shaders\"";

		BOOL Result = CreateProcess(NULL, (wchar_t*)ScriptCommandLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &StartupInfo, &ProcessInformation);

		Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);*/

		COMRCPtr<IDxcCompiler3> Compiler;

		HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void**)&Compiler);
		
		HANDLE ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_VertexShader_GBufferOpaquePass.hlsl", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER ShaderFileSize;
		BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		COMRCPtr<ID3DBlob> ShaderBlob, ErrorBlob;
		COMRCPtr<IDxcOperationResult> OperationResult;
		COMRCPtr<IDxcBlob> ShaderBlobDXC;

		hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_VertexShader_GBufferOpaquePass", NULL, NULL, "VS", "vs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

		DxcBuffer dxcBuffer;
		dxcBuffer.Encoding = 0;
		dxcBuffer.Ptr = ShaderData;
		dxcBuffer.Size = ShaderFileSize.QuadPart;

		DynamicArray<const wchar_t*> DXCompilerArgs;
		DXCompilerArgs.Clear();
		DXCompilerArgs.Add(L"-E");
		DXCompilerArgs.Add(L"VS");
		DXCompilerArgs.Add(L"-T");
		DXCompilerArgs.Add(L"vs_5_1");
		DXCompilerArgs.Add(L"-Zpr");
		DXCompilerArgs.Add(L"-D");
		DXCompilerArgs.Add(L"SPIRV");
		DXCompilerArgs.Add(L"-spirv");
		DXCompilerArgs.Add(L"-fvk-invert-y");

		Compiler->Compile(&dxcBuffer, (LPCWSTR*)DXCompilerArgs.GetData(), DXCompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&OperationResult);

		hr = OperationResult->GetResult(&ShaderBlobDXC);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)L"./../../Build/Shaders/ShaderModel51/MaterialBase_VertexShader_GBufferOpaquePass.dxbc", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		ShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
		Result = WriteFile(ShaderFile, ShaderBlob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
		CloseHandle(ShaderFile);

		ShaderFile = CreateFile((const wchar_t*)L"./../../Build/Shaders/SPIRV/MaterialBase_VertexShader_GBufferOpaquePass.spv", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		ShaderFileSize.QuadPart = ShaderBlobDXC->GetBufferSize();
		Result = WriteFile(ShaderFile, ShaderBlobDXC->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
		CloseHandle(ShaderFile);

		ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_VertexShader_ShadowMapPass.hlsl", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_VertexShader_ShadowMapPass", NULL, NULL, "VS", "vs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

		dxcBuffer.Encoding = 0;
		dxcBuffer.Ptr = ShaderData;
		dxcBuffer.Size = ShaderFileSize.QuadPart;

		DXCompilerArgs.Clear();
		DXCompilerArgs.Add(L"-E");
		DXCompilerArgs.Add(L"VS");
		DXCompilerArgs.Add(L"-T");
		DXCompilerArgs.Add(L"vs_5_1");
		DXCompilerArgs.Add(L"-Zpr");
		DXCompilerArgs.Add(L"-D");
		DXCompilerArgs.Add(L"SPIRV");
		DXCompilerArgs.Add(L"-spirv");
		DXCompilerArgs.Add(L"-fvk-invert-y");

		Compiler->Compile(&dxcBuffer, (LPCWSTR*)DXCompilerArgs.GetData(), DXCompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&OperationResult);

		hr = OperationResult->GetResult(&ShaderBlobDXC);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)L"./../../Build/Shaders/ShaderModel51/MaterialBase_VertexShader_ShadowMapPass.dxbc", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		ShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
		Result = WriteFile(ShaderFile, ShaderBlob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
		CloseHandle(ShaderFile);

		ShaderFile = CreateFile((const wchar_t*)L"./../../Build/Shaders/SPIRV/MaterialBase_VertexShader_ShadowMapPass.spv", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		ShaderFileSize.QuadPart = ShaderBlobDXC->GetBufferSize();
		Result = WriteFile(ShaderFile, ShaderBlobDXC->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
		CloseHandle(ShaderFile);

		ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_PixelShader_GBufferOpaquePass.hlsl", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_PixelShader_GBufferOpaquePass", NULL, NULL, "PS", "ps_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

		dxcBuffer.Encoding = 0;
		dxcBuffer.Ptr = ShaderData;
		dxcBuffer.Size = ShaderFileSize.QuadPart;

		DXCompilerArgs.Clear();
		DXCompilerArgs.Add(L"-E");
		DXCompilerArgs.Add(L"PS");
		DXCompilerArgs.Add(L"-T");
		DXCompilerArgs.Add(L"ps_5_1");
		DXCompilerArgs.Add(L"-Zpr");
		DXCompilerArgs.Add(L"-D");
		DXCompilerArgs.Add(L"SPIRV");
		DXCompilerArgs.Add(L"-spirv");
		DXCompilerArgs.Add(L"-fvk-use-dx-position-w");

		Compiler->Compile(&dxcBuffer, (LPCWSTR*)DXCompilerArgs.GetData(), DXCompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&OperationResult);

		hr = OperationResult->GetResult(&ShaderBlobDXC);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)L"./../../Build/Shaders/ShaderModel51/MaterialBase_PixelShader_GBufferOpaquePass.dxbc", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		ShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
		Result = WriteFile(ShaderFile, ShaderBlob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
		CloseHandle(ShaderFile);

		ShaderFile = CreateFile((const wchar_t*)L"./../../Build/Shaders/SPIRV/MaterialBase_PixelShader_GBufferOpaquePass.spv", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		ShaderFileSize.QuadPart = ShaderBlobDXC->GetBufferSize();
		Result = WriteFile(ShaderFile, ShaderBlobDXC->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
		CloseHandle(ShaderFile);
	}
	else if (strcmp(Action, "Clean") == 0)
	{

	}
}