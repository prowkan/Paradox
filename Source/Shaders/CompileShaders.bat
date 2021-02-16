@echo off

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T vs_5_1 -E VS -Zpr -Fo MaterialBase_VertexShader.dxbc MaterialBase_VertexShader.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo MaterialBase_PixelShader.dxbc MaterialBase_PixelShader.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T vs_5_1 -E VS -Zpr -Fo FullScreenQuad.dxbc FullScreenQuad.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo DeferredLighting.dxbc DeferredLighting.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo HDRToneMapping.dxbc HDRToneMapping.hlsl

move /Y *.dxbc .\..\..\Build\GameContent\Shaders\ShaderModel51