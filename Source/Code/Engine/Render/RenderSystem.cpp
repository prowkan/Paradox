// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderSystem.h"

#include <Core/Application.h>

#include <Engine/Engine.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

void RenderSystem::InitSystem()
{
	UINT FactoryCreationFlags = 0;
	UINT DeviceCreationFlags = 0;
	
#ifdef _DEBUG
	FactoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
	DeviceCreationFlags |= D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
#endif

	COMRCPtr<IDXGIFactory7> Factory;

	SAFE_DX(CreateDXGIFactory2(FactoryCreationFlags, UUIDOF(Factory)));

	COMRCPtr<IDXGIAdapter> Adapter;

	SAFE_DX(Factory->EnumAdapters(0, &Adapter));

	COMRCPtr<IDXGIOutput> Monitor;

	SAFE_DX(Adapter->EnumOutputs(0, &Monitor));

	UINT DisplayModesCount;
	SAFE_DX(Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, nullptr));
	DXGI_MODE_DESC *DisplayModes = new DXGI_MODE_DESC[(size_t)DisplayModesCount];
	SAFE_DX(Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, DisplayModes));

	/*ResolutionWidth = DisplayModes[(size_t)DisplayModesCount - 1].Width;
	ResolutionHeight = DisplayModes[(size_t)DisplayModesCount - 1].Height;*/

	ResolutionWidth = 1280;
	ResolutionHeight = 720;

	D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;

	SAFE_DX(D3D11CreateDevice(Adapter, D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN, NULL, DeviceCreationFlags, &FeatureLevel, 1, D3D11_SDK_VERSION, &Device, nullptr, &DeviceContext));

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
	SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_UNSPECIFIED;
	SwapChainDesc.BufferCount = 2;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	SwapChainDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.Height = ResolutionHeight;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH;
	SwapChainDesc.Stereo = FALSE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	SwapChainDesc.Width = ResolutionWidth;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC SwapChainFullScreenDesc;
	SwapChainFullScreenDesc.RefreshRate.Numerator = DisplayModes[(size_t)DisplayModesCount - 1].RefreshRate.Numerator;
	SwapChainFullScreenDesc.RefreshRate.Denominator = DisplayModes[(size_t)DisplayModesCount - 1].RefreshRate.Denominator;
	SwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainFullScreenDesc.Windowed = TRUE;

	COMRCPtr<IDXGISwapChain1> SwapChain1;
	SAFE_DX(Factory->CreateSwapChainForHwnd(Device, Application::GetMainWindowHandle(), &SwapChainDesc, &SwapChainFullScreenDesc, nullptr, &SwapChain1));
	SAFE_DX(SwapChain1->QueryInterface<IDXGISwapChain4>(&SwapChain));

	SAFE_DX(Factory->MakeWindowAssociation(Application::GetMainWindowHandle(), DXGI_MWA_NO_ALT_ENTER));

	delete[] DisplayModes;

	SAFE_DX(SwapChain->GetBuffer(0, UUIDOF(BackBufferTexture)));

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateRenderTargetView(BackBufferTexture, &RTVDesc, &BackBufferRTV));

	D3D11_TEXTURE2D_DESC TextureDesc;
	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Height = ResolutionHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 8;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	TextureDesc.Width = ResolutionWidth;

	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	
	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &GBufferTextures[0]));

	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &GBufferTextures[1]));

	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2DMS;

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SAFE_DX(Device->CreateRenderTargetView(GBufferTextures[0], &RTVDesc, &GBufferRTVs[0]));
	
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	SAFE_DX(Device->CreateRenderTargetView(GBufferTextures[1], &RTVDesc, &GBufferRTVs[1]));

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2DMS;

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SAFE_DX(Device->CreateShaderResourceView(GBufferTextures[0], &SRVDesc, &GBufferSRVs[0]));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	SAFE_DX(Device->CreateShaderResourceView(GBufferTextures[1], &SRVDesc, &GBufferSRVs[1]));

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS;
	TextureDesc.Height = ResolutionHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 8;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	TextureDesc.Width = ResolutionWidth;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &DepthBufferTexture));

	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	DSVDesc.Flags = 0;
	DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2DMS;

	SAFE_DX(Device->CreateDepthStencilView(DepthBufferTexture, &DSVDesc, &DepthBufferDSV));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2DMS;

	SAFE_DX(Device->CreateShaderResourceView(DepthBufferTexture, &SRVDesc, &DepthBufferSRV));

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	TextureDesc.Height = ResolutionHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	TextureDesc.Width = ResolutionWidth;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &ResolvedDepthBufferTexture));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateShaderResourceView(ResolvedDepthBufferTexture, &SRVDesc, &ResolvedDepthBufferSRV));

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS;
	TextureDesc.Height = 2048;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	TextureDesc.Width = 2048;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &CascadedShadowMapTextures[0]));
	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &CascadedShadowMapTextures[1]));
	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &CascadedShadowMapTextures[2]));
	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &CascadedShadowMapTextures[3]));

	DSVDesc.Flags = 0;
	DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	DSVDesc.Texture2D.MipSlice = 0;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateDepthStencilView(CascadedShadowMapTextures[0], &DSVDesc, &CascadedShadowMapDSVs[0]));
	SAFE_DX(Device->CreateDepthStencilView(CascadedShadowMapTextures[1], &DSVDesc, &CascadedShadowMapDSVs[1]));
	SAFE_DX(Device->CreateDepthStencilView(CascadedShadowMapTextures[2], &DSVDesc, &CascadedShadowMapDSVs[2]));
	SAFE_DX(Device->CreateDepthStencilView(CascadedShadowMapTextures[3], &DSVDesc, &CascadedShadowMapDSVs[3]));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateShaderResourceView(CascadedShadowMapTextures[0], &SRVDesc, &CascadedShadowMapSRVs[0]));
	SAFE_DX(Device->CreateShaderResourceView(CascadedShadowMapTextures[1], &SRVDesc, &CascadedShadowMapSRVs[1]));
	SAFE_DX(Device->CreateShaderResourceView(CascadedShadowMapTextures[2], &SRVDesc, &CascadedShadowMapSRVs[2]));
	SAFE_DX(Device->CreateShaderResourceView(CascadedShadowMapTextures[3], &SRVDesc, &CascadedShadowMapSRVs[3]));

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	TextureDesc.Height = ResolutionHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	TextureDesc.Width = ResolutionWidth;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &ShadowMaskTexture));

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateRenderTargetView(ShadowMaskTexture, &RTVDesc, &ShadowMaskRTV));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateShaderResourceView(ShadowMaskTexture, &SRVDesc, &ShadowMaskSRV));

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	TextureDesc.Height = ResolutionHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 8;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	TextureDesc.Width = ResolutionWidth;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &LBufferTexture));

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2DMS;

	SAFE_DX(Device->CreateRenderTargetView(LBufferTexture, &RTVDesc, &LBufferRTV));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2DMS;

	SAFE_DX(Device->CreateShaderResourceView(LBufferTexture, &SRVDesc, &LBufferSRV));

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	TextureDesc.Height = ResolutionHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	TextureDesc.Width = ResolutionWidth;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &ResolvedHDRSceneColorTexture));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateShaderResourceView(ResolvedHDRSceneColorTexture, &SRVDesc, &ResolvedHDRSceneColorSRV));

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	//TextureDesc.Height = ResolutionHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	//TextureDesc.Width = ResolutionWidth;

	int Widths[4] = { 1280, 80, 5, 1 };
	int Heights[4] = { 720, 45, 3, 1 };

	for (int i = 0; i < 4; i++)
	{
		TextureDesc.Width = Widths[i];
		TextureDesc.Height = Heights[i];

		SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &SceneLuminanceTextures[i]));
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 4; i++)
	{
		SAFE_DX(Device->CreateUnorderedAccessView(SceneLuminanceTextures[i], &UAVDesc, &SceneLuminanceUAVs[i]));
	}

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 4; i++)
	{
		SAFE_DX(Device->CreateShaderResourceView(SceneLuminanceTextures[i], &SRVDesc, &SceneLuminanceSRVs[i]));
	}

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	TextureDesc.Height = 1;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	TextureDesc.Width = 1;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &AverageLuminanceTexture));

	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateUnorderedAccessView(AverageLuminanceTexture, &UAVDesc, &AverageLuminanceUAV));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateShaderResourceView(AverageLuminanceTexture, &SRVDesc, &AverageLuminanceSRV));

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	//TextureDesc.Height = ResolutionHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	//TextureDesc.Width = ResolutionWidth;

	for (int i = 0; i < 7; i++)
	{
		TextureDesc.Width = ResolutionWidth >> i;
		TextureDesc.Height = ResolutionHeight >> i;

		SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &BloomTextures[0][i]));
		SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &BloomTextures[1][i]));
		SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &BloomTextures[2][i]));
	}

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 7; i++)
	{
		SAFE_DX(Device->CreateRenderTargetView(BloomTextures[0][i], &RTVDesc, &BloomRTVs[0][i]));
		SAFE_DX(Device->CreateRenderTargetView(BloomTextures[1][i], &RTVDesc, &BloomRTVs[1][i]));
		SAFE_DX(Device->CreateRenderTargetView(BloomTextures[2][i], &RTVDesc, &BloomRTVs[2][i]));
	}

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 7; i++)
	{
		SAFE_DX(Device->CreateShaderResourceView(BloomTextures[0][i], &SRVDesc, &BloomSRVs[0][i]));
		SAFE_DX(Device->CreateShaderResourceView(BloomTextures[1][i], &SRVDesc, &BloomSRVs[1][i]));
		SAFE_DX(Device->CreateShaderResourceView(BloomTextures[2][i], &SRVDesc, &BloomSRVs[2][i]));
	}

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	TextureDesc.Height = ResolutionHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 8;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	TextureDesc.Width = ResolutionWidth;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &ToneMappedImageTexture));

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2DMS;

	SAFE_DX(Device->CreateRenderTargetView(ToneMappedImageTexture, &RTVDesc, &ToneMappedImageRTV));

	D3D11_BUFFER_DESC BufferDesc;
	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.ByteWidth = 256;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

	SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &ShadowResolveConstantBuffer));

	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.ByteWidth = 256;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

	SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &DeferredLightingConstantBuffer));

	UINT SkyMeshVertexCount = 1 + 25 * 100 + 1;
	UINT SkyMeshIndexCount = 300 + 24 * 600 + 300;

	Vertex SkyMeshVertices[1 + 25 * 100 + 1];
	WORD SkyMeshIndices[300 + 24 * 600 + 300];

	SkyMeshVertices[0].Position = XMFLOAT3(0.0f, 1.0f, 0.0f);
	SkyMeshVertices[0].TexCoord = XMFLOAT2(0.5f, 0.5f);
	
	SkyMeshVertices[1 + 25 * 100].Position = XMFLOAT3(0.0f, -1.0f, 0.0f);
	SkyMeshVertices[1 + 25 * 100].TexCoord = XMFLOAT2(0.5f, 0.5f);

	for (int i = 0; i < 100; i++)
	{
		for (int j = 0; j < 25; j++)
		{
			float Theta = ((j + 1) / 25.0f) * 0.5f * 3.1416f;
			float Phi = (i / 100.0f) * 2.0f * 3.1416f;

			float X = sinf(Theta) * cosf(Phi);
			float Y = sinf(Theta) * sinf(Phi);
			float Z = cosf(Theta);

			SkyMeshVertices[1 + j * 100 + i].Position = XMFLOAT3(X, Z, Y);
			SkyMeshVertices[1 + j * 100 + i].TexCoord = XMFLOAT2(X * 0.5f + 0.5f, Y * 0.5f + 0.5f);
		}
	}

	for (int i = 0; i < 100; i++)
	{
		SkyMeshIndices[3 * i] = 0;
		SkyMeshIndices[3 * i + 1] = i + 1;
		SkyMeshIndices[3 * i + 2] = i != 99 ? i + 2 : 1;
	}

	for (int j = 0; j < 24; j++)
	{
		for (int i = 0; i < 100; i++)
		{
			SkyMeshIndices[300 + j * 600 + 6 * i] = 1 + i + j * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 1] = 1 + i + (j + 1) * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 2] = i != 99 ? 1 + i + 1 + (j + 1) * 100 : 1 + (j + 1) * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 3] = 1 + i + j * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 4] = i != 99 ? 1 + i + 1 + (j + 1) * 100 : 1 + (j + 1) * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 5] = i != 99 ? 1 + i + 1 + j * 100 : 1 + j * 100;
		}
	}

	for (int i = 0; i < 100; i++)
	{
		SkyMeshIndices[300 + 24 * 600 + 3 * i] = 1 + 25 * 100;
		SkyMeshIndices[300 + 24 * 600 + 3 * i + 1] = i != 99 ? 1 + 24 * 100 + i + 1 : 1 + 24 * 100;
		SkyMeshIndices[300 + 24 * 600 + 3 * i + 2] = 1 + 24 * 100 + i;
	}

	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = sizeof(Vertex) * SkyMeshVertexCount;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
	
	D3D11_SUBRESOURCE_DATA SubResourceData;
	SubResourceData.pSysMem = SkyMeshVertices;
	SubResourceData.SysMemPitch = 0;
	SubResourceData.SysMemSlicePitch = 0;
	
	SAFE_DX(Device->CreateBuffer(&BufferDesc, &SubResourceData, &SkyVertexBuffer));

	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;
	BufferDesc.ByteWidth = sizeof(WORD) * SkyMeshIndexCount;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

	SubResourceData.pSysMem = SkyMeshIndices;
	SubResourceData.SysMemPitch = 0;
	SubResourceData.SysMemSlicePitch = 0;	

	SAFE_DX(Device->CreateBuffer(&BufferDesc, &SubResourceData, &SkyIndexBuffer));

	Texel *SkyTextureTexels = new Texel[2048 * 2048];

	for (int y = 0; y < 2048; y++)
	{
		for (int x = 0; x < 2048; x++)
		{
			float X = (x / 2048.0f) * 2.0f - 1.0f;
			float Y = (y / 2048.0f) * 2.0f - 1.0f;

			float D = sqrtf(X * X + Y * Y);

			SkyTextureTexels[y * 2048 + x].R = 0;
			SkyTextureTexels[y * 2048 + x].G = 128 * D < 255 ? 128 * D : 255;
			SkyTextureTexels[y * 2048 + x].B = 255;
			SkyTextureTexels[y * 2048 + x].A = 255;
		}
	}

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	TextureDesc.Height = 2048;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
	TextureDesc.Width = 2048;

	SubResourceData.pSysMem = SkyTextureTexels;
	SubResourceData.SysMemPitch = 4 * 2048;
	SubResourceData.SysMemSlicePitch = 0;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, &SubResourceData, &SkyTexture));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateShaderResourceView(SkyTexture, &SRVDesc, &SkyTextureSRV));

	delete[] SkyTextureTexels;

	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.ByteWidth = 256;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

	SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &SkyConstantBuffer));

	UINT SunMeshVertexCount = 4;
	UINT SunMeshIndexCount = 6;

	Vertex SunMeshVertices[4] = {

		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }

	};

	WORD SunMeshIndices[6] = { 0, 1, 2, 2, 1, 3};

	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = sizeof(Vertex) * SunMeshVertexCount;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

	SubResourceData.pSysMem = SunMeshVertices;
	SubResourceData.SysMemPitch = 0;
	SubResourceData.SysMemSlicePitch = 0;

	SAFE_DX(Device->CreateBuffer(&BufferDesc, &SubResourceData, &SunVertexBuffer));

	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;
	BufferDesc.ByteWidth = sizeof(WORD) * SunMeshIndexCount;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

	SubResourceData.pSysMem = SunMeshIndices;
	SubResourceData.SysMemPitch = 0;
	SubResourceData.SysMemSlicePitch = 0;

	SAFE_DX(Device->CreateBuffer(&BufferDesc, &SubResourceData, &SunIndexBuffer));

	Texel *SunTextureTexels = new Texel[512 * 512];

	for (int y = 0; y < 512; y++)
	{
		for (int x = 0; x < 512; x++)
		{
			float X = (x / 512.0f) * 2.0f - 1.0f;
			float Y = (y / 512.0f) * 2.0f - 1.0f;

			float D = sqrtf(X * X + Y * Y);

			SunTextureTexels[y * 512 + x].R = 255;
			SunTextureTexels[y * 512 + x].G = 255;
			SunTextureTexels[y * 512 + x].B = 127 + 128 * D < 255 ? 127 + 128 * D : 255;
			SunTextureTexels[y * 512 + x].A = 255 * D < 255 ? 255 * D : 255;
		}
	}

	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	TextureDesc.Height = 512;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
	TextureDesc.Width = 512;

	SubResourceData.pSysMem = SunTextureTexels;
	SubResourceData.SysMemPitch = 4 * 512;
	SubResourceData.SysMemSlicePitch = 0;

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, &SubResourceData, &SunTexture));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateShaderResourceView(SunTexture, &SRVDesc, &SunTextureSRV));

	delete[] SunTextureTexels;

	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.ByteWidth = 256;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

	SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &SunConstantBuffer));

	HANDLE FullScreenQuadVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/FullScreenQuad.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER FullScreenQuadVertexShaderByteCodeLength;
	BOOL Result = GetFileSizeEx(FullScreenQuadVertexShaderFile, &FullScreenQuadVertexShaderByteCodeLength);
	void *FullScreenQuadVertexShaderByteCodeData = malloc(FullScreenQuadVertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(FullScreenQuadVertexShaderFile, FullScreenQuadVertexShaderByteCodeData, (DWORD)FullScreenQuadVertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(FullScreenQuadVertexShaderFile);

	HANDLE ShadowResolvePixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ShadowResolve.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER ShadowResolvePixelShaderByteCodeLength;
	Result = GetFileSizeEx(ShadowResolvePixelShaderFile, &ShadowResolvePixelShaderByteCodeLength);
	void *ShadowResolvePixelShaderByteCodeData = malloc(ShadowResolvePixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(ShadowResolvePixelShaderFile, ShadowResolvePixelShaderByteCodeData, (DWORD)ShadowResolvePixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(ShadowResolvePixelShaderFile);

	HANDLE DeferredLightingPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/DeferredLighting.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER DeferredLightingPixelShaderByteCodeLength;
	Result = GetFileSizeEx(DeferredLightingPixelShaderFile, &DeferredLightingPixelShaderByteCodeLength);
	void *DeferredLightingPixelShaderByteCodeData = malloc(DeferredLightingPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(DeferredLightingPixelShaderFile, DeferredLightingPixelShaderByteCodeData, (DWORD)DeferredLightingPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(DeferredLightingPixelShaderFile);

	HANDLE FogPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/Fog.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER FogPixelShaderByteCodeLength;
	Result = GetFileSizeEx(FogPixelShaderFile, &FogPixelShaderByteCodeLength);
	void *FogPixelShaderByteCodeData = malloc(FogPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(FogPixelShaderFile, FogPixelShaderByteCodeData, (DWORD)FogPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(FogPixelShaderFile);

	HANDLE HDRToneMappingPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/HDRToneMapping.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER HDRToneMappingPixelShaderByteCodeLength;
	Result = GetFileSizeEx(HDRToneMappingPixelShaderFile, &HDRToneMappingPixelShaderByteCodeLength);
	void *HDRToneMappingPixelShaderByteCodeData = malloc(HDRToneMappingPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(HDRToneMappingPixelShaderFile, HDRToneMappingPixelShaderByteCodeData, (DWORD)HDRToneMappingPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(HDRToneMappingPixelShaderFile);

	HANDLE LuminanceCalcComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceCalc.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LuminanceCalcComputeShaderByteCodeLength;
	Result = GetFileSizeEx(LuminanceCalcComputeShaderFile, &LuminanceCalcComputeShaderByteCodeLength);
	void *LuminanceCalcComputeShaderByteCodeData = malloc(LuminanceCalcComputeShaderByteCodeLength.QuadPart);
	Result = ReadFile(LuminanceCalcComputeShaderFile, LuminanceCalcComputeShaderByteCodeData, (DWORD)LuminanceCalcComputeShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(LuminanceCalcComputeShaderFile);

	HANDLE LuminanceSumComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceSum.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LuminanceSumComputeShaderByteCodeLength;
	Result = GetFileSizeEx(LuminanceSumComputeShaderFile, &LuminanceSumComputeShaderByteCodeLength);
	void *LuminanceSumComputeShaderByteCodeData = malloc(LuminanceSumComputeShaderByteCodeLength.QuadPart);
	Result = ReadFile(LuminanceSumComputeShaderFile, LuminanceSumComputeShaderByteCodeData, (DWORD)LuminanceSumComputeShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(LuminanceSumComputeShaderFile);

	HANDLE LuminanceAvgComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceAvg.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LuminanceAvgComputeShaderByteCodeLength;
	Result = GetFileSizeEx(LuminanceAvgComputeShaderFile, &LuminanceAvgComputeShaderByteCodeLength);
	void *LuminanceAvgComputeShaderByteCodeData = malloc(LuminanceAvgComputeShaderByteCodeLength.QuadPart);
	Result = ReadFile(LuminanceAvgComputeShaderFile, LuminanceAvgComputeShaderByteCodeData, (DWORD)LuminanceAvgComputeShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(LuminanceAvgComputeShaderFile);

	HANDLE SkyVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SkyVertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SkyVertexShaderByteCodeLength;
	Result = GetFileSizeEx(SkyVertexShaderFile, &SkyVertexShaderByteCodeLength);
	void *SkyVertexShaderByteCodeData = malloc(SkyVertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(SkyVertexShaderFile, SkyVertexShaderByteCodeData, (DWORD)SkyVertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SkyVertexShaderFile);

	HANDLE SkyPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SkyPixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SkyPixelShaderByteCodeLength;
	Result = GetFileSizeEx(SkyPixelShaderFile, &SkyPixelShaderByteCodeLength);
	void *SkyPixelShaderByteCodeData = malloc(SkyPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(SkyPixelShaderFile, SkyPixelShaderByteCodeData, (DWORD)SkyPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SkyPixelShaderFile);

	HANDLE SunVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SunVertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SunVertexShaderByteCodeLength;
	Result = GetFileSizeEx(SunVertexShaderFile, &SunVertexShaderByteCodeLength);
	void *SunVertexShaderByteCodeData = malloc(SunVertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(SunVertexShaderFile, SunVertexShaderByteCodeData, (DWORD)SunVertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SunVertexShaderFile);

	HANDLE SunPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SunPixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SunPixelShaderByteCodeLength;
	Result = GetFileSizeEx(SunPixelShaderFile, &SunPixelShaderByteCodeLength);
	void *SunPixelShaderByteCodeData = malloc(SunPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(SunPixelShaderFile, SunPixelShaderByteCodeData, (DWORD)SunPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SunPixelShaderFile);

	HANDLE BrightPassPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/BrightPass.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER BrightPassPixelShaderByteCodeLength;
	Result = GetFileSizeEx(BrightPassPixelShaderFile, &BrightPassPixelShaderByteCodeLength);
	void *BrightPassPixelShaderByteCodeData = malloc(BrightPassPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(BrightPassPixelShaderFile, BrightPassPixelShaderByteCodeData, (DWORD)BrightPassPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(BrightPassPixelShaderFile);

	HANDLE ImageResamplePixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ImageResample.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER ImageResamplePixelShaderByteCodeLength;
	Result = GetFileSizeEx(ImageResamplePixelShaderFile, &ImageResamplePixelShaderByteCodeLength);
	void *ImageResamplePixelShaderByteCodeData = malloc(ImageResamplePixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(ImageResamplePixelShaderFile, ImageResamplePixelShaderByteCodeData, (DWORD)ImageResamplePixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(ImageResamplePixelShaderFile);

	HANDLE HorizontalBlurPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/HorizontalBlur.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER HorizontalBlurPixelShaderByteCodeLength;
	Result = GetFileSizeEx(HorizontalBlurPixelShaderFile, &HorizontalBlurPixelShaderByteCodeLength);
	void *HorizontalBlurPixelShaderByteCodeData = malloc(HorizontalBlurPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(HorizontalBlurPixelShaderFile, HorizontalBlurPixelShaderByteCodeData, (DWORD)HorizontalBlurPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(HorizontalBlurPixelShaderFile);

	HANDLE VerticalBlurPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/VerticalBlur.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER VerticalBlurPixelShaderByteCodeLength;
	Result = GetFileSizeEx(VerticalBlurPixelShaderFile, &VerticalBlurPixelShaderByteCodeLength);
	void *VerticalBlurPixelShaderByteCodeData = malloc(VerticalBlurPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(VerticalBlurPixelShaderFile, VerticalBlurPixelShaderByteCodeData, (DWORD)VerticalBlurPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(VerticalBlurPixelShaderFile);

	SAFE_DX(Device->CreateVertexShader(FullScreenQuadVertexShaderByteCodeData, FullScreenQuadVertexShaderByteCodeLength.QuadPart, nullptr, &FullScreenQuadVertexShader));

	SAFE_DX(Device->CreatePixelShader(ShadowResolvePixelShaderByteCodeData, ShadowResolvePixelShaderByteCodeLength.QuadPart, nullptr, &ShadowResolvePixelShader));
	SAFE_DX(Device->CreatePixelShader(DeferredLightingPixelShaderByteCodeData, DeferredLightingPixelShaderByteCodeLength.QuadPart, nullptr, &DeferredLightingPixelShader));
	SAFE_DX(Device->CreatePixelShader(FogPixelShaderByteCodeData, FogPixelShaderByteCodeLength.QuadPart, nullptr, &FogPixelShader));
	SAFE_DX(Device->CreatePixelShader(HDRToneMappingPixelShaderByteCodeData, HDRToneMappingPixelShaderByteCodeLength.QuadPart, nullptr, &HDRToneMappingPixelShader));
	SAFE_DX(Device->CreatePixelShader(BrightPassPixelShaderByteCodeData, BrightPassPixelShaderByteCodeLength.QuadPart, nullptr, &BrightPassPixelShader));
	SAFE_DX(Device->CreatePixelShader(ImageResamplePixelShaderByteCodeData, ImageResamplePixelShaderByteCodeLength.QuadPart, nullptr, &ImageResamplePixelShader));
	SAFE_DX(Device->CreatePixelShader(HorizontalBlurPixelShaderByteCodeData, HorizontalBlurPixelShaderByteCodeLength.QuadPart, nullptr, &HorizontalBlurPixelShader));
	SAFE_DX(Device->CreatePixelShader(VerticalBlurPixelShaderByteCodeData, VerticalBlurPixelShaderByteCodeLength.QuadPart, nullptr, &VerticalBlurPixelShader));

	SAFE_DX(Device->CreateVertexShader(SkyVertexShaderByteCodeData, SkyVertexShaderByteCodeLength.QuadPart, nullptr, &SkyVertexShader));
	SAFE_DX(Device->CreatePixelShader(SkyPixelShaderByteCodeData, SkyPixelShaderByteCodeLength.QuadPart, nullptr, &SkyPixelShader));
	SAFE_DX(Device->CreateVertexShader(SunVertexShaderByteCodeData, SunVertexShaderByteCodeLength.QuadPart, nullptr, &SunVertexShader));
	SAFE_DX(Device->CreatePixelShader(SunPixelShaderByteCodeData, SunPixelShaderByteCodeLength.QuadPart, nullptr, &SunPixelShader));

	/*ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphisRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = FogPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = FogPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(FogPipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphisRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = ImageResamplePixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = ImageResamplePixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(UpSampleWithAddBlendPipelineState)));*/

	SAFE_DX(Device->CreateComputeShader(LuminanceCalcComputeShaderByteCodeData, LuminanceCalcComputeShaderByteCodeLength.QuadPart, nullptr, &LuminanceCalcComputeShader));
	SAFE_DX(Device->CreateComputeShader(LuminanceSumComputeShaderByteCodeData, LuminanceSumComputeShaderByteCodeLength.QuadPart, nullptr, &LuminanceSumComputeShader));
	SAFE_DX(Device->CreateComputeShader(LuminanceAvgComputeShaderByteCodeData, LuminanceAvgComputeShaderByteCodeLength.QuadPart, nullptr, &LuminanceAvgComputeShader));

	D3D11_INPUT_ELEMENT_DESC InputElementDescs[5];
	InputElementDescs[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[0].InputSlot = 0;
	InputElementDescs[0].InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA;
	InputElementDescs[0].InstanceDataStepRate = 0;
	InputElementDescs[0].SemanticIndex = 0;
	InputElementDescs[0].SemanticName = "POSITION";
	InputElementDescs[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[1].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	InputElementDescs[1].InputSlot = 0;
	InputElementDescs[1].InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA;
	InputElementDescs[1].InstanceDataStepRate = 0;
	InputElementDescs[1].SemanticIndex = 0;
	InputElementDescs[1].SemanticName = "TEXCOORD";
	InputElementDescs[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[2].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[2].InputSlot = 0;
	InputElementDescs[2].InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA;
	InputElementDescs[2].InstanceDataStepRate = 0;
	InputElementDescs[2].SemanticIndex = 0;
	InputElementDescs[2].SemanticName = "NORMAL";
	InputElementDescs[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[3].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[3].InputSlot = 0;
	InputElementDescs[3].InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA;
	InputElementDescs[3].InstanceDataStepRate = 0;
	InputElementDescs[3].SemanticIndex = 0;
	InputElementDescs[3].SemanticName = "TANGENT";
	InputElementDescs[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[4].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[4].InputSlot = 0;
	InputElementDescs[4].InputSlotClass = D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA;
	InputElementDescs[4].InstanceDataStepRate = 0;
	InputElementDescs[4].SemanticIndex = 0;
	InputElementDescs[4].SemanticName = "BINORMAL";

	/*ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 5;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphisRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = SkyPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = SkyPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = SkyVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = SkyVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(SkyPipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 5;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphisRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = SunPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = SunPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = SunVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = SunVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(SunPipelineState)));*/

	free(FullScreenQuadVertexShaderByteCodeData);
	free(DeferredLightingPixelShaderByteCodeData);
	free(HDRToneMappingPixelShaderByteCodeData);
	//free(SkyVertexShaderByteCodeData);
	free(SkyPixelShaderByteCodeData);
	free(SunVertexShaderByteCodeData);
	free(SunPixelShaderByteCodeData);

	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.ByteWidth = 256;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

	SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &ConstantBuffer));

	for (int j = 0; j < 4; j++)
	{
		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		BufferDesc.ByteWidth = 256;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &ConstantBuffers[j]));
	}

	D3D11_SAMPLER_DESC SamplerDesc;
	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.ComparisonFunc = (D3D11_COMPARISON_FUNC)0;
	SamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_ANISOTROPIC;
	SamplerDesc.MaxAnisotropy = 16;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MipLODBias = 0.0f;

	SAFE_DX(Device->CreateSamplerState(&SamplerDesc, &TextureSampler));

	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS;
	SamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MipLODBias = 0.0f;

	SAFE_DX(Device->CreateSamplerState(&SamplerDesc, &ShadowMapSampler));

	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.ComparisonFunc = (D3D11_COMPARISON_FUNC)0;
	SamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MipLODBias = 0.0f;

	SAFE_DX(Device->CreateSamplerState(&SamplerDesc, &BiLinearSampler));

	SAFE_DX(Device->CreateInputLayout(InputElementDescs, 5, SkyVertexShaderByteCodeData, SkyVertexShaderByteCodeLength.QuadPart, &InputLayout));

	D3D11_RASTERIZER_DESC RasterizerDesc;
	ZeroMemory(&RasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	RasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
	RasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;

	SAFE_DX(Device->CreateRasterizerState(&RasterizerDesc, &RasterizerState));

	D3D11_BLEND_DESC BlendDesc;
	ZeroMemory(&BlendDesc, sizeof(D3D11_BLEND_DESC));
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;

	SAFE_DX(Device->CreateBlendState(&BlendDesc, &BlendDisabledBlendState));

	ZeroMemory(&BlendDesc, sizeof(D3D11_BLEND_DESC));
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;
	BlendDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND::D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ZERO;

	SAFE_DX(Device->CreateBlendState(&BlendDesc, &FogBlendState));

	ZeroMemory(&BlendDesc, sizeof(D3D11_BLEND_DESC));
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;
	BlendDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND::D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ZERO;

	SAFE_DX(Device->CreateBlendState(&BlendDesc, &SunBlendState));

	ZeroMemory(&BlendDesc, sizeof(D3D11_BLEND_DESC));
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;
	BlendDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND::D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND::D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ZERO;

	SAFE_DX(Device->CreateBlendState(&BlendDesc, &AdditiveBlendState));

	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
	ZeroMemory(&DepthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	DepthStencilDesc.DepthEnable = TRUE;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;

	SAFE_DX(Device->CreateDepthStencilState(&DepthStencilDesc, &GBufferPassDepthStencilState));

	ZeroMemory(&DepthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	DepthStencilDesc.DepthEnable = TRUE;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;

	SAFE_DX(Device->CreateDepthStencilState(&DepthStencilDesc, &ShadowMapPassDepthStencilState));

	ZeroMemory(&DepthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	DepthStencilDesc.DepthEnable = TRUE;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO;

	SAFE_DX(Device->CreateDepthStencilState(&DepthStencilDesc, &SkyAndSunDepthStencilState));

	ZeroMemory(&DepthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	SAFE_DX(Device->CreateDepthStencilState(&DepthStencilDesc, &DepthDisabledDepthStencilState));
}

void RenderSystem::ShutdownSystem()
{
	for (RenderMesh* renderMesh : RenderMeshDestructionQueue)
	{
		delete renderMesh;
	}

	RenderMeshDestructionQueue.clear();

	for (RenderMaterial* renderMaterial : RenderMaterialDestructionQueue)
	{
		delete renderMaterial;
	}

	RenderMaterialDestructionQueue.clear();

	for (RenderTexture* renderTexture : RenderTextureDestructionQueue)
	{
		delete renderTexture;
	}

	RenderTextureDestructionQueue.clear();
}

void RenderSystem::TickSystem(float DeltaTime)
{
	DeviceContext->ClearState();

	DeviceContext->IASetInputLayout(InputLayout);
	DeviceContext->RSSetState(RasterizerState);
	DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
	DeviceContext->OMSetDepthStencilState(GBufferPassDepthStencilState, 0);

	XMMATRIX ViewProjMatrix = Engine::GetEngine().GetGameFramework().GetCamera().GetViewProjMatrix();

	vector<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
	vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

	OPTICK_EVENT("Draw Calls")

	ID3D11RenderTargetView *GBufferRTVs[2] = { this->GBufferRTVs[0], this->GBufferRTVs[1] };

	DeviceContext->OMSetRenderTargets(2, GBufferRTVs, DepthBufferDSV);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_VIEWPORT Viewport;
	Viewport.Height = float(ResolutionHeight);
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = float(ResolutionWidth);

	DeviceContext->RSSetViewports(1, &Viewport);

	DeviceContext->PSSetSamplers(0, 1, &TextureSampler);

	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	DeviceContext->ClearRenderTargetView(GBufferRTVs[0], ClearColor);
	DeviceContext->ClearRenderTargetView(GBufferRTVs[1], ClearColor);
	DeviceContext->ClearDepthStencilView(DepthBufferDSV, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL, 0.0f, 0);

	for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
	{
		StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

		RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
		RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
		RenderTexture *renderTexture0 = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
		RenderTexture *renderTexture1 = staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

		XMMATRIX WorldMatrix = VisbleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
		XMMATRIX WVPMatrix = WorldMatrix * ViewProjMatrix;
		XMFLOAT3X4 VectorTransformMatrix;

		float Determinant =
			WorldMatrix.m[0][0] * (WorldMatrix.m[1][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[1][2]) -
			WorldMatrix.m[1][0] * (WorldMatrix.m[0][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[0][2]) +
			WorldMatrix.m[2][0] * (WorldMatrix.m[0][1] * WorldMatrix.m[1][2] - WorldMatrix.m[1][1] * WorldMatrix.m[0][2]);

		VectorTransformMatrix.m[0][0] = (WorldMatrix.m[1][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[1][2]) / Determinant;
		VectorTransformMatrix.m[1][0] = -(WorldMatrix.m[0][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[0][2]) / Determinant;
		VectorTransformMatrix.m[2][0] = (WorldMatrix.m[0][1] * WorldMatrix.m[1][2] - WorldMatrix.m[1][1] * WorldMatrix.m[0][2]) / Determinant;

		VectorTransformMatrix.m[0][1] = -(WorldMatrix.m[1][0] * WorldMatrix.m[2][2] - WorldMatrix.m[2][0] * WorldMatrix.m[1][2]) / Determinant;
		VectorTransformMatrix.m[1][1] = (WorldMatrix.m[0][0] * WorldMatrix.m[2][2] - WorldMatrix.m[2][0] * WorldMatrix.m[0][2]) / Determinant;
		VectorTransformMatrix.m[2][1] = -(WorldMatrix.m[0][0] * WorldMatrix.m[1][0] - WorldMatrix.m[0][2] * WorldMatrix.m[1][2]) / Determinant;

		VectorTransformMatrix.m[0][2] = (WorldMatrix.m[1][0] * WorldMatrix.m[2][1] - WorldMatrix.m[2][0] * WorldMatrix.m[1][1]) / Determinant;
		VectorTransformMatrix.m[1][2] = -(WorldMatrix.m[0][0] * WorldMatrix.m[2][1] - WorldMatrix.m[2][0] * WorldMatrix.m[0][1]) / Determinant;
		VectorTransformMatrix.m[2][2] = (WorldMatrix.m[0][0] * WorldMatrix.m[1][1] - WorldMatrix.m[1][0] * WorldMatrix.m[0][1]) / Determinant;

		VectorTransformMatrix.m[0][3] = 0.0f;
		VectorTransformMatrix.m[1][3] = 0.0f;
		VectorTransformMatrix.m[2][3] = 0.0f;

		D3D11_MAPPED_SUBRESOURCE MappedSubResource;

		SAFE_DX(DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource));

		memcpy((BYTE*)MappedSubResource.pData, &WVPMatrix, sizeof(XMMATRIX));
		memcpy((BYTE*)MappedSubResource.pData + sizeof(XMMATRIX), &WorldMatrix, sizeof(XMMATRIX));
		memcpy((BYTE*)MappedSubResource.pData + 2 * sizeof(XMMATRIX), &VectorTransformMatrix, sizeof(XMFLOAT3X4));

		DeviceContext->Unmap(ConstantBuffer, 0);

		UINT Stride = sizeof(Vertex), Offset = 0;

		DeviceContext->IASetVertexBuffers(0, 1, &renderMesh->VertexBuffer, &Stride, &Offset);
		DeviceContext->IASetIndexBuffer(renderMesh->IndexBuffer, DXGI_FORMAT::DXGI_FORMAT_R16_UINT, 0);

		DeviceContext->VSSetShader(renderMaterial->GBufferOpaquePassVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(renderMaterial->GBufferOpaquePassPixelShader, nullptr, 0);

		DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
		DeviceContext->PSSetShaderResources(0, 1, &renderTexture0->TextureSRV);
		DeviceContext->PSSetShaderResources(1, 1, &renderTexture1->TextureSRV);

		DeviceContext->DrawIndexed(8 * 8 * 6 * 6, 0, 0);
	}

	Viewport.Height = 2048.0f;
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = 2048.0f;

	DeviceContext->RSSetViewports(1, &Viewport);

	XMFLOAT3 CameraLocation = Engine::GetEngine().GetGameFramework().GetCamera().GetCameraLocation();

	XMMATRIX ShadowViewMatrices[4], ShadowProjMatrices[4], ShadowViewProjMatrices[4];

	ShadowViewMatrices[0] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 10.0f, CameraLocation.y + 10.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[1] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 20.0f, CameraLocation.y + 20.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[2] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 50.0f, CameraLocation.y + 50.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[3] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 100.0f, CameraLocation.y + 100.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	ShadowProjMatrices[0] = XMMatrixOrthographicLH(10.0f, 10.0f, 0.01f, 500.0f);
	ShadowProjMatrices[1] = XMMatrixOrthographicLH(20.0f, 20.0f, 0.01f, 500.0f);
	ShadowProjMatrices[2] = XMMatrixOrthographicLH(50.0f, 50.0f, 0.01f, 500.0f);
	ShadowProjMatrices[3] = XMMatrixOrthographicLH(100.0f, 100.0f, 0.01f, 500.0f);

	ShadowViewProjMatrices[0] = ShadowViewMatrices[0] * ShadowProjMatrices[0];
	ShadowViewProjMatrices[1] = ShadowViewMatrices[1] * ShadowProjMatrices[1];
	ShadowViewProjMatrices[2] = ShadowViewMatrices[2] * ShadowProjMatrices[2];
	ShadowViewProjMatrices[3] = ShadowViewMatrices[3] * ShadowProjMatrices[3];

	DeviceContext->OMSetDepthStencilState(ShadowMapPassDepthStencilState, 0);

	for (int i = 0; i < 4; i++)
	{
		SIZE_T ConstantBufferOffset = 0;

		vector<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
		vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ShadowViewProjMatrices[i]);
		size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

		DeviceContext->OMSetRenderTargets(0, nullptr, CascadedShadowMapDSVs[i]);

		DeviceContext->ClearDepthStencilView(CascadedShadowMapDSVs[i], D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, 1.0f, 0);

		for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
		{
			StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

			RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
			RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
			RenderTexture *renderTexture0 = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
			RenderTexture *renderTexture1 = staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

			XMMATRIX WorldMatrix = VisbleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
			XMMATRIX WVPMatrix = WorldMatrix * ShadowViewProjMatrices[i];

			D3D11_MAPPED_SUBRESOURCE MappedSubResource;

			SAFE_DX(DeviceContext->Map(ConstantBuffers[i], 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource));

			memcpy((BYTE*)MappedSubResource.pData, &WVPMatrix, sizeof(XMMATRIX));

			DeviceContext->Unmap(ConstantBuffers[i], 0);

			UINT Stride = sizeof(Vertex), Offset = 0;

			DeviceContext->IASetVertexBuffers(0, 1, &renderMesh->VertexBuffer, &Stride, &Offset);
			DeviceContext->IASetIndexBuffer(renderMesh->IndexBuffer, DXGI_FORMAT::DXGI_FORMAT_R16_UINT, 0);

			DeviceContext->VSSetShader(renderMaterial->ShadowMapPassVertexShader, nullptr, 0);
			DeviceContext->PSSetShader(nullptr, nullptr, 0);

			DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffers[i]);

			DeviceContext->DrawIndexed(8 * 8 * 6 * 6, 0, 0);
		}
	}

	DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

	/*COMRCPtr<ID3D12GraphicsCommandList1> CommandList1;

	CommandList->QueryInterface<ID3D12GraphicsCommandList1>(&CommandList1);

	CommandList1->ResolveSubresourceRegion(ResolvedDepthBufferTexture, 0, 0, 0, DepthBufferTexture, 0, nullptr, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT, D3D12_RESOLVE_MODE::D3D12_RESOLVE_MODE_MAX);*/

	Viewport.Height = float(ResolutionHeight);
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = float(ResolutionWidth);

	DeviceContext->RSSetViewports(1, &Viewport);

	DeviceContext->OMSetRenderTargets(1, &ShadowMaskRTV, nullptr);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	XMMATRIX InvViewProjMatrices[4];
	InvViewProjMatrices[0] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[0];
	InvViewProjMatrices[1] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[1];
	InvViewProjMatrices[2] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[2];
	InvViewProjMatrices[3] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[3];

	D3D11_MAPPED_SUBRESOURCE MappedSubResource;

	SAFE_DX(DeviceContext->Map(ShadowResolveConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource));

	memcpy(MappedSubResource.pData, InvViewProjMatrices, 4 * sizeof(XMMATRIX));

	DeviceContext->Unmap(ShadowResolveConstantBuffer, 0);

	DeviceContext->PSSetSamplers(0, 1, &ShadowMapSampler);

	DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(ShadowResolvePixelShader, nullptr, 0);

	DeviceContext->PSSetConstantBuffers(0, 1, &ShadowResolveConstantBuffer);
	DeviceContext->PSSetShaderResources(0, 1, &ResolvedDepthBufferSRV);
	DeviceContext->PSSetShaderResources(1, 1, &CascadedShadowMapSRVs[0]);
	DeviceContext->PSSetShaderResources(2, 1, &CascadedShadowMapSRVs[1]);
	DeviceContext->PSSetShaderResources(3, 1, &CascadedShadowMapSRVs[2]);
	DeviceContext->PSSetShaderResources(4, 1, &CascadedShadowMapSRVs[3]);

	DeviceContext->Draw(4, 0);

	DeviceContext->OMSetRenderTargets(1, &LBufferRTV, nullptr);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	XMMATRIX InvViewProjMatrix = XMMatrixInverse(nullptr, ViewProjMatrix);

	SAFE_DX(DeviceContext->Map(DeferredLightingConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource));

	memcpy(MappedSubResource.pData, &InvViewProjMatrix, sizeof(XMMATRIX));
	memcpy((BYTE*)MappedSubResource.pData + sizeof(XMMATRIX), &CameraLocation, sizeof(XMFLOAT3));

	DeviceContext->Unmap(DeferredLightingConstantBuffer, 0);

	DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(DeferredLightingPixelShader, nullptr, 0);

	DeviceContext->PSSetConstantBuffers(0, 1, &DeferredLightingConstantBuffer);
	DeviceContext->PSSetShaderResources(0, 1, &GBufferSRVs[0]);
	DeviceContext->PSSetShaderResources(1, 1, &GBufferSRVs[1]);
	DeviceContext->PSSetShaderResources(2, 1, &DepthBufferSRV);
	DeviceContext->PSSetShaderResources(3, 1, &ShadowMaskSRV);

	DeviceContext->Draw(4, 0);

	DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(FogPixelShader, nullptr, 0);

	DeviceContext->OMSetBlendState(FogBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);

	DeviceContext->PSSetShaderResources(0, 1, &DepthBufferSRV);

	DeviceContext->Draw(4, 0);

	DeviceContext->OMSetDepthStencilState(SkyAndSunDepthStencilState, 0);
	DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);

	DeviceContext->OMSetRenderTargets(1, &LBufferRTV, DepthBufferDSV);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX SkyWorldMatrix = XMMatrixScaling(900.0f, 900.0f, 900.0f) * XMMatrixTranslation(CameraLocation.x, CameraLocation.y, CameraLocation.z);
	XMMATRIX SkyWVPMatrix = SkyWorldMatrix * ViewProjMatrix;

	SAFE_DX(DeviceContext->Map(SkyConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource));

	memcpy(MappedSubResource.pData, &SkyWVPMatrix, sizeof(XMMATRIX));

	DeviceContext->Unmap(SkyConstantBuffer, 0);

	DeviceContext->PSSetSamplers(0, 1, &TextureSampler);

	UINT Stride = sizeof(Vertex), Offset = 0;

	DeviceContext->IASetVertexBuffers(0, 1, &SkyVertexBuffer, &Stride, &Offset);
	DeviceContext->IASetIndexBuffer(SkyIndexBuffer, DXGI_FORMAT::DXGI_FORMAT_R16_UINT, 0);

	DeviceContext->VSSetShader(SkyVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(SkyPixelShader, nullptr, 0);

	DeviceContext->VSSetConstantBuffers(0, 1, &SkyConstantBuffer);
	DeviceContext->PSSetShaderResources(0, 1, &SkyTextureSRV);

	DeviceContext->DrawIndexed(300 + 24 * 600 + 300, 0, 0);

	XMMATRIX ViewMatrix = Engine::GetEngine().GetGameFramework().GetCamera().GetViewMatrix();
	XMMATRIX ProjMatrix = Engine::GetEngine().GetGameFramework().GetCamera().GetProjMatrix();

	XMFLOAT3 SunPosition(-500.0f + CameraLocation.x, 500.0f + CameraLocation.y, -500.f + CameraLocation.z);
	
	SAFE_DX(DeviceContext->Map(SunConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource));

	memcpy(MappedSubResource.pData, &ViewMatrix, sizeof(XMMATRIX));
	memcpy((BYTE*)MappedSubResource.pData + sizeof(XMMATRIX), &ProjMatrix, sizeof(XMMATRIX));
	memcpy((BYTE*)MappedSubResource.pData + 2 * sizeof(XMMATRIX), &SunPosition, sizeof(XMFLOAT3));

	DeviceContext->Unmap(SunConstantBuffer, 0);

	DeviceContext->IASetVertexBuffers(0, 1, &SunVertexBuffer, &Stride, &Offset);
	DeviceContext->IASetIndexBuffer(SunIndexBuffer, DXGI_FORMAT::DXGI_FORMAT_R16_UINT, 0);

	DeviceContext->VSSetShader(SunVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(SunPixelShader, nullptr, 0);
	DeviceContext->OMSetBlendState(SunBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);

	DeviceContext->VSSetConstantBuffers(0, 1, &SunConstantBuffer);
	DeviceContext->PSSetShaderResources(0, 1, &SunTextureSRV);

	DeviceContext->DrawIndexed(6, 0, 0);

	DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);
	DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);

	DeviceContext->ResolveSubresource(ResolvedHDRSceneColorTexture, 0, LBufferTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT);

	DeviceContext->CSSetShader(LuminanceCalcComputeShader, nullptr, 0);
	DeviceContext->CSSetUnorderedAccessViews(0, 1, &SceneLuminanceUAVs[0], nullptr);
	DeviceContext->CSSetShaderResources(0, 1, &ResolvedHDRSceneColorSRV);

	DeviceContext->Dispatch(80, 45, 1);

	DeviceContext->CSSetShader(LuminanceSumComputeShader, nullptr, 0);
	
	DeviceContext->CSSetUnorderedAccessViews(0, 1, &SceneLuminanceUAVs[1], nullptr);
	DeviceContext->CSSetShaderResources(0, 1, &SceneLuminanceSRVs[0]);

	DeviceContext->Dispatch(80, 45, 1);

	DeviceContext->CSSetUnorderedAccessViews(0, 1, &SceneLuminanceUAVs[2], nullptr);
	DeviceContext->CSSetShaderResources(0, 1, &SceneLuminanceSRVs[1]);

	DeviceContext->Dispatch(5, 3, 1);

	DeviceContext->CSSetUnorderedAccessViews(0, 1, &SceneLuminanceUAVs[3], nullptr);
	DeviceContext->CSSetShaderResources(0, 1, &SceneLuminanceSRVs[2]);

	DeviceContext->Dispatch(1, 1, 1);

	DeviceContext->CSSetShader(LuminanceAvgComputeShader, nullptr, 0);

	DeviceContext->CSSetUnorderedAccessViews(0, 1, &AverageLuminanceUAV, nullptr);
	DeviceContext->CSSetShaderResources(0, 1, &SceneLuminanceSRVs[3]);

	DeviceContext->Dispatch(1, 1, 1);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	Viewport.Height = FLOAT(ResolutionHeight);
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = FLOAT(ResolutionWidth);

	DeviceContext->RSSetViewports(1, &Viewport);	

	DeviceContext->OMSetRenderTargets(1, &BloomRTVs[0][0], nullptr);

	DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(BrightPassPixelShader, nullptr, 0);

	DeviceContext->PSSetShaderResources(0, 1, &ResolvedHDRSceneColorSRV);
	DeviceContext->PSSetShaderResources(1, 1, &SceneLuminanceSRVs[0]);

	DeviceContext->Draw(4, 0);

	DeviceContext->OMSetRenderTargets(1, &BloomRTVs[1][0], nullptr);
	
	DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(HorizontalBlurPixelShader, nullptr, 0);

	DeviceContext->PSSetShaderResources(0, 1, &BloomSRVs[0][0]);

	DeviceContext->Draw(4, 0);

	DeviceContext->OMSetRenderTargets(1, &BloomRTVs[2][0], nullptr);

	DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(VerticalBlurPixelShader, nullptr, 0);

	DeviceContext->PSSetShaderResources(0, 1, &BloomSRVs[1][0]);

	DeviceContext->Draw(4, 0);

	for (int i = 1; i < 7; i++)
	{
		Viewport.Height = FLOAT(ResolutionHeight >> i);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth >> i);

		DeviceContext->RSSetViewports(1, &Viewport);

		DeviceContext->OMSetRenderTargets(1, &BloomRTVs[0][i], nullptr);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(ImageResamplePixelShader, nullptr, 0);

		DeviceContext->PSSetShaderResources(0, 1, &BloomSRVs[0][i - 1]);

		DeviceContext->Draw(4, 0);

		DeviceContext->OMSetRenderTargets(1, &BloomRTVs[1][i], nullptr);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(HorizontalBlurPixelShader, nullptr, 0);

		DeviceContext->PSSetShaderResources(0, 1, &BloomSRVs[0][i]);

		DeviceContext->Draw(4, 0);

		DeviceContext->OMSetRenderTargets(1, &BloomRTVs[2][i], nullptr);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(VerticalBlurPixelShader, nullptr, 0);

		DeviceContext->PSSetShaderResources(0, 1, &BloomSRVs[1][i]);

		DeviceContext->Draw(4, 0);
	}

	for (int i = 5; i >= 0; i--)
	{
		Viewport.Height = FLOAT(ResolutionHeight >> i);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth >> i);

		DeviceContext->RSSetViewports(1, &Viewport);

		DeviceContext->OMSetRenderTargets(1, &BloomRTVs[2][i], nullptr);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(ImageResamplePixelShader, nullptr, 0);

		DeviceContext->OMSetBlendState(AdditiveBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);

		DeviceContext->PSSetShaderResources(0, 1, &BloomSRVs[2][i + 1]);

		DeviceContext->Draw(4, 0);
	}

	Viewport.Height = FLOAT(ResolutionHeight);
	Viewport.Height = float(ResolutionHeight);
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = FLOAT(ResolutionWidth);

	DeviceContext->RSSetViewports(1, &Viewport);

	DeviceContext->OMSetRenderTargets(1, &ToneMappedImageRTV, nullptr);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
	DeviceContext->PSSetShader(HDRToneMappingPixelShader, nullptr, 0);

	DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);

	DeviceContext->PSSetShaderResources(0, 1, &LBufferSRV);
	DeviceContext->PSSetShaderResources(1, 1, &BloomSRVs[2][0]);

	DeviceContext->Draw(4, 0);

	DeviceContext->ResolveSubresource(BackBufferTexture, 0, ToneMappedImageTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

	SAFE_DX(SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
}

RenderMesh* RenderSystem::CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo)
{
	RenderMesh *renderMesh = new RenderMesh();

	D3D11_BUFFER_DESC BufferDesc;
	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = sizeof(Vertex) * renderMeshCreateInfo.VertexCount;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA SubResourceData;
	SubResourceData.pSysMem = renderMeshCreateInfo.VertexData;
	SubResourceData.SysMemPitch = 0;
	SubResourceData.SysMemSlicePitch = 0;

	SAFE_DX(Device->CreateBuffer(&BufferDesc, &SubResourceData, &renderMesh->VertexBuffer));

	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;
	BufferDesc.ByteWidth = sizeof(WORD) * renderMeshCreateInfo.IndexCount;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

	SubResourceData.pSysMem = renderMeshCreateInfo.IndexData;
	SubResourceData.SysMemPitch = 0;
	SubResourceData.SysMemSlicePitch = 0;

	SAFE_DX(Device->CreateBuffer(&BufferDesc, &SubResourceData, &renderMesh->IndexBuffer));

	return renderMesh;
}

RenderTexture* RenderSystem::CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo)
{
	RenderTexture *renderTexture = new RenderTexture();

	DXGI_FORMAT TextureFormat;

	if (renderTextureCreateInfo.Compressed)
	{
		if (renderTextureCreateInfo.SRGB)
		{
			switch (renderTextureCreateInfo.CompressionType)
			{
				case BlockCompression::BC1:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB;
					break;
				case BlockCompression::BC2:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB;
					break;
				case BlockCompression::BC3:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB;
					break;
				default:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
					break;
			}
		}
		else
		{
			switch (renderTextureCreateInfo.CompressionType)
			{
				case BlockCompression::BC1:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM;
					break;
				case BlockCompression::BC2:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM;
					break;
				case BlockCompression::BC3:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM;
					break;
				case BlockCompression::BC4:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM;
					break;
				case BlockCompression::BC5:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM;
					break;
				default:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
					break;
			}
		}
	}
	else
	{
		if (renderTextureCreateInfo.SRGB)
		{
			TextureFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		}
		else
		{
			TextureFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	D3D11_TEXTURE2D_DESC TextureDesc;
	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = TextureFormat;
	TextureDesc.Height = renderTextureCreateInfo.Height;
	TextureDesc.MipLevels = renderTextureCreateInfo.MIPLevels;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
	TextureDesc.Width = renderTextureCreateInfo.Width;

	D3D11_SUBRESOURCE_DATA SubResourceData[16];

	BYTE *TexelData = renderTextureCreateInfo.TexelData;

	for (UINT i = 0; i < renderTextureCreateInfo.MIPLevels; i++)
	{
		SubResourceData[i].pSysMem = TexelData;
		SubResourceData[i].SysMemPitch = 8 * ((renderTextureCreateInfo.Width / 4) >> i);
		SubResourceData[i].SysMemSlicePitch = 0;

		if (renderTextureCreateInfo.Compressed)
		{
			if (renderTextureCreateInfo.CompressionType == BlockCompression::BC1)
			{
				SubResourceData[i].SysMemPitch = 8 * ((renderTextureCreateInfo.Width / 4) >> i);
				TexelData += 8 * ((renderTextureCreateInfo.Width / 4) >> i) * ((renderTextureCreateInfo.Height / 4) >> i);
			}
			else if (renderTextureCreateInfo.CompressionType == BlockCompression::BC5)
			{
				SubResourceData[i].SysMemPitch = 16 * ((renderTextureCreateInfo.Width / 4) >> i);
				TexelData += 16 * ((renderTextureCreateInfo.Width / 4) >> i) * ((renderTextureCreateInfo.Height / 4) >> i);
			}
		}
		else
		{
			SubResourceData[i].SysMemPitch = 4 * (renderTextureCreateInfo.Width >> i);
			TexelData += 4 * (renderTextureCreateInfo.Width >> i) * (renderTextureCreateInfo.Height >> i);
		}
	}

	SAFE_DX(Device->CreateTexture2D(&TextureDesc, SubResourceData, &renderTexture->Texture));

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = TextureFormat;
	SRVDesc.Texture2D.MipLevels = renderTextureCreateInfo.MIPLevels;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	SAFE_DX(Device->CreateShaderResourceView(renderTexture->Texture, &SRVDesc, &renderTexture->TextureSRV));

	return renderTexture;
}

RenderMaterial* RenderSystem::CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo)
{
	RenderMaterial *renderMaterial = new RenderMaterial();

	SAFE_DX(Device->CreateVertexShader(renderMaterialCreateInfo.GBufferOpaquePassVertexShaderByteCodeData, renderMaterialCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength, nullptr, &renderMaterial->GBufferOpaquePassVertexShader));
	SAFE_DX(Device->CreatePixelShader(renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeData, renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength, nullptr, &renderMaterial->GBufferOpaquePassPixelShader));
	SAFE_DX(Device->CreateVertexShader(renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeData, renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeLength, nullptr, &renderMaterial->ShadowMapPassVertexShader));
	renderMaterial->ShadowMapPassPixelShader = nullptr;

	return renderMaterial;
}

void RenderSystem::DestroyRenderMesh(RenderMesh* renderMesh)
{
	RenderMeshDestructionQueue.push_back(renderMesh);
}

void RenderSystem::DestroyRenderTexture(RenderTexture* renderTexture)
{
	RenderTextureDestructionQueue.push_back(renderTexture);
}

void RenderSystem::DestroyRenderMaterial(RenderMaterial* renderMaterial)
{
	RenderMaterialDestructionQueue.push_back(renderMaterial);
}

inline void RenderSystem::CheckDXCallResult(HRESULT hr, const char16_t* Function)
{
	if (FAILED(hr))
	{
		char16_t DXErrorMessageBuffer[2048];
		char16_t DXErrorCodeBuffer[512];

		const char16_t *DXErrorCodePtr = GetDXErrorMessageFromHRESULT(hr);

		if (DXErrorCodePtr) wcscpy((wchar_t*)DXErrorCodeBuffer, (const wchar_t*)DXErrorCodePtr);
		else wsprintf((wchar_t*)DXErrorCodeBuffer, (const wchar_t*)u"0x%08X (неизвестный код)", hr);

		wsprintf((wchar_t*)DXErrorMessageBuffer, (const wchar_t*)u"Произошла ошибка при попытке вызова следующей DirectX-функции:\r\n%s\r\nКод ошибки: %s", (const wchar_t*)Function, (const wchar_t*)DXErrorCodeBuffer);

		int IntResult = MessageBox(NULL, (const wchar_t*)DXErrorMessageBuffer, (const wchar_t*)u"Ошибка DirectX", MB_OK | MB_ICONERROR);

		if (hr == DXGI_ERROR_DEVICE_REMOVED) SAFE_DX(Device->GetDeviceRemovedReason());

		ExitProcess(0);
	}
}

inline const char16_t* RenderSystem::GetDXErrorMessageFromHRESULT(HRESULT hr)
{
	switch (hr)
	{
		case E_UNEXPECTED:
			return u"E_UNEXPECTED"; 
			break; 
		case E_NOTIMPL:
			return u"E_NOTIMPL";
			break;
		case E_OUTOFMEMORY:
			return u"E_OUTOFMEMORY";
			break; 
		case E_INVALIDARG:
			return u"E_INVALIDARG";
			break; 
		case E_NOINTERFACE:
			return u"E_NOINTERFACE";
			break; 
		case E_POINTER:
			return u"E_POINTER"; 
			break; 
		case E_HANDLE:
			return u"E_HANDLE"; 
			break;
		case E_ABORT:
			return u"E_ABORT"; 
			break; 
		case E_FAIL:
			return u"E_FAIL"; 
			break;
		case E_ACCESSDENIED:
			return u"E_ACCESSDENIED";
			break; 
		case E_PENDING:
			return u"E_PENDING";
			break; 
		case E_BOUNDS:
			return u"E_BOUNDS";
			break; 
		case E_CHANGED_STATE:
			return u"E_CHANGED_STATE";
			break; 
		case E_ILLEGAL_STATE_CHANGE:
			return u"E_ILLEGAL_STATE_CHANGE";
			break; 
		case E_ILLEGAL_METHOD_CALL:
			return u"E_ILLEGAL_METHOD_CALL";
			break; 
		case E_STRING_NOT_NULL_TERMINATED:
			return u"E_STRING_NOT_NULL_TERMINATED"; 
			break; 
		case E_ILLEGAL_DELEGATE_ASSIGNMENT:
			return u"E_ILLEGAL_DELEGATE_ASSIGNMENT";
			break; 
		case E_ASYNC_OPERATION_NOT_STARTED:
			return u"E_ASYNC_OPERATION_NOT_STARTED"; 
			break; 
		case E_APPLICATION_EXITING:
			return u"E_APPLICATION_EXITING";
			break; 
		case E_APPLICATION_VIEW_EXITING:
			return u"E_APPLICATION_VIEW_EXITING";
			break; 
		case DXGI_ERROR_INVALID_CALL:
			return u"DXGI_ERROR_INVALID_CALL";
			break; 
		case DXGI_ERROR_NOT_FOUND:
			return u"DXGI_ERROR_NOT_FOUND";
			break; 
		case DXGI_ERROR_MORE_DATA:
			return u"DXGI_ERROR_MORE_DATA";
			break; 
		case DXGI_ERROR_UNSUPPORTED:
			return u"DXGI_ERROR_UNSUPPORTED";
			break; 
		case DXGI_ERROR_DEVICE_REMOVED:
			return u"DXGI_ERROR_DEVICE_REMOVED";
			break; 
		case DXGI_ERROR_DEVICE_HUNG:
			return u"DXGI_ERROR_DEVICE_HUNG";
			break; 
		case DXGI_ERROR_DEVICE_RESET:
			return u"DXGI_ERROR_DEVICE_RESET";
			break; 
		case DXGI_ERROR_WAS_STILL_DRAWING:
			return u"DXGI_ERROR_WAS_STILL_DRAWING";
			break; 
		case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
			return u"DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
			break; 
		case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
			return u"DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE"; 
			break; 
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			return u"DXGI_ERROR_DRIVER_INTERNAL_ERROR";
			break; 
		case DXGI_ERROR_NONEXCLUSIVE:
			return u"DXGI_ERROR_NONEXCLUSIVE";
			break; 
		case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
			return u"DXGI_ERROR_NOT_CURRENTLY_AVAILABLE"; 
			break; 
		case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
			return u"DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";
			break; 
		case DXGI_ERROR_REMOTE_OUTOFMEMORY:
			return u"DXGI_ERROR_REMOTE_OUTOFMEMORY"; 
			break; 
		case DXGI_ERROR_ACCESS_LOST:
			return u"DXGI_ERROR_ACCESS_LOST";
			break; 
		case DXGI_ERROR_WAIT_TIMEOUT:
			return u"DXGI_ERROR_WAIT_TIMEOUT";
			break; 
		case DXGI_ERROR_SESSION_DISCONNECTED:
			return u"DXGI_ERROR_SESSION_DISCONNECTED";
			break; 
		case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
			return u"DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE";
			break; 
		case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
			return u"DXGI_ERROR_CANNOT_PROTECT_CONTENT";
			break; 
		case DXGI_ERROR_ACCESS_DENIED:
			return u"DXGI_ERROR_ACCESS_DENIED"; 
			break; 
		case DXGI_ERROR_NAME_ALREADY_EXISTS:
			return u"DXGI_ERROR_NAME_ALREADY_EXISTS";
			break; 
		case DXGI_ERROR_SDK_COMPONENT_MISSING:
			return u"DXGI_ERROR_SDK_COMPONENT_MISSING"; 
			break; 
		case DXGI_ERROR_NOT_CURRENT:
			return u"DXGI_ERROR_NOT_CURRENT";
			break; 
		case DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY:
			return u"DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY"; 
			break; 
		case DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION:
			return u"DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION"; 
			break; 
		case DXGI_ERROR_NON_COMPOSITED_UI:
			return u"DXGI_ERROR_NON_COMPOSITED_UI";
			break; 
		case DXGI_ERROR_MODE_CHANGE_IN_PROGRESS:
			return u"DXGI_ERROR_MODE_CHANGE_IN_PROGRESS";
			break; 
		case DXGI_ERROR_CACHE_CORRUPT:
			return u"DXGI_ERROR_CACHE_CORRUPT";
			break;
		case DXGI_ERROR_CACHE_FULL:
			return u"DXGI_ERROR_CACHE_FULL";
			break;
		case DXGI_ERROR_CACHE_HASH_COLLISION:
			return u"DXGI_ERROR_CACHE_HASH_COLLISION";
			break;
		case DXGI_ERROR_ALREADY_EXISTS:
			return u"DXGI_ERROR_ALREADY_EXISTS"; 
			break;
		case D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			return u"D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
			break; 
		case D3D10_ERROR_FILE_NOT_FOUND:
			return u"D3D10_ERROR_FILE_NOT_FOUND";
			break;
		case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			return u"D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
			break;
		case D3D11_ERROR_FILE_NOT_FOUND:
			return u"D3D11_ERROR_FILE_NOT_FOUND"; 
			break;
		case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
			return u"D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS"; 
			break; 
		case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
			return u"D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";
			break;
		case D3D12_ERROR_ADAPTER_NOT_FOUND:
			return u"D3D12_ERROR_ADAPTER_NOT_FOUND";
			break;
		case D3D12_ERROR_DRIVER_VERSION_MISMATCH:
			return u"D3D12_ERROR_DRIVER_VERSION_MISMATCH"; 
			break;
		default:
			return nullptr;
			break;
	}

	return nullptr;
}