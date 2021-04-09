extern "C" __declspec(dllexport) void CompileShaders(const char* Action)
{
	if (strcmp(Action, "Compile") == 0)
	{
		const char16_t* FXCompiler = u"C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/fxc.exe";
		const char16_t* DXCompiler = u"C:/VulkanSDK/1.2.170.0/Bin/dxc.exe";

		const char16_t* OutputDirSM1 = u"./../../Build/GameContent/Shaders/ShaderModel51/";
		const char16_t* OutputDirSPV = u"./../../Build/GameContent/Shaders/SPIRV/";

		const char16_t* VertexShaders[] =
		{
			u"MaterialBase_VertexShader_GBufferOpaquePass",
			u"MaterialBase_VertexShader_ShadowMapPass",
			u"SkyVertexShader",
			u"SunVertexShader",
			u"FullScreenQuad"
		};

		const char16_t* PixelShaders[] =
		{
			u"MaterialBase_PixelShader_GBufferOpaquePass",
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

		STARTUPINFO StartupInfo;
		ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
		StartupInfo.cb = sizeof(STARTUPINFO);
		StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		StartupInfo.dwFlags = STARTF_USESTDHANDLES;

		PROCESS_INFORMATION ProcessInformation;

		char16_t ScriptCommandLine[] = u"C:/Windows/py.exe \"F:/Paradox/BundleContent.py\" \"F:/Paradox/Build/GameContent/Shaders\" \"Shaders\"";

		BOOL Result = CreateProcess(NULL, (wchar_t*)ScriptCommandLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &StartupInfo, &ProcessInformation);

		Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
	}
	else if (strcmp(Action, "Clean") == 0)
	{

	}
}