@echo off

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader_GBufferOpaquePass.dxbc MaterialBase_VertexShader_GBufferOpaquePass.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_PixelShader_GBufferOpaquePass.dxbc MaterialBase_PixelShader_GBufferOpaquePass.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader_ShadowMapPass.dxbc MaterialBase_VertexShader_ShadowMapPass.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SkyVertexShader.dxbc SkyVertexShader.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SkyPixelShader.dxbc SkyPixelShader.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SunVertexShader.dxbc SunVertexShader.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SunPixelShader.dxbc SunPixelShader.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\FullScreenQuad.dxbc FullScreenQuad.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\ShadowResolve.dxbc ShadowResolve.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\DeferredLighting.dxbc DeferredLighting.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\Fog.dxbc Fog.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\HDRToneMapping.dxbc HDRToneMapping.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T cs_5_1 -E CS -Zpr -Fo .\..\..\Build\GameContent\Shaders\LuminanceCalc.dxbc LuminanceCalc.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T cs_5_1 -E CS -Zpr -Fo .\..\..\Build\GameContent\Shaders\LuminanceSum.dxbc LuminanceSum.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T cs_5_1 -E CS -Zpr -Fo .\..\..\Build\GameContent\Shaders\LuminanceAvg.dxbc LuminanceAvg.hlsl