extern "C" __declspec(dllexport) void CALLBACK CompileShaders(HWND hWnd, HINSTANCE hInst, LPSTR lpszCmdLine, int nCmdShow)
{
	BOOL Result;

	Result = AllocConsole();
	Result = SetConsoleTitle((wchar_t*)u"Shader Compiling...");
	freopen("CONOUT$", "w", stdout);

	if (strcmp(lpszCmdLine, "Compile") == 0)
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

			PROCESS_INFORMATION ProcessInformation;
			
			BOOL Result = CreateProcess(NULL, (wchar_t*)FXCompilerArgs, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInformation);
			
			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

			char16_t DXCompilerArgs[8192];

			wsprintf((wchar_t*)DXCompilerArgs, (const wchar_t*)u"%s -D SPIRV -spirv -fvk-invert-y -T vs_5_1 -E VS -Zpr -Fo %s%s.spv %s.hlsl", DXCompiler, OutputDirSPV, VertexShader, VertexShader);

			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);

			Result = CreateProcess(NULL, (wchar_t*)DXCompilerArgs, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInformation);
			
			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
		}

		for (auto PixelShader : PixelShaders)
		{
			char16_t FXCompilerArgs[8192];

			wsprintf((wchar_t*)FXCompilerArgs, (const wchar_t*)u"%s -T ps_5_1 -E PS -Zpr -Fo %s%s.dxbc %s.hlsl", FXCompiler, OutputDirSM1, PixelShader, PixelShader);

			STARTUPINFO StartupInfo;
			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);

			PROCESS_INFORMATION ProcessInformation;

			BOOL Result = CreateProcess(NULL, (wchar_t*)FXCompilerArgs, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInformation);

			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

			char16_t DXCompilerArgs[8192];

			wsprintf((wchar_t*)DXCompilerArgs, (const wchar_t*)u"%s -D SPIRV -spirv -fvk-use-dx-position-w -T ps_5_1 -E PS -Zpr -Fo %s%s.spv %s.hlsl", DXCompiler, OutputDirSPV, PixelShader, PixelShader);

			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);

			Result = CreateProcess(NULL, (wchar_t*)DXCompilerArgs, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInformation);

			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
		}

		for (auto ComputeShader : ComputeShaders)
		{
			char16_t FXCompilerArgs[8192];

			wsprintf((wchar_t*)FXCompilerArgs, (const wchar_t*)u"%s -T cs_5_1 -E CS -Zpr -Fo %s%s.dxbc %s.hlsl", FXCompiler, OutputDirSM1, ComputeShader, ComputeShader);

			STARTUPINFO StartupInfo;
			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);

			PROCESS_INFORMATION ProcessInformation;

			BOOL Result = CreateProcess(NULL, (wchar_t*)FXCompilerArgs, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInformation);

			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

			char16_t DXCompilerArgs[8192];

			wsprintf((wchar_t*)DXCompilerArgs, (const wchar_t*)u"%s -D SPIRV -spirv -T cs_5_1 -E CS -Zpr -Fo %s%s.spv %s.hlsl", DXCompiler, OutputDirSPV, ComputeShader, ComputeShader);

			ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);

			Result = CreateProcess(NULL, (wchar_t*)DXCompilerArgs, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInformation);

			Result = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
		}

		HINSTANCE SEResult = ShellExecute(NULL, (const wchar_t*)u"open", (const wchar_t*)u"F:/Paradox/BundleContent.py", (const wchar_t*)u"F:/Paradox/Build/GameContent/Shaders Shaders", NULL, SW_SHOW);

	}
	else if (strcmp(lpszCmdLine, "Clean") == 0)
	{

	}

	//system("PAUSE.EXE");
}