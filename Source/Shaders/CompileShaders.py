import subprocess

FXCompiler = "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/fxc.exe"
DXCompiler = "C:/VulkanSDK/1.2.170.0/Bin/dxc.exe"

OutputDirSM1 = "./../../Build/GameContent/Shaders/ShaderModel51/"
OutputDirSPV = "./../../Build/GameContent/Shaders/SPIRV/"

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

for VertexShader in VertexShaders:
    subprocess.run([FXCompiler, "-T", "vs_5_1", "-E", "VS", "-Zpr", "-Fo", OutputDirSM1 + VertexShader + ".dxbc", VertexShader + ".hlsl"])
    subprocess.run([DXCompiler, "-D", "SPIRV", "-spirv", "-fvk-invert-y", "-T", "vs_5_1", "-E", "VS", "-Zpr", "-Fo", OutputDirSPV + VertexShader + ".spv", VertexShader + ".hlsl"])

for PixelShader in PixelShaders:
    subprocess.run([FXCompiler, "-T", "ps_5_1", "-E", "PS", "-Zpr", "-Fo", OutputDirSM1 + PixelShader + ".dxbc", PixelShader + ".hlsl"])
    subprocess.run([DXCompiler, "-D", "SPIRV", "-spirv", "-T", "ps_5_1", "-E", "PS", "-Zpr", "-Fo", OutputDirSPV + PixelShader + ".spv", PixelShader + ".hlsl"])

for ComputeShader in ComputeShaders:
    subprocess.run([FXCompiler, "-T", "cs_5_1", "-E", "CS", "-Zpr", "-Fo", OutputDirSM1 + ComputeShader + ".dxbc", ComputeShader + ".hlsl"])
    subprocess.run([DXCompiler, "-D", "SPIRV", "-spirv", "-T", "cs_5_1", "-E", "CS", "-Zpr", "-Fo", OutputDirSPV + ComputeShader + ".spv", ComputeShader + ".hlsl"])