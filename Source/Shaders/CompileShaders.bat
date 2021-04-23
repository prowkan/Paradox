@echo off

"F:\DXCompiler\bin\x64\dxc.exe" -T vs_6_6 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader.dxil MaterialBase_VertexShader.hlsl
"F:\DXCompiler\bin\x64\dxc.exe" -T ps_6_6 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_PixelShader.dxil MaterialBase_PixelShader.hlsl