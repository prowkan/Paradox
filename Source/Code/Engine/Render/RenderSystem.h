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
	COMRCPtr<ID3D11VertexShader> VertexShader;
	COMRCPtr<ID3D11PixelShader> PixelShader;
};

struct RenderMeshCreateInfo
{
	UINT VertexCount;
	void *VertexData;
	UINT IndexCount;
	void *IndexData;
};

struct RenderTextureCreateInfo
{
	UINT Width, Height;
	UINT MIPLevels;
	BOOL SRGB;
	BOOL Compressed;
	BYTE *TexelData;
};

struct RenderMaterialCreateInfo
{
	UINT VertexShaderByteCodeLength;
	void *VertexShaderByteCodeData;
	UINT PixelShaderByteCodeLength;
	void *PixelShaderByteCodeData;
};

struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT2 TexCoord;
};

struct Texel
{
	BYTE R, G, B, A;
};

struct CompressedTexelBlock
{
	uint16_t Colors[2];
	uint8_t Texels[4];
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
	return powf((float)Color1.R - (float)Color2.R, 2.0f) + powf((float)Color1.G - (float)Color2.G, 2.0f) + powf((float)Color1.B - (float)Color2.B, 2.0f);
}

#define SAFE_DX(Func) CheckDXCallResult(Func, L#Func);
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
		COMRCPtr<IDXGISwapChain> SwapChain;

		int ResolutionWidth;
		int ResolutionHeight;

		COMRCPtr<ID3D11DeviceContext> DeviceContext;

		ID3D11Texture2D *BackBufferTexture;
		ID3D11RenderTargetView *BackBufferRTV;

		COMRCPtr<ID3D11Texture2D> DepthBufferTexture;
		COMRCPtr<ID3D11DepthStencilView> DepthBufferDSV;

		COMRCPtr<ID3D11Buffer> ConstantBuffers[20000];

		COMRCPtr<ID3D11SamplerState> Sampler;

		COMRCPtr<ID3D11InputLayout> InputLayout;
		COMRCPtr<ID3D11RasterizerState> RasterizerState;
		COMRCPtr<ID3D11BlendState> BlendState;
		COMRCPtr<ID3D11DepthStencilState> DepthStencilState;

		vector<RenderMesh*> RenderMeshDestructionQueue;
		vector<RenderMaterial*> RenderMaterialDestructionQueue;
		vector<RenderTexture*> RenderTextureDestructionQueue;

		CullingSubSystem cullingSubSystem;

		inline void CheckDXCallResult(HRESULT hr, const wchar_t* Function);
		inline const wchar_t* GetDXErrorMessageFromHRESULT(HRESULT hr);

		UINT FramesCount = 0;
		UINT64 MilliSeconds = 0;

		float FPS;

		COMRCPtr<ID2D1Device6> D2DDevice;
		COMRCPtr<ID2D1DeviceContext6> D2DDeviceContext;

		COMRCPtr<ID2D1Bitmap1> BackBufferBitmap;

		COMRCPtr<ID2D1SolidColorBrush> WhiteTextBrush, BlueTextBrush;

		COMRCPtr<IDWriteFactory> DWFactory;
		COMRCPtr<IDWriteTextFormat> DWTextFormat;
};