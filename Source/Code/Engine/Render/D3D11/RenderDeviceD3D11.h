#pragma once

#include "../RenderDevice.h"

#include "../CullingSubSystem.h"
#include "../ClusterizationSubSystem.h"

#include <Containers/COMRCPtr.h>

struct RenderMeshD3D11 : public RenderMesh
{
	COMRCPtr<ID3D11Buffer> VertexBuffer, IndexBuffer;
};

struct RenderTextureD3D11 : public RenderTexture
{
	COMRCPtr<ID3D11Texture2D> Texture;
	COMRCPtr<ID3D11ShaderResourceView> TextureSRV;
};

struct RenderMaterialD3D11 : public RenderMaterial
{
	COMRCPtr<ID3D11VertexShader> GBufferOpaquePassVertexShader;
	COMRCPtr<ID3D11PixelShader> GBufferOpaquePassPixelShader;
	COMRCPtr<ID3D11VertexShader> ShadowMapPassVertexShader;
	COMRCPtr<ID3D11PixelShader> ShadowMapPassPixelShader;
};

/*enum class BlockCompression { BC1, BC2, BC3, BC4, BC5 };

struct RenderMeshCreateInfo
{
	void *VertexData;
	void *IndexData;
	UINT VertexCount;
	UINT IndexCount;
};

struct RenderTextureCreateInfo
{
	UINT Width, Height;
	UINT MIPLevels;
	BOOL SRGB;
	BOOL Compressed;
	BlockCompression CompressionType;
	BYTE *TexelData;
};

struct RenderMaterialCreateInfo
{
	void *GBufferOpaquePassVertexShaderByteCodeData;
	void *GBufferOpaquePassPixelShaderByteCodeData;
	size_t GBufferOpaquePassVertexShaderByteCodeLength;
	size_t GBufferOpaquePassPixelShaderByteCodeLength;
	void *ShadowMapPassVertexShaderByteCodeData;
	void *ShadowMapPassPixelShaderByteCodeData;
	size_t ShadowMapPassVertexShaderByteCodeLength;
	size_t ShadowMapPassPixelShaderByteCodeLength;
};

struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT2 TexCoord;
	XMFLOAT3 Normal;
	XMFLOAT3 Tangent;
	XMFLOAT3 Binormal;
};

struct Texel
{
	BYTE R, G, B, A;
};

struct CompressedTexelBlockBC1
{
	uint16_t Colors[2];
	uint8_t Texels[4];
};

struct CompressedTexelBlockBC5
{
	uint8_t Red[2];
	uint8_t RedIndices[6];
	uint8_t Green[2];
	uint8_t GreenIndices[6];
};

struct Color
{
	float R, G, B;
};

inline Color operator+(const Color& Color1, const Color& Color2)
{
	Color Result;

	Result.R = Color1.R + Color2.R;
	Result.G = Color1.G + Color2.G;
	Result.B = Color1.B + Color2.B;

	return Result;
}

inline Color operator*(const float Scalar, const Color& color)
{
	Color Result;

	Result.R = Scalar * color.R;
	Result.G = Scalar * color.G;
	Result.B = Scalar * color.B;

	return Result;
}

inline Color operator*(const Color& color, const float Scalar)
{
	return operator*(Scalar, color);
}

inline Color operator/(const Color& color, const float Scalar)
{
	Color Result;

	Result.R = color.R / Scalar;
	Result.G = color.G / Scalar;
	Result.B = color.B / Scalar;

	return Result;
}

inline float DistanceBetweenColor(const Color& Color1, const Color& Color2)
{
	return powf(Color1.R - Color2.R, 2.0f) + powf(Color1.G - Color2.G, 2.0f) + powf(Color1.B - Color2.B, 2.0f);
}*/

#define SAFE_DX(Func) CheckDXCallResult(Func, u#Func);
#define UUIDOF(Value) __uuidof(Value), (void**)&Value

class RenderDeviceD3D11 : public RenderDevice
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

		//CullingSubSystem& GetCullingSubSystem() { return cullingSubSystem; }

	private:

		COMRCPtr<ID3D11Device1> Device;
		COMRCPtr<IDXGISwapChain4> SwapChain;

		int ResolutionWidth;
		int ResolutionHeight;

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

		COMRCPtr<ID3D11InputLayout> InputLayout, SkyAndSunInputLayout;
		COMRCPtr<ID3D11RasterizerState> RasterizerState;
		COMRCPtr<ID3D11BlendState> BlendDisabledBlendState, FogBlendState, SunBlendState, AdditiveBlendState;
		COMRCPtr<ID3D11DepthStencilState> GBufferPassDepthStencilState, ShadowMapPassDepthStencilState, SkyAndSunDepthStencilState, DepthDisabledDepthStencilState;

		DynamicArray<RenderMesh*> RenderMeshDestructionQueue;
		DynamicArray<RenderMaterial*> RenderMaterialDestructionQueue;
		DynamicArray<RenderTexture*> RenderTextureDestructionQueue;

		/*CullingSubSystem cullingSubSystem;
		ClusterizationSubSystem clusterizationSubSystem;*/

		inline void CheckDXCallResult(HRESULT hr, const char16_t* Function);
		inline const char16_t* GetDXErrorMessageFromHRESULT(HRESULT hr);

		static const UINT MAX_MIP_LEVELS_IN_TEXTURE = 16;

		UINT OcclusionBufferIndex = 0;
};