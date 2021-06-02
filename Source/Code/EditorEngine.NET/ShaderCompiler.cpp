#include "../Engine/Containers/COMRCPtr.h"
#include "../Engine/Containers/DynamicArray.h"

extern "C" __declspec(dllexport) void CompileShaders(const char* Action)
{
	if (strcmp(Action, "Compile") == 0)
	{
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

		COMRCPtr<IDxcOperationResult> OperationResult;
		COMRCPtr<IDxcBlob> ShaderBlobDXC;

		HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void**)&Compiler);

		for (auto VertexShader : VertexShaders)
		{
			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (const wchar_t*)u"%s.hlsl", VertexShader);

			HANDLE ShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER ShaderFileSize;
			BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
			Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(ShaderFile);

			DxcBuffer dxcBuffer;
			dxcBuffer.Encoding = 0;
			dxcBuffer.Ptr = ShaderData;
			dxcBuffer.Size = ShaderFileSize.QuadPart;

			DynamicArray<const wchar_t*> DXCompilerArgs;
			DXCompilerArgs.Clear();
			DXCompilerArgs.Add((const wchar_t*)u"-E");
			DXCompilerArgs.Add((const wchar_t*)u"VS");
			DXCompilerArgs.Add((const wchar_t*)u"-T");
			DXCompilerArgs.Add((const wchar_t*)u"vs_6_6");
			DXCompilerArgs.Add((const wchar_t*)u"-Zpr");

			Compiler->Compile(&dxcBuffer, (LPCWSTR*)DXCompilerArgs.GetData(), DXCompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&OperationResult);

			HRESULT CompilationResult;

			hr = OperationResult->GetStatus(&CompilationResult);

			if (SUCCEEDED(hr) && FAILED(CompilationResult))
			{
				COMRCPtr<IDxcBlobEncoding> ErrorBlob;

				hr = OperationResult->GetErrorBuffer(&ErrorBlob);

				if (ErrorBlob) cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
			}

			hr = OperationResult->GetResult(&ShaderBlobDXC);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);

			char16_t OutputFileName[8192];

			wsprintf((wchar_t*)OutputFileName, (const wchar_t*)u"./../../Build/Shaders/ShaderModel66/%s.dxil", VertexShader);

			ShaderFile = CreateFile((wchar_t*)OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = ShaderBlobDXC->GetBufferSize();
			Result = WriteFile(ShaderFile, ShaderBlobDXC->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);
		}

		for (auto PixelShader : PixelShaders)
		{
			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (const wchar_t*)u"%s.hlsl", PixelShader);

			HANDLE ShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER ShaderFileSize;
			BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
			Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(ShaderFile);

			DxcBuffer dxcBuffer;
			dxcBuffer.Encoding = 0;
			dxcBuffer.Ptr = ShaderData;
			dxcBuffer.Size = ShaderFileSize.QuadPart;

			DynamicArray<const wchar_t*> DXCompilerArgs;
			DXCompilerArgs.Clear();
			DXCompilerArgs.Add((const wchar_t*)u"-E");
			DXCompilerArgs.Add((const wchar_t*)u"PS");
			DXCompilerArgs.Add((const wchar_t*)u"-T");
			DXCompilerArgs.Add((const wchar_t*)u"ps_6_6");
			DXCompilerArgs.Add((const wchar_t*)u"-Zpr");

			Compiler->Compile(&dxcBuffer, (LPCWSTR*)DXCompilerArgs.GetData(), DXCompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&OperationResult);

			HRESULT CompilationResult;

			hr = OperationResult->GetStatus(&CompilationResult);

			if (SUCCEEDED(hr) && FAILED(CompilationResult))
			{
				COMRCPtr<IDxcBlobEncoding> ErrorBlob;

				hr = OperationResult->GetErrorBuffer(&ErrorBlob);

				if (ErrorBlob) cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
			}

			hr = OperationResult->GetResult(&ShaderBlobDXC);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);

			char16_t OutputFileName[8192];

			wsprintf((wchar_t*)OutputFileName, (const wchar_t*)u"./../../Build/Shaders/ShaderModel66/%s.dxil", PixelShader);

			ShaderFile = CreateFile((wchar_t*)OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = ShaderBlobDXC->GetBufferSize();
			Result = WriteFile(ShaderFile, ShaderBlobDXC->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);
		}

		for (auto ComputeShader : ComputeShaders)
		{
			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (const wchar_t*)u"%s.hlsl", ComputeShader);

			HANDLE ShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER ShaderFileSize;
			BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
			Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(ShaderFile);

			DxcBuffer dxcBuffer;
			dxcBuffer.Encoding = 0;
			dxcBuffer.Ptr = ShaderData;
			dxcBuffer.Size = ShaderFileSize.QuadPart;

			DynamicArray<const wchar_t*> DXCompilerArgs;
			DXCompilerArgs.Clear();
			DXCompilerArgs.Add((const wchar_t*)u"-E");
			DXCompilerArgs.Add((const wchar_t*)u"CS");
			DXCompilerArgs.Add((const wchar_t*)u"-T");
			DXCompilerArgs.Add((const wchar_t*)u"cs_6_6");
			DXCompilerArgs.Add((const wchar_t*)u"-Zpr");

			Compiler->Compile(&dxcBuffer, (LPCWSTR*)DXCompilerArgs.GetData(), DXCompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&OperationResult);

			HRESULT CompilationResult;

			hr = OperationResult->GetStatus(&CompilationResult);

			if (SUCCEEDED(hr) && FAILED(CompilationResult))
			{
				COMRCPtr<IDxcBlobEncoding> ErrorBlob;

				hr = OperationResult->GetErrorBuffer(&ErrorBlob);

				if (ErrorBlob) cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
			}

			hr = OperationResult->GetResult(&ShaderBlobDXC);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);

			char16_t OutputFileName[8192];

			wsprintf((wchar_t*)OutputFileName, (const wchar_t*)u"./../../Build/Shaders/ShaderModel66/%s.dxil", ComputeShader);

			ShaderFile = CreateFile((wchar_t*)OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = ShaderBlobDXC->GetBufferSize();
			Result = WriteFile(ShaderFile, ShaderBlobDXC->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);
		}

		HANDLE ShaderFile = CreateFile((const wchar_t*)u"MaterialBase_VertexShader_GBufferOpaquePass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

		if (ShaderFile == INVALID_HANDLE_VALUE)
		{
			DWORD Error = GetLastError();
			cout << "System error: " << Error << endl;
		}

		LARGE_INTEGER ShaderFileSize;
		BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		DxcBuffer dxcBuffer;
		dxcBuffer.Encoding = 0;
		dxcBuffer.Ptr = ShaderData;
		dxcBuffer.Size = ShaderFileSize.QuadPart;

		DynamicArray<const wchar_t*> DXCompilerArgs;
		DXCompilerArgs.Clear();
		DXCompilerArgs.Add((const wchar_t*)u"-E");
		DXCompilerArgs.Add((const wchar_t*)u"VS");
		DXCompilerArgs.Add((const wchar_t*)u"-T");
		DXCompilerArgs.Add((const wchar_t*)u"vs_6_6");
		DXCompilerArgs.Add((const wchar_t*)u"-Zpr");

		hr = Compiler->Compile(&dxcBuffer, (LPCWSTR*)DXCompilerArgs.GetData(), DXCompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&OperationResult);

		HRESULT CompilationResult;

		hr = OperationResult->GetStatus(&CompilationResult);

		if (SUCCEEDED(hr) && FAILED(CompilationResult))
		{
			COMRCPtr<IDxcBlobEncoding> ErrorBlob;

			hr = OperationResult->GetErrorBuffer(&ErrorBlob);

			if (ErrorBlob) cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
		}

		COMRCPtr<IDxcBlob> VertexShader1BlobDXC;

		hr = OperationResult->GetResult(&VertexShader1BlobDXC);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)u"MaterialBase_VertexShader_ShadowMapPass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

		if (ShaderFile == INVALID_HANDLE_VALUE)
		{
			DWORD Error = GetLastError();
			cout << "System error: " << Error << endl;
		}

		Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		dxcBuffer.Encoding = 0;
		dxcBuffer.Ptr = ShaderData;
		dxcBuffer.Size = ShaderFileSize.QuadPart;

		DXCompilerArgs.Clear();
		DXCompilerArgs.Add((const wchar_t*)u"-E");
		DXCompilerArgs.Add((const wchar_t*)u"VS");
		DXCompilerArgs.Add((const wchar_t*)u"-T");
		DXCompilerArgs.Add((const wchar_t*)u"vs_6_6");
		DXCompilerArgs.Add((const wchar_t*)u"-Zpr");

		hr = Compiler->Compile(&dxcBuffer, (LPCWSTR*)DXCompilerArgs.GetData(), DXCompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&OperationResult);

		hr = OperationResult->GetStatus(&CompilationResult);

		if (SUCCEEDED(hr) && FAILED(CompilationResult))
		{
			COMRCPtr<IDxcBlobEncoding> ErrorBlob;

			hr = OperationResult->GetErrorBuffer(&ErrorBlob);

			if (ErrorBlob) cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
		}

		COMRCPtr<IDxcBlob> VertexShader2BlobDXC;

		hr = OperationResult->GetResult(&VertexShader2BlobDXC);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)u"MaterialBase_PixelShader_GBufferOpaquePass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

		if (ShaderFile == INVALID_HANDLE_VALUE)
		{
			DWORD Error = GetLastError();
			cout << "System error: " << Error << endl;
		}

		Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		dxcBuffer.Encoding = 0;
		dxcBuffer.Ptr = ShaderData;
		dxcBuffer.Size = ShaderFileSize.QuadPart;

		DXCompilerArgs.Clear();
		DXCompilerArgs.Add((const wchar_t*)u"-E");
		DXCompilerArgs.Add((const wchar_t*)u"PS");
		DXCompilerArgs.Add((const wchar_t*)u"-T");
		DXCompilerArgs.Add((const wchar_t*)u"ps_6_6");
		DXCompilerArgs.Add((const wchar_t*)u"-Zpr");

		hr = Compiler->Compile(&dxcBuffer, (LPCWSTR*)DXCompilerArgs.GetData(), DXCompilerArgs.GetLength(), NULL, __uuidof(IDxcOperationResult), (void**)&OperationResult);

		hr = OperationResult->GetStatus(&CompilationResult);

		if (SUCCEEDED(hr) && FAILED(CompilationResult))
		{
			COMRCPtr<IDxcBlobEncoding> ErrorBlob;

			hr = OperationResult->GetErrorBuffer(&ErrorBlob);

			if (ErrorBlob) cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
		}

		COMRCPtr<IDxcBlob> PixelShaderBlobDXC;

		hr = OperationResult->GetResult(&PixelShaderBlobDXC);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		for (int i = 0; i < 4000; i++)
		{
			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, (const wchar_t*)u"./../../Build/Shaders/ShaderModel66/Test.M_Standart_%d.GBufferOpaquePass_VertexShader.dxil", i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = VertexShader1BlobDXC->GetBufferSize();
			Result = WriteFile(ShaderFile, VertexShader1BlobDXC->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);			

			wsprintf(OutputFileName, (const wchar_t*)u"./../../Build/Shaders/ShaderModel66/Test.M_Standart_%d.ShadowMapPass_VertexShader.dxil", i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = VertexShader2BlobDXC->GetBufferSize();
			Result = WriteFile(ShaderFile, VertexShader2BlobDXC->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);

			wsprintf(OutputFileName, (const wchar_t*)u"./../../Build/Shaders/ShaderModel66/Test.M_Standart_%d.GBufferOpaquePass_PixelShader.dxil", i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = PixelShaderBlobDXC->GetBufferSize();
			Result = WriteFile(ShaderFile, PixelShaderBlobDXC->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);

			cout << (i + 1) << "/4000" << endl;
		}
	}
	else if (strcmp(Action, "Clean") == 0)
	{

	}
}