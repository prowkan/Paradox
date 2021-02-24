@echo off

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader.dxbc MaterialBase_VertexShader.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_PixelShader.dxbc MaterialBase_PixelShader.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\FullScreenQuad.dxbc FullScreenQuad.hlsl

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\OcclusionBuffer.dxbc OcclusionBuffer.hlsl