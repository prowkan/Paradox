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
	COMRCPtr<ID3D11VertexShader> VertexShader;
	COMRCPtr<ID3D11PixelShader> PixelShader;
};

class RenderDeviceDX11 : public RenderDevice
{
	public:

		virtual void InitSystem() override;
		virtual void ShutdownSystem() override;
		virtual void TickSystem(float DeltaTime) override;

		virtual RenderMesh* CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo) override;
		virtual RenderTexture* CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo) override;
		virtual RenderMaterial* CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo) override;

		virtual void DestroyRenderMesh(RenderMesh* renderMesh) override;
		virtual void DestroyRenderTexture(RenderTexture* renderTexture) override;
		virtual void DestroyRenderMaterial(RenderMaterial* renderMaterial) override;

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

		inline void CheckDXCallResult(HRESULT hr, const char16_t* Function);
		inline const char16_t* GetDXErrorMessageFromHRESULT(HRESULT hr);
};