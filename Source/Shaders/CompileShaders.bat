@echo off

"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -fvk-invert-y -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader.spv MaterialBase_VertexShader.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_PixelShader.spv MaterialBase_PixelShader.hlsl

"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -fvk-invert-y -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader_GBufferOpaquePass.spv MaterialBase_VertexShader_GBufferOpaquePass.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_PixelShader_GBufferOpaquePass.spv MaterialBase_PixelShader_GBufferOpaquePass.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -fvk-invert-y  -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\MaterialBase_VertexShader_ShadowMapPass.spv MaterialBase_VertexShader_ShadowMapPass.hlsl

"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -fvk-invert-y -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SkyVertexShader.spv SkyVertexShader.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SkyPixelShader.spv SkyPixelShader.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -fvk-invert-y -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SunVertexShader.spv SunVertexShader.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\SunPixelShader.spv SunPixelShader.hlsl

"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -fvk-invert-y -T vs_5_1 -E VS -Zpr -Fo .\..\..\Build\GameContent\Shaders\FullScreenQuad.spv FullScreenQuad.hlsl

"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\OcclusionBuffer.spv OcclusionBuffer.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\ShadowResolve.spv ShadowResolve.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\DeferredLighting.spv DeferredLighting.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\Fog.spv Fog.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\BrightPass.spv BrightPass.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\ImageResample.spv ImageResample.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\HorizontalBlur.spv HorizontalBlur.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\VerticalBlur.spv VerticalBlur.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T ps_5_1 -E PS -Zpr -Fo .\..\..\Build\GameContent\Shaders\HDRToneMapping.spv HDRToneMapping.hlsl

"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T cs_5_1 -E CS -Zpr -Fo .\..\..\Build\GameContent\Shaders\LuminanceCalc.spv LuminanceCalc.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T cs_5_1 -E CS -Zpr -Fo .\..\..\Build\GameContent\Shaders\LuminanceSum.spv LuminanceSum.hlsl
"C:\VulkanSDK\1.2.162.1\Bin\dxc.exe" -spirv -T cs_5_1 -E CS -Zpr -Fo .\..\..\Build\GameContent\Shaders\LuminanceAvg.spv LuminanceAvg.hlsl