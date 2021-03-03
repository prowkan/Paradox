// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderSystem.h"

#include <Core/Application.h>

#include <Engine/Engine.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>
#include <Game/Components/Render/Lights/PointLightComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

struct PointLight
{
	XMFLOAT3 Position;
	float Radius;
	XMFLOAT3 Color;
	float Brightness;
};

struct GBufferOpaquePassConstantBufferStruct
{
	XMMATRIX WVPMatrix;
	XMMATRIX WorldMatrix;
	XMFLOAT3X4 VectorTransformMatrix;
};

struct ShadowMapPassConstantBufferStruct
{
	XMMATRIX WVPMatrix;
};

struct ShadowResolveConstantBufferStruct
{
	XMMATRIX ReProjMatrices[4];
};

struct DeferredLightingConstantBufferStruct
{
	XMMATRIX InvViewProjMatrix;
	XMFLOAT3 CameraWorldPosition;
};

struct SkyConstantBufferStruct
{
	XMMATRIX WVPMatrix;
};

struct SunConstantBufferStruct
{
	XMMATRIX ViewMatrix;
	XMMATRIX ProjMatrix;
	XMFLOAT3 SunPosition;
};

void RenderSystem::InitSystem()
{
	clusterizationSubSystem.PreComputeClustersPlanes();

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

	{
		SAFE_DX(SwapChain->GetBuffer(0, UUIDOF(BackBufferTexture)));

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateRenderTargetView(BackBufferTexture, &RTVDesc, &BackBufferTextureRTV));
	}

	{
		HANDLE FullScreenQuadVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/FullScreenQuad.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER FullScreenQuadVertexShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(FullScreenQuadVertexShaderFile, &FullScreenQuadVertexShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> FullScreenQuadVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(FullScreenQuadVertexShaderByteCodeLength.QuadPart);
		Result = ReadFile(FullScreenQuadVertexShaderFile, FullScreenQuadVertexShaderByteCodeData, (DWORD)FullScreenQuadVertexShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(FullScreenQuadVertexShaderFile);

		SAFE_DX(Device->CreateVertexShader(FullScreenQuadVertexShaderByteCodeData, FullScreenQuadVertexShaderByteCodeLength.QuadPart, nullptr, &FullScreenQuadVertexShader));
	}

	// ===============================================================================================================

	{
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

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2DMS;

		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		SAFE_DX(Device->CreateRenderTargetView(GBufferTextures[0], &RTVDesc, &GBufferTexturesRTVs[0]));

		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
		SAFE_DX(Device->CreateRenderTargetView(GBufferTextures[1], &RTVDesc, &GBufferTexturesRTVs[1]));

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2DMS;

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		SAFE_DX(Device->CreateShaderResourceView(GBufferTextures[0], &SRVDesc, &GBufferTexturesSRVs[0]));

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
		SAFE_DX(Device->CreateShaderResourceView(GBufferTextures[1], &SRVDesc, &GBufferTexturesSRVs[1]));

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

		SAFE_DX(Device->CreateDepthStencilView(DepthBufferTexture, &DSVDesc, &DepthBufferTextureDSV));

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2DMS;

		SAFE_DX(Device->CreateShaderResourceView(DepthBufferTexture, &SRVDesc, &DepthBufferTextureSRV));

		D3D11_BUFFER_DESC BufferDesc;
		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		BufferDesc.ByteWidth = 256;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &ConstantBuffer));
	}

	// ===============================================================================================================

	{
		D3D11_TEXTURE2D_DESC TextureDesc;
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

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateRenderTargetView(ResolvedDepthBufferTexture, &RTVDesc, &ResolvedDepthBufferTextureRTV));

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateShaderResourceView(ResolvedDepthBufferTexture, &SRVDesc, &ResolvedDepthBufferTextureSRV));

		HANDLE MSAADepthResolvePixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/MSAADepthResolve.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER MSAADepthResolvePixelShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(MSAADepthResolvePixelShaderFile, &MSAADepthResolvePixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> MSAADepthResolvePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(MSAADepthResolvePixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(MSAADepthResolvePixelShaderFile, MSAADepthResolvePixelShaderByteCodeData, (DWORD)MSAADepthResolvePixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(MSAADepthResolvePixelShaderFile);

		SAFE_DX(Device->CreatePixelShader(MSAADepthResolvePixelShaderByteCodeData, MSAADepthResolvePixelShaderByteCodeLength.QuadPart, nullptr, &MSAADepthResolvePixelShader));
	}

	// ===============================================================================================================

	{
		D3D11_TEXTURE2D_DESC TextureDesc;
		TextureDesc.ArraySize = 1;
		TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
		TextureDesc.CPUAccessFlags = 0;
		TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		TextureDesc.Height = 144;
		TextureDesc.MipLevels = 1;
		TextureDesc.MiscFlags = 0;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
		TextureDesc.Width = 256;

		SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &OcclusionBufferTexture));

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateRenderTargetView(OcclusionBufferTexture, &RTVDesc, &OcclusionBufferTextureRTV));

		TextureDesc.ArraySize = 1;
		TextureDesc.BindFlags = 0;
		TextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ;
		TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		TextureDesc.Height = 144;
		TextureDesc.MipLevels = 1;
		TextureDesc.MiscFlags = 0;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_STAGING;
		TextureDesc.Width = 256;

		SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &OcclusionBufferStagingTextures[0]));
		SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &OcclusionBufferStagingTextures[1]));
		SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &OcclusionBufferStagingTextures[2]));

		HANDLE OcclusionBufferPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/OcclusionBuffer.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER OcclusionBufferPixelShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(OcclusionBufferPixelShaderFile, &OcclusionBufferPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> OcclusionBufferPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(OcclusionBufferPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(OcclusionBufferPixelShaderFile, OcclusionBufferPixelShaderByteCodeData, (DWORD)OcclusionBufferPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(OcclusionBufferPixelShaderFile);

		SAFE_DX(Device->CreatePixelShader(OcclusionBufferPixelShaderByteCodeData, OcclusionBufferPixelShaderByteCodeLength.QuadPart, nullptr, &OcclusionBufferPixelShader));
	}

	// ===============================================================================================================

	{
		D3D11_TEXTURE2D_DESC TextureDesc;
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

		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
		DSVDesc.Flags = 0;
		DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		DSVDesc.Texture2D.MipSlice = 0;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateDepthStencilView(CascadedShadowMapTextures[0], &DSVDesc, &CascadedShadowMapTexturesDSVs[0]));
		SAFE_DX(Device->CreateDepthStencilView(CascadedShadowMapTextures[1], &DSVDesc, &CascadedShadowMapTexturesDSVs[1]));
		SAFE_DX(Device->CreateDepthStencilView(CascadedShadowMapTextures[2], &DSVDesc, &CascadedShadowMapTexturesDSVs[2]));
		SAFE_DX(Device->CreateDepthStencilView(CascadedShadowMapTextures[3], &DSVDesc, &CascadedShadowMapTexturesDSVs[3]));

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateShaderResourceView(CascadedShadowMapTextures[0], &SRVDesc, &CascadedShadowMapTexturesSRVs[0]));
		SAFE_DX(Device->CreateShaderResourceView(CascadedShadowMapTextures[1], &SRVDesc, &CascadedShadowMapTexturesSRVs[1]));
		SAFE_DX(Device->CreateShaderResourceView(CascadedShadowMapTextures[2], &SRVDesc, &CascadedShadowMapTexturesSRVs[2]));
		SAFE_DX(Device->CreateShaderResourceView(CascadedShadowMapTextures[3], &SRVDesc, &CascadedShadowMapTexturesSRVs[3]));

		D3D11_BUFFER_DESC BufferDesc;
		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		BufferDesc.ByteWidth = 256;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &ConstantBuffers[0]));
		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &ConstantBuffers[1]));
		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &ConstantBuffers[2]));
		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &ConstantBuffers[3]));
	}

	// ===============================================================================================================

	{
		D3D11_TEXTURE2D_DESC TextureDesc;
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

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateRenderTargetView(ShadowMaskTexture, &RTVDesc, &ShadowMaskTextureRTV));

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateShaderResourceView(ShadowMaskTexture, &SRVDesc, &ShadowMaskTextureSRV));

		D3D11_BUFFER_DESC BufferDesc;
		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		BufferDesc.ByteWidth = 256;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &ShadowResolveConstantBuffer));

		HANDLE ShadowResolvePixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ShadowResolve.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER ShadowResolvePixelShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(ShadowResolvePixelShaderFile, &ShadowResolvePixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> ShadowResolvePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ShadowResolvePixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(ShadowResolvePixelShaderFile, ShadowResolvePixelShaderByteCodeData, (DWORD)ShadowResolvePixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(ShadowResolvePixelShaderFile);

		SAFE_DX(Device->CreatePixelShader(ShadowResolvePixelShaderByteCodeData, ShadowResolvePixelShaderByteCodeLength.QuadPart, nullptr, &ShadowResolvePixelShader));
	}

	// ===============================================================================================================

	{
		D3D11_TEXTURE2D_DESC TextureDesc;
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

		SAFE_DX(Device->CreateTexture2D(&TextureDesc, nullptr, &HDRSceneColorTexture));

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2DMS;

		SAFE_DX(Device->CreateRenderTargetView(HDRSceneColorTexture, &RTVDesc, &HDRSceneColorTextureRTV));

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2DMS;

		SAFE_DX(Device->CreateShaderResourceView(HDRSceneColorTexture, &SRVDesc, &HDRSceneColorTextureSRV));

		D3D11_BUFFER_DESC BufferDesc;
		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		BufferDesc.ByteWidth = 256;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &DeferredLightingConstantBuffer));

		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		BufferDesc.ByteWidth = 2 * sizeof(uint32_t) * 32 * 18 * 24;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &LightClustersBuffer));

		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.NumElements = 32 * 18 * 24;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFER;

		SAFE_DX(Device->CreateShaderResourceView(LightClustersBuffer, &SRVDesc, &LightClustersBufferSRV));

		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		BufferDesc.ByteWidth = 256 * sizeof(uint16_t) * 32 * 18 * 24;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &LightIndicesBuffer));

		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.NumElements = 256 * 32 * 18 * 24;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFER;

		SAFE_DX(Device->CreateShaderResourceView(LightIndicesBuffer, &SRVDesc, &LightIndicesBufferSRV));

		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		BufferDesc.ByteWidth = 10000 * 2 * 4 * sizeof(float);
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		BufferDesc.StructureByteStride = 2 * 4 * sizeof(float);
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &PointLightsBuffer));

		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.NumElements = 10000;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFER;

		SAFE_DX(Device->CreateShaderResourceView(PointLightsBuffer, &SRVDesc, &PointLightsBufferSRV));

		HANDLE DeferredLightingPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/DeferredLighting.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER DeferredLightingPixelShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(DeferredLightingPixelShaderFile, &DeferredLightingPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> DeferredLightingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(DeferredLightingPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(DeferredLightingPixelShaderFile, DeferredLightingPixelShaderByteCodeData, (DWORD)DeferredLightingPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(DeferredLightingPixelShaderFile);

		SAFE_DX(Device->CreatePixelShader(DeferredLightingPixelShaderByteCodeData, DeferredLightingPixelShaderByteCodeLength.QuadPart, nullptr, &DeferredLightingPixelShader));
	}

	// ===============================================================================================================

	{
		HANDLE FogPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/Fog.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER FogPixelShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(FogPixelShaderFile, &FogPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> FogPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(FogPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(FogPixelShaderFile, FogPixelShaderByteCodeData, (DWORD)FogPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(FogPixelShaderFile);

		SAFE_DX(Device->CreatePixelShader(FogPixelShaderByteCodeData, FogPixelShaderByteCodeLength.QuadPart, nullptr, &FogPixelShader));

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

		D3D11_BUFFER_DESC BufferDesc;
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

		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		BufferDesc.ByteWidth = 256;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &SkyConstantBuffer));

		HANDLE SkyVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SkyVertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER SkyVertexShaderByteCodeLength;
		Result = GetFileSizeEx(SkyVertexShaderFile, &SkyVertexShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> SkyVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SkyVertexShaderByteCodeLength.QuadPart);
		Result = ReadFile(SkyVertexShaderFile, SkyVertexShaderByteCodeData, (DWORD)SkyVertexShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(SkyVertexShaderFile);

		HANDLE SkyPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SkyPixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER SkyPixelShaderByteCodeLength;
		Result = GetFileSizeEx(SkyPixelShaderFile, &SkyPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> SkyPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SkyPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(SkyPixelShaderFile, SkyPixelShaderByteCodeData, (DWORD)SkyPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(SkyPixelShaderFile);

		SAFE_DX(Device->CreateVertexShader(SkyVertexShaderByteCodeData, SkyVertexShaderByteCodeLength.QuadPart, nullptr, &SkyVertexShader));
		SAFE_DX(Device->CreatePixelShader(SkyPixelShaderByteCodeData, SkyPixelShaderByteCodeLength.QuadPart, nullptr, &SkyPixelShader));

		ScopedMemoryBlockArray<Texel> SkyTextureTexels = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<Texel>(2048 * 2048);

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

		D3D11_TEXTURE2D_DESC TextureDesc;
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

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateShaderResourceView(SkyTexture, &SRVDesc, &SkyTextureSRV));

		UINT SunMeshVertexCount = 4;
		UINT SunMeshIndexCount = 6;

		Vertex SunMeshVertices[4] = {

			{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }

		};

		WORD SunMeshIndices[6] = { 0, 1, 2, 2, 1, 3 };

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

		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		BufferDesc.ByteWidth = 256;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

		SAFE_DX(Device->CreateBuffer(&BufferDesc, nullptr, &SunConstantBuffer));

		HANDLE SunVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SunVertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER SunVertexShaderByteCodeLength;
		Result = GetFileSizeEx(SunVertexShaderFile, &SunVertexShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> SunVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SunVertexShaderByteCodeLength.QuadPart);
		Result = ReadFile(SunVertexShaderFile, SunVertexShaderByteCodeData, (DWORD)SunVertexShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(SunVertexShaderFile);

		HANDLE SunPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SunPixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER SunPixelShaderByteCodeLength;
		Result = GetFileSizeEx(SunPixelShaderFile, &SunPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> SunPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SunPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(SunPixelShaderFile, SunPixelShaderByteCodeData, (DWORD)SunPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(SunPixelShaderFile);

		SAFE_DX(Device->CreateVertexShader(SunVertexShaderByteCodeData, SunVertexShaderByteCodeLength.QuadPart, nullptr, &SunVertexShader));
		SAFE_DX(Device->CreatePixelShader(SunPixelShaderByteCodeData, SunPixelShaderByteCodeLength.QuadPart, nullptr, &SunPixelShader));

		ScopedMemoryBlockArray<Texel> SunTextureTexels = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<Texel>(512 * 512);

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
	}

	// ===============================================================================================================

	{
		D3D11_TEXTURE2D_DESC TextureDesc;
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

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateShaderResourceView(ResolvedHDRSceneColorTexture, &SRVDesc, &ResolvedHDRSceneColorTextureSRV));
	}

	// ===============================================================================================================

	{
		D3D11_TEXTURE2D_DESC TextureDesc;
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
			SAFE_DX(Device->CreateUnorderedAccessView(SceneLuminanceTextures[i], &UAVDesc, &SceneLuminanceTexturesUAVs[i]));
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < 4; i++)
		{
			SAFE_DX(Device->CreateShaderResourceView(SceneLuminanceTextures[i], &SRVDesc, &SceneLuminanceTexturesSRVs[i]));
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

		SAFE_DX(Device->CreateUnorderedAccessView(AverageLuminanceTexture, &UAVDesc, &AverageLuminanceTextureUAV));

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

		SAFE_DX(Device->CreateShaderResourceView(AverageLuminanceTexture, &SRVDesc, &AverageLuminanceTextureSRV));

		HANDLE LuminanceCalcComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceCalc.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER LuminanceCalcComputeShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(LuminanceCalcComputeShaderFile, &LuminanceCalcComputeShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> LuminanceCalcComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceCalcComputeShaderByteCodeLength.QuadPart);
		Result = ReadFile(LuminanceCalcComputeShaderFile, LuminanceCalcComputeShaderByteCodeData, (DWORD)LuminanceCalcComputeShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(LuminanceCalcComputeShaderFile);

		HANDLE LuminanceSumComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceSum.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER LuminanceSumComputeShaderByteCodeLength;
		Result = GetFileSizeEx(LuminanceSumComputeShaderFile, &LuminanceSumComputeShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> LuminanceSumComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceSumComputeShaderByteCodeLength.QuadPart);
		Result = ReadFile(LuminanceSumComputeShaderFile, LuminanceSumComputeShaderByteCodeData, (DWORD)LuminanceSumComputeShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(LuminanceSumComputeShaderFile);

		HANDLE LuminanceAvgComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceAvg.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER LuminanceAvgComputeShaderByteCodeLength;
		Result = GetFileSizeEx(LuminanceAvgComputeShaderFile, &LuminanceAvgComputeShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> LuminanceAvgComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceAvgComputeShaderByteCodeLength.QuadPart);
		Result = ReadFile(LuminanceAvgComputeShaderFile, LuminanceAvgComputeShaderByteCodeData, (DWORD)LuminanceAvgComputeShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(LuminanceAvgComputeShaderFile);

		SAFE_DX(Device->CreateComputeShader(LuminanceCalcComputeShaderByteCodeData, LuminanceCalcComputeShaderByteCodeLength.QuadPart, nullptr, &LuminanceCalcComputeShader));
		SAFE_DX(Device->CreateComputeShader(LuminanceSumComputeShaderByteCodeData, LuminanceSumComputeShaderByteCodeLength.QuadPart, nullptr, &LuminanceSumComputeShader));
		SAFE_DX(Device->CreateComputeShader(LuminanceAvgComputeShaderByteCodeData, LuminanceAvgComputeShaderByteCodeLength.QuadPart, nullptr, &LuminanceAvgComputeShader));
	}

	// ===============================================================================================================

	{
		D3D11_TEXTURE2D_DESC TextureDesc;
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

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < 7; i++)
		{
			SAFE_DX(Device->CreateRenderTargetView(BloomTextures[0][i], &RTVDesc, &BloomTexturesRTVs[0][i]));
			SAFE_DX(Device->CreateRenderTargetView(BloomTextures[1][i], &RTVDesc, &BloomTexturesRTVs[1][i]));
			SAFE_DX(Device->CreateRenderTargetView(BloomTextures[2][i], &RTVDesc, &BloomTexturesRTVs[2][i]));
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < 7; i++)
		{
			SAFE_DX(Device->CreateShaderResourceView(BloomTextures[0][i], &SRVDesc, &BloomTexturesSRVs[0][i]));
			SAFE_DX(Device->CreateShaderResourceView(BloomTextures[1][i], &SRVDesc, &BloomTexturesSRVs[1][i]));
			SAFE_DX(Device->CreateShaderResourceView(BloomTextures[2][i], &SRVDesc, &BloomTexturesSRVs[2][i]));
		}

		HANDLE BrightPassPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/BrightPass.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER BrightPassPixelShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(BrightPassPixelShaderFile, &BrightPassPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> BrightPassPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(BrightPassPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(BrightPassPixelShaderFile, BrightPassPixelShaderByteCodeData, (DWORD)BrightPassPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(BrightPassPixelShaderFile);

		HANDLE ImageResamplePixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ImageResample.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER ImageResamplePixelShaderByteCodeLength;
		Result = GetFileSizeEx(ImageResamplePixelShaderFile, &ImageResamplePixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> ImageResamplePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ImageResamplePixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(ImageResamplePixelShaderFile, ImageResamplePixelShaderByteCodeData, (DWORD)ImageResamplePixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(ImageResamplePixelShaderFile);

		HANDLE HorizontalBlurPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/HorizontalBlur.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER HorizontalBlurPixelShaderByteCodeLength;
		Result = GetFileSizeEx(HorizontalBlurPixelShaderFile, &HorizontalBlurPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> HorizontalBlurPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HorizontalBlurPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(HorizontalBlurPixelShaderFile, HorizontalBlurPixelShaderByteCodeData, (DWORD)HorizontalBlurPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(HorizontalBlurPixelShaderFile);

		HANDLE VerticalBlurPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/VerticalBlur.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER VerticalBlurPixelShaderByteCodeLength;
		Result = GetFileSizeEx(VerticalBlurPixelShaderFile, &VerticalBlurPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> VerticalBlurPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(VerticalBlurPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(VerticalBlurPixelShaderFile, VerticalBlurPixelShaderByteCodeData, (DWORD)VerticalBlurPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(VerticalBlurPixelShaderFile);

		SAFE_DX(Device->CreatePixelShader(BrightPassPixelShaderByteCodeData, BrightPassPixelShaderByteCodeLength.QuadPart, nullptr, &BrightPassPixelShader));
		SAFE_DX(Device->CreatePixelShader(ImageResamplePixelShaderByteCodeData, ImageResamplePixelShaderByteCodeLength.QuadPart, nullptr, &ImageResamplePixelShader));
		SAFE_DX(Device->CreatePixelShader(HorizontalBlurPixelShaderByteCodeData, HorizontalBlurPixelShaderByteCodeLength.QuadPart, nullptr, &HorizontalBlurPixelShader));
		SAFE_DX(Device->CreatePixelShader(VerticalBlurPixelShaderByteCodeData, VerticalBlurPixelShaderByteCodeLength.QuadPart, nullptr, &VerticalBlurPixelShader));
	}

	// ===============================================================================================================

	{
		D3D11_TEXTURE2D_DESC TextureDesc;
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

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2DMS;

		SAFE_DX(Device->CreateRenderTargetView(ToneMappedImageTexture, &RTVDesc, &ToneMappedImageRTV));

		HANDLE HDRToneMappingPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/HDRToneMapping.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER HDRToneMappingPixelShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(HDRToneMappingPixelShaderFile, &HDRToneMappingPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> HDRToneMappingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HDRToneMappingPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(HDRToneMappingPixelShaderFile, HDRToneMappingPixelShaderByteCodeData, (DWORD)HDRToneMappingPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(HDRToneMappingPixelShaderFile);

		SAFE_DX(Device->CreatePixelShader(HDRToneMappingPixelShaderByteCodeData, HDRToneMappingPixelShaderByteCodeLength.QuadPart, nullptr, &HDRToneMappingPixelShader));
	}

	// ===============================================================================================================

	HANDLE VertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/MaterialBase_VertexShader_GBufferOpaquePass.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER VertexShaderByteCodeLength;
	BOOL Result = GetFileSizeEx(VertexShaderFile, &VertexShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> VertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(VertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(VertexShaderFile, VertexShaderByteCodeData, (DWORD)VertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(VertexShaderFile);

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

	SAFE_DX(Device->CreateInputLayout(InputElementDescs, 5, VertexShaderByteCodeData, VertexShaderByteCodeLength.QuadPart, &InputLayout));

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

	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.ComparisonFunc = (D3D11_COMPARISON_FUNC)0;
	SamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MipLODBias = 0.0f;

	SAFE_DX(Device->CreateSamplerState(&SamplerDesc, &BiLinearSampler));

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
	XMMATRIX ViewProjMatrix = Engine::GetEngine().GetGameFramework().GetCamera().GetViewProjMatrix();

	vector<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
	vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix, true);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

	vector<PointLightComponent*> AllPointLightComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetPointLightComponents();
	vector<PointLightComponent*> VisblePointLightComponents = cullingSubSystem.GetVisiblePointLightsInFrustum(AllPointLightComponents, ViewProjMatrix);

	XMMATRIX ViewMatrix = Engine::GetEngine().GetGameFramework().GetCamera().GetViewMatrix();
	XMMATRIX ProjMatrix = Engine::GetEngine().GetGameFramework().GetCamera().GetProjMatrix();

	XMFLOAT3 CameraLocation = Engine::GetEngine().GetGameFramework().GetCamera().GetCameraLocation();

	XMMATRIX ShadowViewMatrices[4], ShadowProjMatrices[4], ShadowViewProjMatrices[4];

	ShadowViewMatrices[0] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 10.0f, CameraLocation.y + 10.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[1] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 20.0f, CameraLocation.y + 20.0f, CameraLocation.z - 20.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[2] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 50.0f, CameraLocation.y + 50.0f, CameraLocation.z - 50.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[3] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 100.0f, CameraLocation.y + 100.0f, CameraLocation.z - 100.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	ShadowProjMatrices[0] = XMMatrixOrthographicLH(10.0f, 10.0f, 0.01f, 500.0f);
	ShadowProjMatrices[1] = XMMatrixOrthographicLH(20.0f, 20.0f, 0.01f, 500.0f);
	ShadowProjMatrices[2] = XMMatrixOrthographicLH(50.0f, 50.0f, 0.01f, 500.0f);
	ShadowProjMatrices[3] = XMMatrixOrthographicLH(100.0f, 100.0f, 0.01f, 500.0f);

	ShadowViewProjMatrices[0] = ShadowViewMatrices[0] * ShadowProjMatrices[0];
	ShadowViewProjMatrices[1] = ShadowViewMatrices[1] * ShadowProjMatrices[1];
	ShadowViewProjMatrices[2] = ShadowViewMatrices[2] * ShadowProjMatrices[2];
	ShadowViewProjMatrices[3] = ShadowViewMatrices[3] * ShadowProjMatrices[3];

	clusterizationSubSystem.ClusterizeLights(VisblePointLightComponents, ViewMatrix);

	vector<PointLight> PointLights;

	for (PointLightComponent *pointLightComponent : VisblePointLightComponents)
	{
		PointLight pointLight;
		pointLight.Brightness = pointLightComponent->GetBrightness();
		pointLight.Color = pointLightComponent->GetColor();
		pointLight.Position = pointLightComponent->GetTransformComponent()->GetLocation();
		pointLight.Radius = pointLightComponent->GetRadius();

		PointLights.push_back(pointLight);
	}

	OPTICK_EVENT("Draw Calls")

		DeviceContext->ClearState();

	// ===============================================================================================================

	{
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ID3D11RenderTargetView *GBufferTexturesRTVs[2] = { this->GBufferTexturesRTVs[0], this->GBufferTexturesRTVs[1] };

		DeviceContext->OMSetRenderTargets(2, GBufferTexturesRTVs, DepthBufferTextureDSV);

		D3D11_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		DeviceContext->RSSetViewports(1, &Viewport);

		DeviceContext->IASetInputLayout(InputLayout);
		DeviceContext->RSSetState(RasterizerState);
		DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
		DeviceContext->OMSetDepthStencilState(GBufferPassDepthStencilState, 0);

		DeviceContext->PSSetSamplers(0, 1, &TextureSampler);

		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		DeviceContext->ClearRenderTargetView(GBufferTexturesRTVs[0], ClearColor);
		DeviceContext->ClearRenderTargetView(GBufferTexturesRTVs[1], ClearColor);
		DeviceContext->ClearDepthStencilView(DepthBufferTextureDSV, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL, 0.0f, 0);

		for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
		{
			StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

			RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
			RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
			RenderTexture *renderTexture0 = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
			RenderTexture *renderTexture1 = staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

			ID3D11ShaderResourceView *TexturesSRVs[2] =
			{
				renderTexture0->TextureSRV,
				renderTexture1->TextureSRV
			};

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

			GBufferOpaquePassConstantBufferStruct& ConstantBufferRef = *(GBufferOpaquePassConstantBufferStruct*)MappedSubResource.pData;

			ConstantBufferRef.WVPMatrix = WVPMatrix;
			ConstantBufferRef.WorldMatrix = WorldMatrix;
			ConstantBufferRef.VectorTransformMatrix = VectorTransformMatrix;

			DeviceContext->Unmap(ConstantBuffer, 0);

			UINT Stride = sizeof(Vertex), Offset = 0;

			DeviceContext->IASetVertexBuffers(0, 1, &renderMesh->VertexBuffer, &Stride, &Offset);
			DeviceContext->IASetIndexBuffer(renderMesh->IndexBuffer, DXGI_FORMAT::DXGI_FORMAT_R16_UINT, 0);

			DeviceContext->VSSetShader(renderMaterial->GBufferOpaquePassVertexShader, nullptr, 0);
			DeviceContext->PSSetShader(renderMaterial->GBufferOpaquePassPixelShader, nullptr, 0);

			DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
			DeviceContext->PSSetShaderResources(0, 2, TexturesSRVs);

			DeviceContext->DrawIndexed(8 * 8 * 6 * 6, 0, 0);
		}
	}

	// ===============================================================================================================

	{
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		DeviceContext->OMSetRenderTargets(1, &ResolvedDepthBufferTextureRTV, nullptr);

		D3D11_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		DeviceContext->RSSetViewports(1, &Viewport);

		DeviceContext->IASetInputLayout(nullptr);
		DeviceContext->RSSetState(RasterizerState);
		DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
		DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(MSAADepthResolvePixelShader, nullptr, 0);

		DeviceContext->PSSetShaderResources(0, 1, &DepthBufferTextureSRV);

		DeviceContext->Draw(4, 0);
	}

	// ===============================================================================================================

	{
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		DeviceContext->OMSetRenderTargets(1, &OcclusionBufferTextureRTV, nullptr);

		D3D11_VIEWPORT Viewport;
		Viewport.Height = 144.0f;
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = 256.0f;

		DeviceContext->RSSetViewports(1, &Viewport);

		DeviceContext->PSSetSamplers(0, 1, &MinSampler);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(OcclusionBufferPixelShader, nullptr, 0);

		DeviceContext->PSSetShaderResources(0, 1, &ResolvedDepthBufferTextureSRV);

		DeviceContext->Draw(4, 0);

		DeviceContext->CopyResource(OcclusionBufferStagingTextures[OcclusionBufferIndex], OcclusionBufferTexture);

		OcclusionBufferIndex = (OcclusionBufferIndex + 1) % 3;

		float *OcclusionBufferData = cullingSubSystem.GetOcclusionBufferData();

		D3D11_MAPPED_SUBRESOURCE MappedSubResource;

		SAFE_DX(DeviceContext->Map(OcclusionBufferStagingTextures[OcclusionBufferIndex], 0, D3D11_MAP::D3D11_MAP_READ, 0, &MappedSubResource));

		for (UINT i = 0; i < 144; i++)
		{
			memcpy((BYTE*)OcclusionBufferData + i * 256 * sizeof(float), (BYTE*)MappedSubResource.pData + i * MappedSubResource.RowPitch, 256 * sizeof(float));
		}

		DeviceContext->Unmap(OcclusionBufferStagingTextures[OcclusionBufferIndex], 0);
	}

	// ===============================================================================================================

	{
		for (int i = 0; i < 4; i++)
		{
			SIZE_T ConstantBufferOffset = 0;

			vector<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
			vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ShadowViewProjMatrices[i], false);
			size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

			DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			DeviceContext->OMSetRenderTargets(0, nullptr, CascadedShadowMapTexturesDSVs[i]);

			D3D11_VIEWPORT Viewport;
			Viewport.Height = 2048.0f;
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = 2048.0f;

			DeviceContext->RSSetViewports(1, &Viewport);

			DeviceContext->IASetInputLayout(InputLayout);
			DeviceContext->RSSetState(RasterizerState);
			DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
			DeviceContext->OMSetDepthStencilState(ShadowMapPassDepthStencilState, 0);

			DeviceContext->ClearDepthStencilView(CascadedShadowMapTexturesDSVs[i], D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, 1.0f, 0);

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

				ShadowMapPassConstantBufferStruct& ConstantBufferRef = *(ShadowMapPassConstantBufferStruct*)MappedSubResource.pData;

				ConstantBufferRef.WVPMatrix = WVPMatrix;

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
	}

	// ===============================================================================================================

	{
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		DeviceContext->OMSetRenderTargets(1, &ShadowMaskTextureRTV, nullptr);

		D3D11_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		DeviceContext->RSSetViewports(1, &Viewport);

		DeviceContext->IASetInputLayout(nullptr);
		DeviceContext->RSSetState(RasterizerState);
		DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
		DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

		XMMATRIX ReProjMatrices[4];
		ReProjMatrices[0] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[0];
		ReProjMatrices[1] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[1];
		ReProjMatrices[2] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[2];
		ReProjMatrices[3] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[3];

		D3D11_MAPPED_SUBRESOURCE MappedSubResource;

		SAFE_DX(DeviceContext->Map(ShadowResolveConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource));

		ShadowResolveConstantBufferStruct& ConstantBufferRef = *(ShadowResolveConstantBufferStruct*)MappedSubResource.pData;

		ConstantBufferRef.ReProjMatrices[0] = ReProjMatrices[0];
		ConstantBufferRef.ReProjMatrices[1] = ReProjMatrices[1];
		ConstantBufferRef.ReProjMatrices[2] = ReProjMatrices[2];
		ConstantBufferRef.ReProjMatrices[3] = ReProjMatrices[3];

		DeviceContext->Unmap(ShadowResolveConstantBuffer, 0);

		DeviceContext->PSSetSamplers(0, 1, &ShadowMapSampler);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(ShadowResolvePixelShader, nullptr, 0);

		ID3D11ShaderResourceView *SRVs[5] =
		{
			ResolvedDepthBufferTextureSRV,
			CascadedShadowMapTexturesSRVs[0],
			CascadedShadowMapTexturesSRVs[1],
			CascadedShadowMapTexturesSRVs[2],
			CascadedShadowMapTexturesSRVs[3]
		};

		DeviceContext->PSSetConstantBuffers(0, 1, &ShadowResolveConstantBuffer);
		DeviceContext->PSSetShaderResources(0, 5, SRVs);

		DeviceContext->Draw(4, 0);
	}

	// ===============================================================================================================

	{

		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		DeviceContext->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, nullptr);

		D3D11_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		DeviceContext->RSSetViewports(1, &Viewport);

		DeviceContext->IASetInputLayout(nullptr);
		DeviceContext->RSSetState(RasterizerState);
		DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
		DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

		XMMATRIX InvViewProjMatrix = XMMatrixInverse(nullptr, ViewProjMatrix);

		D3D11_MAPPED_SUBRESOURCE MappedSubResource;
		SAFE_DX(DeviceContext->Map(DeferredLightingConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource));

		DeferredLightingConstantBufferStruct& ConstantBufferRef = *(DeferredLightingConstantBufferStruct*)MappedSubResource.pData;

		ConstantBufferRef.InvViewProjMatrix = InvViewProjMatrix;
		ConstantBufferRef.CameraWorldPosition = CameraLocation;

		DeviceContext->Unmap(DeferredLightingConstantBuffer, 0);

		DeviceContext->Map(LightClustersBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource);

		memcpy(MappedSubResource.pData, clusterizationSubSystem.GetLightClustersData(), 32 * 18 * 24 * 2 * sizeof(uint32_t));

		DeviceContext->Unmap(LightClustersBuffer, 0);

		DeviceContext->Map(LightIndicesBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource);

		memcpy(MappedSubResource.pData, clusterizationSubSystem.GetLightIndicesData(), clusterizationSubSystem.GetTotalIndexCount() * sizeof(uint16_t));

		DeviceContext->Unmap(LightIndicesBuffer, 0);

		DeviceContext->Map(PointLightsBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource);

		memcpy(MappedSubResource.pData, PointLights.data(), PointLights.size() * sizeof(PointLight));

		DeviceContext->Unmap(PointLightsBuffer, 0);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(DeferredLightingPixelShader, nullptr, 0);

		ID3D11ShaderResourceView *SRVs[7] =
		{
			GBufferTexturesSRVs[0],
			GBufferTexturesSRVs[1],
			DepthBufferTextureSRV,
			ShadowMaskTextureSRV,
			LightClustersBufferSRV,
			LightIndicesBufferSRV,
			PointLightsBufferSRV
		};

		DeviceContext->PSSetConstantBuffers(0, 1, &DeferredLightingConstantBuffer);
		DeviceContext->PSSetShaderResources(0, 7, SRVs);

		DeviceContext->Draw(4, 0);
	}

	// ===============================================================================================================

	{
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		DeviceContext->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, nullptr);

		D3D11_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		DeviceContext->RSSetViewports(1, &Viewport);

		DeviceContext->IASetInputLayout(nullptr);
		DeviceContext->RSSetState(RasterizerState);
		DeviceContext->OMSetBlendState(FogBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
		DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(FogPixelShader, nullptr, 0);

		DeviceContext->PSSetShaderResources(0, 1, &DepthBufferTextureSRV);

		DeviceContext->Draw(4, 0);

		ID3D11ShaderResourceView *NullSRVs[3] = { nullptr, nullptr, nullptr };

		DeviceContext->PSSetShaderResources(0, 3, NullSRVs);

		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		DeviceContext->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, DepthBufferTextureDSV);

		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		DeviceContext->RSSetViewports(1, &Viewport);

		DeviceContext->IASetInputLayout(InputLayout);
		DeviceContext->RSSetState(RasterizerState);
		DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
		DeviceContext->OMSetDepthStencilState(SkyAndSunDepthStencilState, 0);

		XMMATRIX SkyWorldMatrix = XMMatrixScaling(900.0f, 900.0f, 900.0f) * XMMatrixTranslation(CameraLocation.x, CameraLocation.y, CameraLocation.z);
		XMMATRIX SkyWVPMatrix = SkyWorldMatrix * ViewProjMatrix;

		D3D11_MAPPED_SUBRESOURCE MappedSubResource;
		SAFE_DX(DeviceContext->Map(SkyConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource));

		SkyConstantBufferStruct& SkyConstantBufferRef = *(SkyConstantBufferStruct*)MappedSubResource.pData;

		SkyConstantBufferRef.WVPMatrix = SkyWVPMatrix;

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

		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		DeviceContext->IASetInputLayout(InputLayout);
		DeviceContext->RSSetState(RasterizerState);
		DeviceContext->OMSetBlendState(SunBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
		DeviceContext->OMSetDepthStencilState(SkyAndSunDepthStencilState, 0);

		XMFLOAT3 SunPosition(-500.0f + CameraLocation.x, 500.0f + CameraLocation.y, -500.f + CameraLocation.z);

		SAFE_DX(DeviceContext->Map(SunConstantBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource));

		SunConstantBufferStruct& SunConstantBufferRef = *(SunConstantBufferStruct*)MappedSubResource.pData;

		SunConstantBufferRef.ViewMatrix = ViewMatrix;
		SunConstantBufferRef.ProjMatrix = ProjMatrix;
		SunConstantBufferRef.SunPosition = SunPosition;

		DeviceContext->Unmap(SunConstantBuffer, 0);

		DeviceContext->IASetVertexBuffers(0, 1, &SunVertexBuffer, &Stride, &Offset);
		DeviceContext->IASetIndexBuffer(SunIndexBuffer, DXGI_FORMAT::DXGI_FORMAT_R16_UINT, 0);

		DeviceContext->VSSetShader(SunVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(SunPixelShader, nullptr, 0);

		DeviceContext->VSSetConstantBuffers(0, 1, &SunConstantBuffer);
		DeviceContext->PSSetShaderResources(0, 1, &SunTextureSRV);

		DeviceContext->DrawIndexed(6, 0, 0);
	}

	// ===============================================================================================================

	{
		DeviceContext->ResolveSubresource(ResolvedHDRSceneColorTexture, 0, HDRSceneColorTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

	// ===============================================================================================================

	{
		DeviceContext->CSSetShader(LuminanceCalcComputeShader, nullptr, 0);
		DeviceContext->CSSetUnorderedAccessViews(0, 1, &SceneLuminanceTexturesUAVs[0], nullptr);
		DeviceContext->CSSetShaderResources(0, 1, &ResolvedHDRSceneColorTextureSRV);

		DeviceContext->Dispatch(80, 45, 1);

		DeviceContext->CSSetShader(LuminanceSumComputeShader, nullptr, 0);

		DeviceContext->CSSetUnorderedAccessViews(0, 1, &SceneLuminanceTexturesUAVs[1], nullptr);
		DeviceContext->CSSetShaderResources(0, 1, &SceneLuminanceTexturesSRVs[0]);

		DeviceContext->Dispatch(80, 45, 1);

		DeviceContext->CSSetUnorderedAccessViews(0, 1, &SceneLuminanceTexturesUAVs[2], nullptr);
		DeviceContext->CSSetShaderResources(0, 1, &SceneLuminanceTexturesSRVs[1]);

		DeviceContext->Dispatch(5, 3, 1);

		DeviceContext->CSSetUnorderedAccessViews(0, 1, &SceneLuminanceTexturesUAVs[3], nullptr);
		DeviceContext->CSSetShaderResources(0, 1, &SceneLuminanceTexturesSRVs[2]);

		DeviceContext->Dispatch(1, 1, 1);

		DeviceContext->CSSetShader(LuminanceAvgComputeShader, nullptr, 0);

		DeviceContext->CSSetUnorderedAccessViews(0, 1, &AverageLuminanceTextureUAV, nullptr);
		DeviceContext->CSSetShaderResources(0, 1, &SceneLuminanceTexturesSRVs[3]);

		DeviceContext->Dispatch(1, 1, 1);
	}

	// ===============================================================================================================

	{
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		DeviceContext->OMSetRenderTargets(1, &BloomTexturesRTVs[0][0], nullptr);

		D3D11_VIEWPORT Viewport;
		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		DeviceContext->RSSetViewports(1, &Viewport);

		DeviceContext->IASetInputLayout(nullptr);
		DeviceContext->RSSetState(RasterizerState);
		DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
		DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(BrightPassPixelShader, nullptr, 0);

		ID3D11ShaderResourceView *SRVs[2] =
		{
			ResolvedHDRSceneColorTextureSRV,
			SceneLuminanceTexturesSRVs[0]
		};

		DeviceContext->PSSetShaderResources(0, 2, SRVs);

		DeviceContext->Draw(4, 0);

		DeviceContext->OMSetRenderTargets(1, &BloomTexturesRTVs[1][0], nullptr);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(HorizontalBlurPixelShader, nullptr, 0);

		DeviceContext->PSSetShaderResources(0, 1, &BloomTexturesSRVs[0][0]);

		DeviceContext->Draw(4, 0);

		DeviceContext->OMSetRenderTargets(1, &BloomTexturesRTVs[2][0], nullptr);

		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(VerticalBlurPixelShader, nullptr, 0);

		DeviceContext->PSSetShaderResources(0, 1, &BloomTexturesSRVs[1][0]);

		DeviceContext->Draw(4, 0);

		for (int i = 1; i < 7; i++)
		{
			DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			DeviceContext->OMSetRenderTargets(1, &BloomTexturesRTVs[0][i], nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			DeviceContext->RSSetViewports(1, &Viewport);

			DeviceContext->IASetInputLayout(nullptr);
			DeviceContext->RSSetState(RasterizerState);
			DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
			DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

			DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
			DeviceContext->PSSetShader(ImageResamplePixelShader, nullptr, 0);

			DeviceContext->PSSetShaderResources(0, 1, &BloomTexturesSRVs[0][i - 1]);

			DeviceContext->Draw(4, 0);

			DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			DeviceContext->OMSetRenderTargets(1, &BloomTexturesRTVs[1][i], nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			DeviceContext->RSSetViewports(1, &Viewport);

			DeviceContext->IASetInputLayout(nullptr);
			DeviceContext->RSSetState(RasterizerState);
			DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
			DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

			DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
			DeviceContext->PSSetShader(HorizontalBlurPixelShader, nullptr, 0);

			DeviceContext->PSSetShaderResources(0, 1, &BloomTexturesSRVs[0][i]);

			DeviceContext->Draw(4, 0);

			DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			DeviceContext->OMSetRenderTargets(1, &BloomTexturesRTVs[2][i], nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			DeviceContext->RSSetViewports(1, &Viewport);

			DeviceContext->IASetInputLayout(nullptr);
			DeviceContext->RSSetState(RasterizerState);
			DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
			DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

			DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
			DeviceContext->PSSetShader(VerticalBlurPixelShader, nullptr, 0);

			DeviceContext->PSSetShaderResources(0, 1, &BloomTexturesSRVs[1][i]);

			DeviceContext->Draw(4, 0);
		}

		for (int i = 5; i >= 0; i--)
		{
			DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			DeviceContext->OMSetRenderTargets(1, &BloomTexturesRTVs[2][i], nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			DeviceContext->RSSetViewports(1, &Viewport);

			DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
			DeviceContext->PSSetShader(ImageResamplePixelShader, nullptr, 0);

			DeviceContext->IASetInputLayout(nullptr);
			DeviceContext->RSSetState(RasterizerState);
			DeviceContext->OMSetBlendState(AdditiveBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
			DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

			DeviceContext->PSSetShaderResources(0, 1, &BloomTexturesSRVs[2][i + 1]);

			DeviceContext->Draw(4, 0);
		}
	}

	// ===============================================================================================================

	{
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		DeviceContext->OMSetRenderTargets(1, &ToneMappedImageRTV, nullptr);

		D3D11_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		DeviceContext->RSSetViewports(1, &Viewport);


		DeviceContext->VSSetShader(FullScreenQuadVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(HDRToneMappingPixelShader, nullptr, 0);

		DeviceContext->IASetInputLayout(nullptr);
		DeviceContext->RSSetState(RasterizerState);
		DeviceContext->OMSetBlendState(BlendDisabledBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
		DeviceContext->OMSetDepthStencilState(DepthDisabledDepthStencilState, 0);

		ID3D11ShaderResourceView *SRVs[2] =
		{
			HDRSceneColorTextureSRV,
			BloomTexturesSRVs[2][0]
		};

		DeviceContext->PSSetShaderResources(0, 2, SRVs);

		DeviceContext->Draw(4, 0);
	}

	// ===============================================================================================================

	{
		DeviceContext->ResolveSubresource(BackBufferTexture, 0, ToneMappedImageTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	}

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