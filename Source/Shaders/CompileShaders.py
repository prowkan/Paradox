import subprocess

FXCompiler = "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/fxc.exe"

OutputDir = "./../../Build/GameContent/Shaders/"

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
    subprocess.run([FXCompiler, "-T", "vs_5_1", "-E", "VS", "-Zpr", "-Fo", OutputDir + VertexShader + ".dxbc", VertexShader + ".hlsl"])

for PixelShader in PixelShaders:
    subprocess.run([FXCompiler, "-T", "ps_5_1", "-E", "PS", "-Zpr", "-Fo", OutputDir + PixelShader + ".dxbc", PixelShader + ".hlsl"])

for ComputeShader in ComputeShaders:
    subprocess.run([FXCompiler, "-T", "cs_5_1", "-E", "CS", "-Zpr", "-Fo", OutputDir + ComputeShader + ".dxbc", ComputeShader + ".hlsl"])