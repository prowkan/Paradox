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
};