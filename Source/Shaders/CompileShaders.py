import subprocess

FXCompiler = "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/fxc.exe"

OutputDirSM50 = "./../../Build/GameContent/Shaders/ShaderModel50/"
OutputDirSM51 = "./../../Build/GameContent/Shaders/ShaderModel51/"

VertexShaders = [
"MaterialBase_VertexShader_GBufferOpaquePass",
"MaterialBase_VertexShader_ShadowMapPass",
"SkyVertexShader",
"SunVertexShader",
"FullScreenQuad"
]

PixelShaders = [
"MaterialBase_PixelShader_GBufferOpaquePass",
"SkyPixelShader",
"SunPixelShader",
"OcclusionBuffer",
"MSAADepthResolve",
"ShadowResolve",
"DeferredLighting",
"Fog",
"BrightPass",
"ImageResample",
"HorizontalBlur",
"VerticalBlur",
"HDRToneMapping",
]

ComputeShaders = [
"LuminanceCalc",    
"LuminanceSum",    
"LuminanceAvg"    
]

# subprocess.run([FXCompiler, "-T", "vs_5_1", "-E", "VS", "-Zpr", "-Fo", "./../../Build/GameContent/Shaders/MaterialBase_VertexShader.dxbc", "MaterialBase_VertexShader.hlsl"])
# subprocess.run([FXCompiler, "-T", "ps_5_1", "-E", "PS", "-Zpr", "-Fo", "./../../Build/GameContent/Shaders/MaterialBase_PixelShader.dxbc", "MaterialBase_PixelShader.hlsl"])

for VertexShader in VertexShaders:
    subprocess.run([FXCompiler, "-D", "SHADER_MODEL=50", "-T", "vs_5_0", "-E", "VS", "-Zpr", "-Fo", OutputDirSM50 + VertexShader + ".dxbc", VertexShader + ".hlsl"])
    subprocess.run([FXCompiler, "-D", "SHADER_MODEL=51", "-T", "vs_5_1", "-E", "VS", "-Zpr", "-Fo", OutputDirSM51 + VertexShader + ".dxbc", VertexShader + ".hlsl"])

for PixelShader in PixelShaders:
    subprocess.run([FXCompiler, "-D", "SHADER_MODEL=50", "-T", "ps_5_0", "-E", "PS", "-Zpr", "-Fo", OutputDirSM50 + PixelShader + ".dxbc", PixelShader + ".hlsl"])
    subprocess.run([FXCompiler, "-D", "SHADER_MODEL=51", "-T", "ps_5_1", "-E", "PS", "-Zpr", "-Fo", OutputDirSM51 + PixelShader + ".dxbc", PixelShader + ".hlsl"])

for ComputeShader in ComputeShaders:
    subprocess.run([FXCompiler, "-D", "SHADER_MODEL=50", "-T", "cs_5_0", "-E", "CS", "-Zpr", "-Fo", OutputDirSM50 + ComputeShader + ".dxbc", ComputeShader + ".hlsl"])
    subprocess.run([FXCompiler, "-D", "SHADER_MODEL=51", "-T", "cs_5_1", "-E", "CS", "-Zpr", "-Fo", OutputDirSM51 + ComputeShader + ".dxbc", ComputeShader + ".hlsl"])