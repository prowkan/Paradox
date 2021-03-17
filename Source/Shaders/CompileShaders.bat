@echo off

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T vs_6_3 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader_GBufferOpaquePass.dxil MaterialBase_VertexShader_GBufferOpaquePass.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_PixelShader_GBufferOpaquePass.dxil MaterialBase_PixelShader_GBufferOpaquePass.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T vs_6_3 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader_ShadowMapPass.dxil MaterialBase_VertexShader_ShadowMapPass.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T vs_6_3 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SkyVertexShader.dxil SkyVertexShader.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SkyPixelShader.dxil SkyPixelShader.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T vs_6_3 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SunVertexShader.dxil SunVertexShader.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SunPixelShader.dxil SunPixelShader.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T vs_6_3 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\FullScreenQuad.dxil FullScreenQuad.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\OcclusionBuffer.dxil OcclusionBuffer.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\ShadowResolve.dxil ShadowResolve.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\DeferredLighting.dxil DeferredLighting.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\Fog.dxil Fog.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\BrightPass.dxil BrightPass.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\ImageResample.dxil ImageResample.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\HorizontalBlur.dxil HorizontalBlur.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\VerticalBlur.dxil VerticalBlur.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T ps_6_3 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\HDRToneMapping.dxil HDRToneMapping.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T cs_6_3 -E CS -Zpr -Fo .\..\..\Build\GameContent\Shaders\LuminanceCalc.dxil LuminanceCalc.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T cs_6_3 -E CS -Zpr -Fo .\..\..\Build\GameContent\Shaders\LuminanceSum.dxil LuminanceSum.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe" -T cs_6_3 -E CS -Zpr -Fo .\..\..\Build\GameContent\Shaders\LuminanceAvg.dxil LuminanceAvg.hlsl