#pragma once

#include "../RenderDevice.h"
#include "../CullingSubSystem.h"

#include <Containers/COMRCPtr.h>

struct RenderMeshDX11 : public RenderMesh
{
	COMRCPtr<ID3D11Buffer> VertexBuffer, IndexBuffer;
};

struct RenderTextureDX11 : public RenderTexture
{
	COMRCPtr<ID3D11Texture2D> Texture;
	COMRCPtr<ID3D11ShaderResourceView> TextureSRV;
};

struct RenderMaterialDX11 : public RenderMaterial
{
	COMRCPtr<ID3D11VertexShader> GBufferOpaquePassVertexShader;
	COMRCPtr<ID3D11PixelShader> GBufferOpaquePassPixelShader;
	COMRCPtr<ID3D11VertexShader> ShadowMapPassVertexShader;
	COMRCPtr<ID3D11PixelShader> ShadowMapPassPixelShader;
};

class RenderDeviceDX11 : public RenderDevice
{
	public:

		virtual void InitDevice() override;
		virtual void ShutdownDevice() override;
		virtual void TickDevice(float DeltaTime) override;

		virtual RenderMesh* CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo) override;
		virtual RenderTexture* CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo) override;
		virtual RenderMaterial* CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo) override;

		virtual void DestroyRenderMesh(RenderMesh* renderMesh) override;
		virtual void DestroyRenderTexture(RenderTexture* renderTexture) override;
		virtual void DestroyRenderMaterial(RenderMaterial* renderMaterial) override;

		virtual DirectXVersion GetDirectXVersion() override { return DirectXVersion::DirectX11; }

	private:

		COMRCPtr<ID3D11Device1> Device;
		COMRCPtr<IDXGISwapChain4> SwapChain;

		COMRCPtr<ID3D11DeviceContext1> DeviceContext;

		COMRCPtr<ID3D11Texture2D> BackBufferTexture;
		COMRCPtr<ID3D11RenderTargetView> BackBufferTextureRTV;

		COMRCPtr<ID3D11VertexShader> FullScreenQuadVertexShader;

		// ===============================================================================================================

		COMRCPtr<ID3D11Texture2D> GBufferTextures[2];
		COMRCPtr<ID3D11RenderTargetView>  GBufferTexturesRTVs[2];
		COMRCPtr<ID3D11ShaderResourceView> GBufferTexturesSRVs[2];

		COMRCPtr<ID3D11Texture2D> DepthBufferTexture;
		COMRCPtr<ID3D11DepthStencilView> DepthBufferTextureDSV;
		COMRCPtr<ID3D11ShaderResourceView> DepthBufferTextureSRV;

		COMRCPtr<ID3D11Buffer> ConstantBuffer;

		// ===============================================================================================================

		COMRCPtr<ID3D11Texture2D> ResolvedDepthBufferTexture;
		COMRCPtr<ID3D11RenderTargetView> ResolvedDepthBufferTextureRTV;
		COMRCPtr<ID3D11ShaderResourceView> ResolvedDepthBufferTextureSRV;

		COMRCPtr<ID3D11PixelShader> MSAADepthResolvePixelShader;

		// ===============================================================================================================

		COMRCPtr<ID3D11Texture2D> OcclusionBufferTexture, OcclusionBufferStagingTextures[3];
		COMRCPtr<ID3D11RenderTargetView> OcclusionBufferTextureRTV;

		COMRCPtr<ID3D11PixelShader> OcclusionBufferPixelShader;

		// ===============================================================================================================

		COMRCPtr<ID3D11Texture2D> CascadedShadowMapTextures[4];
		COMRCPtr<ID3D11DepthStencilView> CascadedShadowMapTexturesDSVs[4];
		COMRCPtr<ID3D11ShaderResourceView> CascadedShadowMapTexturesSRVs[4];

		COMRCPtr<ID3D11Buffer> ConstantBuffers[4];

		// ===============================================================================================================

		COMRCPtr<ID3D11Texture2D> ShadowMaskTexture;
		COMRCPtr<ID3D11RenderTargetView> ShadowMaskTextureRTV;
		COMRCPtr<ID3D11ShaderResourceView> ShadowMaskTextureSRV;

		COMRCPtr<ID3D11Buffer> ShadowResolveConstantBuffer;

		COMRCPtr<ID3D11PixelShader> ShadowResolvePixelShader;

		// ===============================================================================================================

		COMRCPtr<ID3D11Texture2D> HDRSceneColorTexture;
		COMRCPtr<ID3D11RenderTargetView> HDRSceneColorTextureRTV;
		COMRCPtr<ID3D11ShaderResourceView> HDRSceneColorTextureSRV;

		COMRCPtr<ID3D11Buffer> DeferredLightingConstantBuffer;

		COMRCPtr<ID3D11PixelShader> DeferredLightingPixelShader;

		COMRCPtr<ID3D11Buffer> LightClustersBuffer;
		COMRCPtr<ID3D11ShaderResourceView> LightClustersBufferSRV;

		COMRCPtr<ID3D11Buffer> LightIndicesBuffer;
		COMRCPtr<ID3D11ShaderResourceView> LightIndicesBufferSRV;

		COMRCPtr<ID3D11Buffer> PointLightsBuffer;
		COMRCPtr<ID3D11ShaderResourceView> PointLightsBufferSRV;

		// ===============================================================================================================

		COMRCPtr<ID3D11PixelShader> FogPixelShader;

		COMRCPtr<ID3D11Buffer> SkyVertexBuffer, SkyIndexBuffer;
		COMRCPtr<ID3D11Buffer> SkyConstantBuffer;
		COMRCPtr<ID3D11VertexShader> SkyVertexShader;
		COMRCPtr<ID3D11PixelShader> SkyPixelShader;
		COMRCPtr<ID3D11Texture2D> SkyTexture;
		COMRCPtr<ID3D11ShaderResourceView> SkyTextureSRV;

		COMRCPtr<ID3D11Buffer> SunVertexBuffer, SunIndexBuffer;
		COMRCPtr<ID3D11Buffer> SunConstantBuffer;
		COMRCPtr<ID3D11VertexShader> SunVertexShader;
		COMRCPtr<ID3D11PixelShader> SunPixelShader;
		COMRCPtr<ID3D11Texture2D> SunTexture;
		COMRCPtr<ID3D11ShaderResourceView> SunTextureSRV;

		// ===============================================================================================================

		COMRCPtr<ID3D11Texture2D> ResolvedHDRSceneColorTexture;
		COMRCPtr<ID3D11ShaderResourceView> ResolvedHDRSceneColorTextureSRV;

		// ===============================================================================================================

		COMRCPtr<ID3D11Texture2D> SceneLuminanceTextures[4];
		COMRCPtr<ID3D11UnorderedAccessView> SceneLuminanceTexturesUAVs[4];
		COMRCPtr<ID3D11ShaderResourceView> SceneLuminanceTexturesSRVs[4];

		COMRCPtr<ID3D11Texture2D> AverageLuminanceTexture;
		COMRCPtr<ID3D11UnorderedAccessView> AverageLuminanceTextureUAV;
		COMRCPtr<ID3D11ShaderResourceView> AverageLuminanceTextureSRV;

		COMRCPtr<ID3D11ComputeShader> LuminanceCalcComputeShader;
		COMRCPtr<ID3D11ComputeShader> LuminanceSumComputeShader;
		COMRCPtr<ID3D11ComputeShader> LuminanceAvgComputeShader;

		// ===============================================================================================================

		COMRCPtr<ID3D11Texture2D> BloomTextures[3][7];
		COMRCPtr<ID3D11RenderTargetView> BloomTexturesRTVs[3][7];
		COMRCPtr<ID3D11ShaderResourceView> BloomTexturesSRVs[3][7];

		COMRCPtr<ID3D11PixelShader> BrightPassPixelShader;
		COMRCPtr<ID3D11PixelShader> ImageResamplePixelShader;
		COMRCPtr<ID3D11PixelShader> HorizontalBlurPixelShader;
		COMRCPtr<ID3D11PixelShader> VerticalBlurPixelShader;

		// ===============================================================================================================

		COMRCPtr<ID3D11Texture2D> ToneMappedImageTexture;
		COMRCPtr<ID3D11RenderTargetView> ToneMappedImageRTV;

		COMRCPtr<ID3D11PixelShader> HDRToneMappingPixelShader;

		// ===============================================================================================================

		COMRCPtr<ID3D11SamplerState> TextureSampler, ShadowMapSampler, BiLinearSampler, MinSampler;

		COMRCPtr<ID3D11InputLayout> InputLayout;
		COMRCPtr<ID3D11RasterizerState> RasterizerState;
		COMRCPtr<ID3D11BlendState> BlendDisabledBlendState, FogBlendState, SunBlendState, AdditiveBlendState;
		COMRCPtr<ID3D11DepthStencilState> GBufferPassDepthStencilState, ShadowMapPassDepthStencilState, SkyAndSunDepthStencilState, DepthDisabledDepthStencilState;

		vector<RenderMesh*> RenderMeshDestructionQueue;
		vector<RenderMaterial*> RenderMaterialDestructionQueue;
		vector<RenderTexture*> RenderTextureDestructionQueue;

		inline void CheckDXCallResult(HRESULT hr, const char16_t* Function);
		inline const char16_t* GetDXErrorMessageFromHRESULT(HRESULT hr);

		UINT OcclusionBufferIndex = 0;
};