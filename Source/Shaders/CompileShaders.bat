@echo off

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T vs_5_1 -E VS -Zpr -enable_unbounded_descriptor_tables -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader.dxbc MaterialBase_VertexShader.hlsl
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe" -T ps_5_1 -E PS -Zpr -enable_unbounded_descriptor_tables -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_PixelShader.dxbc MaterialBase_PixelShader.hlsl