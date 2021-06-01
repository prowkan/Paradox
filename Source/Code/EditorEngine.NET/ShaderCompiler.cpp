#include "../Engine/Containers/COMRCPtr.h"
#include "../Engine/Containers/DynamicArray.h"

extern "C" __declspec(dllexport) void CompileShaders(const char* Action)
{
	if (strcmp(Action, "Compile") == 0)
	{
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

			HANDLE ShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER ShaderFileSize;
			BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
			Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(ShaderFile);

			HRESULT hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, NULL, NULL, NULL, "VS", "vs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);

			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/%s.dxbc", OutputDirSM51, VertexShader);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(ShaderFile, ShaderBlob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);
		}

		for (auto PixelShader : PixelShaders)
		{
			COMRCPtr<ID3DBlob> ShaderBlob, ErrorBlob;

			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (wchar_t*)u"%s.hlsl", PixelShader);

			HANDLE ShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER ShaderFileSize;
			BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
			Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(ShaderFile);

			HRESULT hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, NULL, NULL, NULL, "PS", "ps_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);

			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/%s.dxbc", OutputDirSM51, PixelShader);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(ShaderFile, ShaderBlob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);
		}

		for (auto ComputeShader : ComputeShaders)
		{
			COMRCPtr<ID3DBlob> ShaderBlob, ErrorBlob;

			char16_t InputFileName[8192];

			wsprintf((wchar_t*)InputFileName, (wchar_t*)u"%s.hlsl", ComputeShader);

			HANDLE ShaderFile = CreateFile((const wchar_t*)InputFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER ShaderFileSize;
			BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
			void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
			Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(ShaderFile);

			HRESULT hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, NULL, NULL, NULL, "CS", "cs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &ShaderBlob, &ErrorBlob);

			Result = HeapFree(GetProcessHeap(), 0, ShaderData);

			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/%s.dxbc", OutputDirSM51, ComputeShader);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = ShaderBlob->GetBufferSize();
			Result = WriteFile(ShaderFile, ShaderBlob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);
		}

		/*COMRCPtr<ID3DBlob> VertexShader1Blob, VertexShader2Blob, PixelShaderBlob, ErrorBlob;

		HANDLE ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_VertexShader_GBufferOpaquePass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER ShaderFileSize;
		BOOL Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		void *ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		HRESULT hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_VertexShader_GBufferOpaquePass", NULL, NULL, "VS", "vs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &VertexShader1Blob, &ErrorBlob);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_VertexShader_ShadowMapPass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_VertexShader_ShadowMapPass", NULL, NULL, "VS", "vs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &VertexShader2Blob, &ErrorBlob);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		ShaderFile = CreateFile((const wchar_t*)L"MaterialBase_PixelShader_GBufferOpaquePass.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		Result = GetFileSizeEx(ShaderFile, &ShaderFileSize);
		ShaderData = HeapAlloc(GetProcessHeap(), 0, ShaderFileSize.QuadPart);
		Result = ReadFile(ShaderFile, ShaderData, (DWORD)ShaderFileSize.QuadPart, NULL, NULL);
		Result = CloseHandle(ShaderFile);

		hr = D3DCompile(ShaderData, ShaderFileSize.QuadPart, "MaterialBase_PixelShader_GBufferOpaquePass", NULL, NULL, "PS", "ps_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &PixelShaderBlob, &ErrorBlob);

		Result = HeapFree(GetProcessHeap(), 0, ShaderData);

		for (int i = 0; i < 4000; i++)
		{
			wchar_t OutputFileName[8192];

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.GBufferOpaquePass_VertexShader.dxbc", OutputDirSM51, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = VertexShader1Blob->GetBufferSize();
			Result = WriteFile(ShaderFile, VertexShader1Blob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);			

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.ShadowMapPass_VertexShader.dxbc", OutputDirSM51, i);

			ShaderFile = CreateFile(OutputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ShaderFileSize.QuadPart = VertexShader2Blob->GetBufferSize();
			Result = WriteFile(ShaderFile, VertexShader2Blob->GetBufferPointer(), ShaderFileSize.QuadPart, NULL, NULL);
			CloseHandle(ShaderFile);			

			wsprintf(OutputFileName, L"%s/Test.M_Standart_%d.GBufferOpaquePass_PixelShader.dxbc", OutputDirSM51, i);

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