#pragma once

#include "CullingSubSystem.h"

struct RenderMesh
{
	ID3D11Buffer *VertexBuffer, *IndexBuffer;
};

struct RenderTexture
{
	ID3D11Texture2D *Texture;
	ID3D11ShaderResourceView *TextureSRV;
};

struct RenderMaterial
{
	ID3D11VertexShader *VertexShader;
	ID3D11PixelShader *PixelShader;
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

#define SAFE_DX(Func) CheckDXCallResult(Func, L#Func);
#define SAFE_RELEASE(Object) if (Object) { ULONG RefCount = Object->Release(); Object = nullptr; }

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

		ID3D11Device *Device;
		IDXGISwapChain *SwapChain;

		int ResolutionWidth;
		int ResolutionHeight;

		ID3D11DeviceContext *DeviceContext;

		ID3D11Texture2D *BackBufferTexture;
		ID3D11RenderTargetView *BackBufferRTV;

		ID3D11Texture2D *DepthBufferTexture;
		ID3D11DepthStencilView *DepthBufferDSV;

		ID3D11Buffer *ConstantBuffers[20000];

		ID3D11SamplerState *Sampler;

		ID3D11InputLayout *InputLayout;
		ID3D11RasterizerState *RasterizerState;
		ID3D11BlendState *BlendState;
		ID3D11DepthStencilState *DepthStencilState;

		vector<RenderMesh*> RenderMeshDestructionQueue;
		vector<RenderMaterial*> RenderMaterialDestructionQueue;
		vector<RenderTexture*> RenderTextureDestructionQueue;

		CullingSubSystem cullingSubSystem;

		inline void CheckDXCallResult(HRESULT hr, const wchar_t* Function);
		inline const wchar_t* GetDXErrorMessageFromHRESULT(HRESULT hr);
};