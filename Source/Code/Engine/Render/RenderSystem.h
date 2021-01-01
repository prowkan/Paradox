#pragma once

class RenderSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();
		void TickSystem(float DeltaTime);

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

		ID3D11Buffer *VertexBuffers[4000], *IndexBuffers[4000];

		ID3D11InputLayout *InputLayout;
		ID3D11RasterizerState *RasterizerState;
		ID3D11BlendState *BlendState;
		ID3D11DepthStencilState *DepthStencilState;

		ID3D11VertexShader *VertexShaders[4000];
		ID3D11PixelShader *PixelShaders[4000];

		ID3D11Texture2D *Textures[4000];
		ID3D11ShaderResourceView *TextureSRVs[4000];

		struct
		{
			XMFLOAT3 Location;
			XMFLOAT3 Rotation;
			XMFLOAT3 Scale;
			ID3D11Buffer *VertexBuffer, *IndexBuffer;
			ID3D11VertexShader *VertexShader;
			ID3D11PixelShader *PixelShader;
			ID3D11ShaderResourceView *TextureSRV;
		} RenderObjects[20000];
};