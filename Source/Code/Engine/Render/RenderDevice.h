#pragma once

struct RenderMesh {};

struct RenderTexture {};

struct RenderMaterial {};

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

#define SAFE_DX(Func) CheckDXCallResult(Func, u#Func);
#define UUIDOF(Value) __uuidof(Value), (void**)&Value

class RenderDevice
{
	public:

		virtual void InitSystem() = 0;
		virtual void ShutdownSystem() = 0;
		virtual void TickSystem(float DeltaTime) = 0;

		virtual RenderMesh* CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo) = 0;
		virtual RenderTexture* CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo) = 0;
		virtual RenderMaterial* CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo) = 0;

		virtual void DestroyRenderMesh(RenderMesh* renderMesh) = 0;
		virtual void DestroyRenderTexture(RenderTexture* renderTexture) = 0;
		virtual void DestroyRenderMaterial(RenderMaterial* renderMaterial) = 0;

	private:
};