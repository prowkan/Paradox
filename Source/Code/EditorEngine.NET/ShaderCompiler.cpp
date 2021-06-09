#include "../Engine/Containers/COMRCPtr.h"
#include "../Engine/Containers/DynamicArray.h"

extern "C" __declspec(dllexport) void CompileShaders(const char* Action)
{
	if (strcmp(Action, "Compile") == 0)
	{
		const char16_t* OutputDirSM60 = u"./../../Build/Shaders/ShaderModel60";

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

		COMRCPtr<IDxcCompiler3> Compiler;

		HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void**)&Compiler);

		for (auto VertexShader : VertexShaders)
		{
			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (wchar_t*)u"%s.hlsl", VertexShader);

			HANDLE ShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER ShaderFileSize;
			BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
			Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(ShaderFile);

			DxcBuffer ShaderSource;
			ShaderSource.Encoding = 0;
			ShaderSource.Ptr = ShaderData;
			ShaderSource.Size = ShaderFileSize.QuadPart;

			DynamicArray<const char16_t*> CompilerArgs;
			CompilerArgs.Add(u"-E VS");
			CompilerArgs.Add(u"-T vs_6_0");
			CompilerArgs.Add(u"-Zpr");

			COMRCPtr<IDxcOperationResult> CompilationResult;

			HRESULT hr = Compiler->Compile(&ShaderSource, (const wchar_t**)CompilerArgs.GetData(), CompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&CompilationResult);

			HRESULT CompilationStatus;

			hr = CompilationResult->GetStatus(&CompilationStatus);

			if (FAILED(CompilationStatus))
			{
				COMRCPtr<IDxcBlobEncoding> ErrorBuffer;
				hr = CompilationResult->GetErrorBuffer(&ErrorBuffer);
				if (ErrorBuffer.Pointer)
				{
					cout << (const char*)ErrorBuffer->GetBufferPointer() << endl;
				}
				ExitProcess(0);
			}

			COMRCPtr<IDxcBlob> ShaderBlob;

			hr = CompilationResult->GetResult(&ShaderBlob);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);

			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/%s.dxil", OutputDirSM60, VertexShader);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(ShaderFile, ShaderBlob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);
		}

		for (auto PixelShader : PixelShaders)
		{
			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (wchar_t*)u"%s.hlsl", PixelShader);

			HANDLE ShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER ShaderFileSize;
			BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
			Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(ShaderFile);

			DxcBuffer ShaderSource;
			ShaderSource.Encoding = 0;
			ShaderSource.Ptr = ShaderData;
			ShaderSource.Size = ShaderFileSize.QuadPart;

			DynamicArray<const char16_t*> CompilerArgs;
			CompilerArgs.Add(u"-E PS");
			CompilerArgs.Add(u"-T ps_6_0");
			CompilerArgs.Add(u"-Zpr");

			COMRCPtr<IDxcOperationResult> CompilationResult;

			HRESULT hr = Compiler->Compile(&ShaderSource, (const wchar_t**)CompilerArgs.GetData(), CompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&CompilationResult);

			HRESULT CompilationStatus;

			hr = CompilationResult->GetStatus(&CompilationStatus);

			if (FAILED(CompilationStatus))
			{
				COMRCPtr<IDxcBlobEncoding> ErrorBuffer;
				hr = CompilationResult->GetErrorBuffer(&ErrorBuffer);
				if (ErrorBuffer.Pointer)
				{
					cout << (const char*)ErrorBuffer->GetBufferPointer() << endl;
				}
				ExitProcess(0);
			}

			COMRCPtr<IDxcBlob> ShaderBlob;

			hr = CompilationResult->GetResult(&ShaderBlob);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);

			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/%s.dxil", OutputDirSM60, PixelShader);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(ShaderFile, ShaderBlob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);
		}

		for (auto ComputeShader : ComputeShaders)
		{
			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (wchar_t*)u"%s.hlsl", ComputeShader);

			HANDLE ShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER ShaderFileSize;
			BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
			Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(ShaderFile);

			DxcBuffer ShaderSource;
			ShaderSource.Encoding = 0;
			ShaderSource.Ptr = ShaderData;
			ShaderSource.Size = ShaderFileSize.QuadPart;

			DynamicArray<const char16_t*> CompilerArgs;
			CompilerArgs.Add(u"-E CS");
			CompilerArgs.Add(u"-T cs_6_0");
			CompilerArgs.Add(u"-Zpr");

			COMRCPtr<IDxcOperationResult> CompilationResult;

			HRESULT hr = Compiler->Compile(&ShaderSource, (const wchar_t**)CompilerArgs.GetData(), CompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&CompilationResult);

			HRESULT CompilationStatus;

			hr = CompilationResult->GetStatus(&CompilationStatus);

			if (FAILED(CompilationStatus))
			{
				COMRCPtr<IDxcBlobEncoding> ErrorBuffer;
				hr = CompilationResult->GetErrorBuffer(&ErrorBuffer);
				if (ErrorBuffer.Pointer)
				{
					cout << (const char*)ErrorBuffer->GetBufferPointer() << endl;
				}
				ExitProcess(0);
			}

			COMRCPtr<IDxcBlob> ShaderBlob;

			hr = CompilationResult->GetResult(&ShaderBlob);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);

			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/%s.dxil", OutputDirSM60, ComputeShader);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(ShaderFile, ShaderBlob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);
		}

		/*HANDLE ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_VertexShader_GBufferOpaquePass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER ShaderFileSize;
		BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		DxcBuffer ShaderSource;
		ShaderSource.Encoding = 0;
		ShaderSource.Ptr = ShaderData;
		ShaderSource.Size = ShaderFileSize.QuadPart;

		DynamicArray<const char16_t*> CompilerArgs;
		CompilerArgs.Add(u"-E VS");
		CompilerArgs.Add(u"-T vs_6_0");
		CompilerArgs.Add(u"-Zpr");

		COMRCPtr<IDxcOperationResult> CompilationResult;

		hr = Compiler->Compile(&ShaderSource, (const wchar_t**)CompilerArgs.GetData(), CompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&CompilationResult);

		HRESULT CompilationStatus;

		hr = CompilationResult->GetStatus(&CompilationStatus);

		if (FAILED(CompilationStatus))
		{
			COMRCPtr<IDxcBlobEncoding> ErrorBuffer;
			hr = CompilationResult->GetErrorBuffer(&ErrorBuffer);
			if (ErrorBuffer.Pointer)
			{
				cout << (const char*)ErrorBuffer->GetBufferPointer() << endl;
			}
			ExitProcess(0);
		}

		COMRCPtr<IDxcBlob> VertexShader1Blob;

		hr = CompilationResult->GetResult(&VertexShader1Blob);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_VertexShader_ShadowMapPass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		ShaderSource.Encoding = 0;
		ShaderSource.Ptr = ShaderData;
		ShaderSource.Size = ShaderFileSize.QuadPart;

		CompilerArgs.Clear();
		CompilerArgs.Add(u"-E VS");
		CompilerArgs.Add(u"-T vs_6_0");
		CompilerArgs.Add(u"-Zpr");

		hr = Compiler->Compile(&ShaderSource, (const wchar_t**)CompilerArgs.GetData(), CompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&CompilationResult);

		hr = CompilationResult->GetStatus(&CompilationStatus);

		if (FAILED(CompilationStatus))
		{
			COMRCPtr<IDxcBlobEncoding> ErrorBuffer;
			hr = CompilationResult->GetErrorBuffer(&ErrorBuffer);
			if (ErrorBuffer.Pointer)
			{
				cout << (const char*)ErrorBuffer->GetBufferPointer() << endl;
			}
			ExitProcess(0);
		}

		COMRCPtr<IDxcBlob> VertexShader2Blob;

		hr = CompilationResult->GetResult(&VertexShader2Blob);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_PixelShader_GBufferOpaquePass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		ShaderSource.Encoding = 0;
		ShaderSource.Ptr = ShaderData;
		ShaderSource.Size = ShaderFileSize.QuadPart;

		CompilerArgs.Clear();
		CompilerArgs.Add(u"-E PS");
		CompilerArgs.Add(u"-T ps_6_0");
		CompilerArgs.Add(u"-Zpr");

		hr = Compiler->Compile(&ShaderSource, (const wchar_t**)CompilerArgs.GetData(), CompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&CompilationResult);

		hr = CompilationResult->GetStatus(&CompilationStatus);

		if (FAILED(CompilationStatus))
		{
			COMRCPtr<IDxcBlobEncoding> ErrorBuffer;
			hr = CompilationResult->GetErrorBuffer(&ErrorBuffer);
			if (ErrorBuffer.Pointer)
			{
				cout << (const char*)ErrorBuffer->GetBufferPointer() << endl;
			}
			ExitProcess(0);
		}

		COMRCPtr<IDxcBlob> PixelShaderBlob;

		hr = CompilationResult->GetResult(&PixelShaderBlob);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		for (int i = 0; i < 4000; i++)
		{
			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.GBufferOpaquePass_VertexShader.dxil", OutputDirSM60, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = VertexShader1Blob->GetBufferSize();
			Result = WriteFile(ShaderFile, VertexShader1Blob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);			

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.ShadowMapPass_VertexShader.dxil", OutputDirSM60, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = VertexShader2Blob->GetBufferSize();
			Result = WriteFile(ShaderFile, VertexShader2Blob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);			

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.GBufferOpaquePass_PixelShader.dxil", OutputDirSM60, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = PixelShaderBlob->GetBufferSize();
			Result = WriteFile(ShaderFile, PixelShaderBlob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);

			cout << (i + 1) << "/4000" << endl;
		}*/
	}
	else if (strcmp(Action, "Clean") == 0)
	{

	}
}