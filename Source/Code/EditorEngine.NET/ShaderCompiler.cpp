#include "../Engine/Containers/COMRCPtr.h"
#include "../Engine/Containers/DynamicArray.h"

extern "C" __declspec(dllexport) void CompileShaders(const char* Action)
{
	if (strcmp(Action, "Compile") == 0)
	{
		D3D_SHADER_MACRO ShaderMacro[2] =
		{
			{ "HLSL_SHADER_MODEL", "50" },
			{ NULL, NULL }
		};

		const char16_t* OutputDirSM50 = u"./../../Build/Shaders/ShaderModel50";
		const char16_t* OutputDirSM51 = u"./../../Build/Shaders/ShaderModel51";

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
			COMRCPtr<ID3DBlob> ShaderBlob, ErrorBlob;

			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (wchar_t*)u"%s.hlsl", VertexShader);

			HANDLE InputShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER InputShaderFileSize;
			BOOL Result = GetFileSizeEx(InputShaderFile, &InputShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, InputShaderFileSize.QuadPart);
			Result = ReadFile(InputShaderFile, ShaderData, (DWORD)InputShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(InputShaderFile);

			ShaderMacro[0].Definition = "50";

			HRESULT hr = D3DCompile(ShaderData, InputShaderFileSize.QuadPart, (char*)VertexShader, ShaderMacro, NULL, "VS", "vs_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

			if (FAILED(hr))
			{
				if (ErrorBlob != nullptr)
				{
					cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
				}
			}

			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/%s.dxbc", OutputDirSM50, VertexShader);

			HANDLE OutputShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			LARGE_INTEGER OutputShaderFileSize;
			OutputShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(OutputShaderFile, ShaderBlob->GetBufferPointer(), OutputShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(OutputShaderFile);

			ShaderMacro[0].Definition = "51";

			hr = D3DCompile(ShaderData, InputShaderFileSize.QuadPart, (char*)VertexShader, ShaderMacro, NULL, "VS", "vs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

			if (FAILED(hr))
			{
				if (ErrorBlob != nullptr)
				{
					cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
				}
			}

			wsprintf(OutputFileName, L"%s/%s.dxbc", OutputDirSM51, VertexShader);

			OutputShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			OutputShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(OutputShaderFile, ShaderBlob->GetBufferPointer(), OutputShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(OutputShaderFile);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		}

		for (auto PixelShader : PixelShaders)
		{
			COMRCPtr<ID3DBlob> ShaderBlob, ErrorBlob;

			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (wchar_t*)u"%s.hlsl", PixelShader);

			HANDLE InputShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER InputShaderFileSize;
			BOOL Result = GetFileSizeEx(InputShaderFile, &InputShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, InputShaderFileSize.QuadPart);
			Result = ReadFile(InputShaderFile, ShaderData, (DWORD)InputShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(InputShaderFile);

			ShaderMacro[0].Definition = "50";

			HRESULT hr = D3DCompile(ShaderData, InputShaderFileSize.QuadPart, (char*)PixelShader, ShaderMacro, NULL, "PS", "ps_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

			if (FAILED(hr))
			{
				if (ErrorBlob != nullptr)
				{
					cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
				}
			}

			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/%s.dxbc", OutputDirSM50, PixelShader);

			HANDLE OutputShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			LARGE_INTEGER OutputShaderFileSize;
			OutputShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(OutputShaderFile, ShaderBlob->GetBufferPointer(), OutputShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(OutputShaderFile);

			ShaderMacro[0].Definition = "51";

			hr = D3DCompile(ShaderData, InputShaderFileSize.QuadPart, (char*)PixelShader, ShaderMacro, NULL, "PS", "ps_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

			if (FAILED(hr))
			{
				if (ErrorBlob != nullptr)
				{
					cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
				}
			}

			wsprintf(OutputFileName, L"%s/%s.dxbc", OutputDirSM51, PixelShader);

			OutputShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			OutputShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(OutputShaderFile, ShaderBlob->GetBufferPointer(), OutputShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(OutputShaderFile);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);			
		}

		for (auto ComputeShader : ComputeShaders)
		{
			COMRCPtr<ID3DBlob> ShaderBlob, ErrorBlob;

			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (wchar_t*)u"%s.hlsl", ComputeShader);

			HANDLE InputShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER InputShaderFileSize;
			BOOL Result = GetFileSizeEx(InputShaderFile, &InputShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, InputShaderFileSize.QuadPart);
			Result = ReadFile(InputShaderFile, ShaderData, (DWORD)InputShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(InputShaderFile);

			ShaderMacro[0].Definition = "50";

			HRESULT hr = D3DCompile(ShaderData, InputShaderFileSize.QuadPart, (char*)ComputeShader, ShaderMacro, NULL, "CS", "cs_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

			if (FAILED(hr))
			{
				if (ErrorBlob != nullptr)
				{
					cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
				}
			}

			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/%s.dxbc", OutputDirSM50, ComputeShader);

			HANDLE OutputShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			LARGE_INTEGER OutputShaderFileSize;
			OutputShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(OutputShaderFile, ShaderBlob->GetBufferPointer(), OutputShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(OutputShaderFile);

			ShaderMacro[0].Definition = "51";

			hr = D3DCompile(ShaderData, InputShaderFileSize.QuadPart, (char*)ComputeShader, ShaderMacro, NULL, "CS", "cs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

			if (FAILED(hr))
			{
				if (ErrorBlob != nullptr)
				{
					cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
				}
			}

			wsprintf(OutputFileName, L"%s/%s.dxbc", OutputDirSM51, ComputeShader);

			OutputShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			OutputShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(OutputShaderFile, ShaderBlob->GetBufferPointer(), OutputShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(OutputShaderFile);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);			
		}

		COMRCPtr<ID3DBlob> VertexShader1SM50Blob, VertexShader2SM50Blob, PixelShaderSM50Blob;
		COMRCPtr<ID3DBlob> VertexShader1SM51Blob, VertexShader2SM51Blob, PixelShaderSM51Blob;
		COMRCPtr<ID3DBlob> ErrorBlob;

		HANDLE ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_VertexShader_GBufferOpaquePass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER ShaderFileSize;
		BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		ShaderMacro[0].Definition = "50";
		HRESULT hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_VertexShader_GBufferOpaquePass", ShaderMacro, NULL, "VS", "vs_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &VertexShader1SM50Blob, &ErrorBlob);
		
		if (FAILED(hr))
		{
			if (ErrorBlob != nullptr)
			{
				cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
			}
		}
		
		ShaderMacro[0].Definition = "51";
		hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_VertexShader_GBufferOpaquePass", ShaderMacro, NULL, "VS", "vs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &VertexShader1SM51Blob, &ErrorBlob);

		if (FAILED(hr))
		{
			if (ErrorBlob != nullptr)
			{
				cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
			}
		}

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_VertexShader_ShadowMapPass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		ShaderMacro[0].Definition = "50";
		hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_VertexShader_ShadowMapPass", ShaderMacro, NULL, "VS", "vs_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &VertexShader2SM50Blob, &ErrorBlob);

		if (FAILED(hr))
		{
			if (ErrorBlob != nullptr)
			{
				cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
			}
		}

		ShaderMacro[0].Definition = "51";
		hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_VertexShader_ShadowMapPass", ShaderMacro, NULL, "VS", "vs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &VertexShader2SM51Blob, &ErrorBlob);

		if (FAILED(hr))
		{
			if (ErrorBlob != nullptr)
			{
				cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
			}
		}

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_PixelShader_GBufferOpaquePass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		ShaderMacro[0].Definition = "50";
		hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_PixelShader_GBufferOpaquePass", ShaderMacro, NULL, "PS", "ps_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &PixelShaderSM50Blob, &ErrorBlob);

		if (FAILED(hr))
		{
			if (ErrorBlob != nullptr)
			{
				cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
			}
		}

		ShaderMacro[0].Definition = "51";
		hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_PixelShader_GBufferOpaquePass", ShaderMacro, NULL, "PS", "ps_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &PixelShaderSM51Blob, &ErrorBlob);

		if (FAILED(hr))
		{
			if (ErrorBlob != nullptr)
			{
				cout << (const char*)ErrorBlob->GetBufferPointer() << endl;
			}
		}

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		for (int i = 0; i < 4000; i++)
		{
			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.GBufferOpaquePass_VertexShader.dxbc", OutputDirSM50, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = VertexShader1SM50Blob->GetBufferSize();
			Result = WriteFile(ShaderFile, VertexShader1SM50Blob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);			

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.ShadowMapPass_VertexShader.dxbc", OutputDirSM50, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = VertexShader2SM50Blob->GetBufferSize();
			Result = WriteFile(ShaderFile, VertexShader2SM50Blob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);			

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.GBufferOpaquePass_PixelShader.dxbc", OutputDirSM50, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = PixelShaderSM50Blob->GetBufferSize();
			Result = WriteFile(ShaderFile, PixelShaderSM50Blob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.GBufferOpaquePass_VertexShader.dxbc", OutputDirSM51, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = VertexShader1SM51Blob->GetBufferSize();
			Result = WriteFile(ShaderFile, VertexShader1SM51Blob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.ShadowMapPass_VertexShader.dxbc", OutputDirSM51, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = VertexShader2SM51Blob->GetBufferSize();
			Result = WriteFile(ShaderFile, VertexShader2SM51Blob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.GBufferOpaquePass_PixelShader.dxbc", OutputDirSM51, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = PixelShaderSM51Blob->GetBufferSize();
			Result = WriteFile(ShaderFile, PixelShaderSM51Blob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);

			cout << (i + 1) << "/4000" << endl;
		}
	}
	else if (strcmp(Action, "Clean") == 0)
	{

	}
}