#pragma once

#include <Containers/COMRCPtr.h>

#include "CullingSubSystem.h"

struct RenderMesh
{
	COMRCPtr<ID3D11Buffer> VertexBuffer, IndexBuffer;
};

struct RenderTexture
{
	COMRCPtr<ID3D11Texture2D> Texture;
	COMRCPtr<ID3D11ShaderResourceView> TextureSRV;
};

struct RenderMaterial
{
	COMRCPtr<ID3D11VertexShader> GBufferOpaquePassVertexShader;
	COMRCPtr<ID3D11PixelShader> GBufferOpaquePassPixelShader;
	COMRCPtr<ID3D11VertexShader> ShadowMapPassVertexShader;
	COMRCPtr<ID3D11PixelShader> ShadowMapPassPixelShader;
};

enum class BlockCompression { BC1, BC2, BC3, BC4, BC5 };

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
}

#define SAFE_DX(Func) CheckDXCallResult(Func, u#Func);
#define UUIDOF(Value) __uuidof(Value), (void**)&Value

class RenderSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();
		void TickSystem(float DeltaTime);

		RenderMesh* CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo);
		RenderTexture* CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo);
		RenderMaterial* CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo);

		void DestroyRenderMesh(RenderMesh* renderMesh);
		void DestroyRenderTexture(RenderTexture* renderTexture);
		void DestroyRenderMaterial(RenderMaterial* renderMaterial);

	private:

		COMRCPtr<ID3D11Device> Device;
		COMRCPtr<IDXGISwapChain4> SwapChain;

		int ResolutionWidth;
		int ResolutionHeight;

		COMRCPtr<ID3D11DeviceContext> DeviceContext;

		COMRCPtr<ID3D11Texture2D> BackBufferTexture;
		COMRCPtr<ID3D11RenderTargetView> BackBufferRTV;

		COMRCPtr<ID3D11Texture2D> DepthBufferTexture;
		COMRCPtr<ID3D11DepthStencilView> DepthBufferDSV;
		COMRCPtr<ID3D11ShaderResourceView> DepthBufferSRV;

		COMRCPtr<ID3D11Buffer> ConstantBuffer, ConstantBuffers[4];

		COMRCPtr<ID3D11Texture2D> GBufferTextures[2];
		COMRCPtr<ID3D11RenderTargetView>  GBufferRTVs[2];
		COMRCPtr<ID3D11ShaderResourceView> GBufferSRVs[2];

		COMRCPtr<ID3D11Texture2D> ResolvedDepthBufferTexture;
		COMRCPtr<ID3D11ShaderResourceView> ResolvedDepthBufferSRV;

		COMRCPtr<ID3D11Texture2D> CascadedShadowMapTextures[4];
		COMRCPtr<ID3D11DepthStencilView> CascadedShadowMapDSVs[4];
		COMRCPtr<ID3D11ShaderResourceView> CascadedShadowMapSRVs[4];

		COMRCPtr<ID3D11Texture2D> ShadowMaskTexture;
		COMRCPtr<ID3D11RenderTargetView> ShadowMaskRTV;
		COMRCPtr<ID3D11ShaderResourceView> ShadowMaskSRV;

		COMRCPtr<ID3D11Texture2D> LBufferTexture;
		COMRCPtr<ID3D11RenderTargetView> LBufferRTV;
		COMRCPtr<ID3D11ShaderResourceView> LBufferSRV;

		COMRCPtr<ID3D11Texture2D> ResolvedHDRSceneColorTexture;
		COMRCPtr<ID3D11ShaderResourceView> ResolvedHDRSceneColorSRV;

		COMRCPtr<ID3D11Texture2D> SceneLuminanceTextures[4];
		COMRCPtr<ID3D11UnorderedAccessView> SceneLuminanceUAVs[4];
		COMRCPtr<ID3D11ShaderResourceView> SceneLuminanceSRVs[4];

		COMRCPtr<ID3D11Texture2D> AverageLuminanceTexture;
		COMRCPtr<ID3D11UnorderedAccessView> AverageLuminanceUAV;
		COMRCPtr<ID3D11ShaderResourceView> AverageLuminanceSRV;

		COMRCPtr<ID3D11Texture2D> BloomTextures[3][7];
		COMRCPtr<ID3D11RenderTargetView> BloomRTVs[3][7];
		COMRCPtr<ID3D11ShaderResourceView> BloomSRVs[3][7];

		COMRCPtr<ID3D11Texture2D> ToneMappedImageTexture;
		COMRCPtr<ID3D11RenderTargetView> ToneMappedImageRTV;

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

		COMRCPtr<ID3D11Buffer> ShadowResolveConstantBuffer;

		COMRCPtr<ID3D11Buffer> DeferredLightingConstantBuffer;

		COMRCPtr<ID3D11VertexShader> FullScreenQuadVertexShader;

		COMRCPtr<ID3D11PixelShader> ShadowResolvePixelShader;
		COMRCPtr<ID3D11PixelShader> DeferredLightingPixelShader;
		COMRCPtr<ID3D11PixelShader> FogPixelShader;
		COMRCPtr<ID3D11PixelShader> HDRToneMappingPixelShader;
		COMRCPtr<ID3D11ComputeShader> LuminanceCalcComputeShader;
		COMRCPtr<ID3D11ComputeShader> LuminanceSumComputeShader;
		COMRCPtr<ID3D11ComputeShader> LuminanceAvgComputeShader;
		COMRCPtr<ID3D11PixelShader> BrightPassPixelShader;
		COMRCPtr<ID3D11PixelShader> ImageResamplePixelShader;
		COMRCPtr<ID3D11PixelShader> HorizontalBlurPixelShader;
		COMRCPtr<ID3D11PixelShader> VerticalBlurPixelShader;

		COMRCPtr<ID3D11SamplerState> TextureSampler, ShadowMapSampler, BiLinearSampler;

		COMRCPtr<ID3D11InputLayout> InputLayout;
		COMRCPtr<ID3D11RasterizerState> RasterizerState;
		COMRCPtr<ID3D11BlendState> BlendDisabledBlendState, FogBlendState, SunBlendState, AdditiveBlendState;
		COMRCPtr<ID3D11DepthStencilState> GBufferPassDepthStencilState, ShadowMapPassDepthStencilState, SkyAndSunDepthStencilState, DepthDisabledDepthStencilState;

		vector<RenderMesh*> RenderMeshDestructionQueue;
		vector<RenderMaterial*> RenderMaterialDestructionQueue;
		vector<RenderTexture*> RenderTextureDestructionQueue;

		CullingSubSystem cullingSubSystem;

		inline void CheckDXCallResult(HRESULT hr, const char16_t* Function);
		inline const char16_t* GetDXErrorMessageFromHRESULT(HRESULT hr);

		static const UINT MAX_MIP_LEVELS_IN_TEXTURE = 16;
};