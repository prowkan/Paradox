@echo off

"C:\VulkanSDK\1.2.170.0\Bin\dxc.exe" -spirv -fvk-invert-y -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader.spv MaterialBase_VertexShader.hlsl
"C:\VulkanSDK\1.2.170.0\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_PixelShader.spv MaterialBase_PixelShader.hlsl