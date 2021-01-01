#include "RenderSystem.h"

#include <Core/Application.h>

struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT2 TexCoord;
};

struct Texel
{
	BYTE R, G, B, A;
};

void RenderSystem::InitSystem()
{
	HRESULT hr;

	UINT DeviceCreationFlags = 0;
	ULONG RefCount;

#ifdef _DEBUG
	DeviceCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	IDXGIFactory *Factory;

	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)&Factory);

	IDXGIAdapter *Adapter;

	hr = Factory->EnumAdapters(0, (IDXGIAdapter**)&Adapter);

	IDXGIOutput *Monitor;

	hr = Adapter->EnumOutputs(0, &Monitor);

	UINT DisplayModesCount;
	hr = Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, nullptr);
	DXGI_MODE_DESC *DisplayModes = new DXGI_MODE_DESC[DisplayModesCount];
	hr = Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, DisplayModes);

	RefCount = Monitor->Release();

	ResolutionWidth = DisplayModes[DisplayModesCount - 1].Width;
	ResolutionHeight = DisplayModes[DisplayModesCount - 1].Height;

	D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;

	hr = D3D11CreateDevice(Adapter, D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN, NULL, DeviceCreationFlags, &FeatureLevel, 1, D3D11_SDK_VERSION, &Device, nullptr, &DeviceContext);

	RefCount = Adapter->Release();

	DXGI_SWAP_CHAIN_DESC SwapChainDesc;
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SwapChainDesc.BufferDesc.Height = ResolutionHeight;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = DisplayModes[DisplayModesCount - 1].RefreshRate.Numerator;
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = DisplayModes[DisplayModesCount - 1].RefreshRate.Denominator;
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainDesc.BufferDesc.Width = ResolutionWidth;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	SwapChainDesc.OutputWindow = Application::GetMainWindowHandle();
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_SEQUENTIAL;
	SwapChainDesc.Windowed = TRUE;

	hr = Factory->CreateSwapChain(Device, &SwapChainDesc, &SwapChain);
	
	hr = Factory->MakeWindowAssociation(Application::GetMainWindowHandle(), DXGI_MWA_NO_ALT_ENTER);

	RefCount = Factory->Release();

	delete[] DisplayModes;

	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBufferTexture);

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;

	hr = Device->CreateRenderTargetView(BackBufferTexture, &RTVDesc, &BackBufferRTV);
	
	D3D11_TEXTURE2D_DESC TextureDesc;
	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	TextureDesc.Height = ResolutionHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	TextureDesc.Width = ResolutionWidth;

	hr = Device->CreateTexture2D(&TextureDesc, nullptr, &DepthBufferTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	DSVDesc.Flags = 0;
	DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	DSVDesc.Texture2D.MipSlice = 0;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;

	hr = Device->CreateDepthStencilView(DepthBufferTexture, &DSVDesc, &DepthBufferDSV);

	D3D11_BUFFER_DESC BufferDesc;
	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.ByteWidth = 64;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;

	for (int i = 0; i < 20000; i++)
	{
		hr = Device->CreateBuffer(&BufferDesc, nullptr, &ConstantBuffers[i]);
	}

	D3D11_SAMPLER_DESC SamplerDesc;
	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.ComparisonFunc = (D3D11_COMPARISON_FUNC)0;
	SamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_ANISOTROPIC;
	SamplerDesc.MaxAnisotropy = 16;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MipLODBias = 0.0f;

	hr = Device->CreateSamplerState(&SamplerDesc, &Sampler);

	const int VertexCount = 9 * 9 * 6;
	const int IndexCount = 8 * 8 * 6 * 6;

	Vertex Vertices[VertexCount];

	for (int i = 0; i < 9; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			Vertices[0 + 9 * i + j].Position = XMFLOAT3(-1.0f + j * 0.25f, 1.0f - i * 0.25f, -1.0f);
			Vertices[0 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);

			Vertices[81 + 9 * i + j].Position = XMFLOAT3(1.0f, 1.0f - i * 0.25f, -1.0f + j * 0.25f);
			Vertices[81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);

			Vertices[2 * 81 + 9 * i + j].Position = XMFLOAT3(1.0f - j * 0.25f, 1.0f - i * 0.25f, 1.0f);
			Vertices[2 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);

			Vertices[3 * 81 + 9 * i + j].Position = XMFLOAT3(-1.0f, 1.0f - i * 0.25f, 1.0f - j * 0.25f);
			Vertices[3 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);

			Vertices[4 * 81 + 9 * i + j].Position = XMFLOAT3(-1.0f + j * 0.25f, 1.0f, 1.0f - i * 0.25f);
			Vertices[4 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);

			Vertices[5 * 81 + 9 * i + j].Position = XMFLOAT3(-1.0f + j * 0.25f, -1.0f, -1.0f + i * 0.25f);
			Vertices[5 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);
		}
	}	

	WORD Indices[IndexCount];

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 0 * 81 + 9 * i + j;
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 0 * 81 + 9 * i + j + 1;
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 0 * 81 + 9 * (i + 1) + j;
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 0 * 81 + 9 * (i + 1) + j;
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 0 * 81 + 9 * i + j + 1;
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 0 * 81 + 9 * (i + 1) + j + 1;

			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 1 * 81 + 9 * i + j;
			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 1 * 81 + 9 * i + j + 1;
			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 1 * 81 + 9 * (i + 1) + j;
			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 1 * 81 + 9 * (i + 1) + j;
			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 1 * 81 + 9 * i + j + 1;
			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 1 * 81 + 9 * (i + 1) + j + 1;

			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 2 * 81 + 9 * i + j;
			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 2 * 81 + 9 * i + j + 1;
			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 2 * 81 + 9 * (i + 1) + j;
			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 2 * 81 + 9 * (i + 1) + j;
			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 2 * 81 + 9 * i + j + 1;
			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 2 * 81 + 9 * (i + 1) + j + 1;

			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 3 * 81 + 9 * i + j;
			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 3 * 81 + 9 * i + j + 1;
			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 3 * 81 + 9 * (i + 1) + j;
			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 3 * 81 + 9 * (i + 1) + j;
			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 3 * 81 + 9 * i + j + 1;
			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 3 * 81 + 9 * (i + 1) + j + 1;

			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 4 * 81 + 9 * i + j;
			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 4 * 81 + 9 * i + j + 1;
			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 4 * 81 + 9 * (i + 1) + j;
			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 4 * 81 + 9 * (i + 1) + j;
			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 4 * 81 + 9 * i + j + 1;
			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 4 * 81 + 9 * (i + 1) + j + 1;

			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 5 * 81 + 9 * i + j;
			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 5 * 81 + 9 * i + j + 1;
			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 5 * 81 + 9 * (i + 1) + j;
			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 5 * 81 + 9 * (i + 1) + j;
			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 5 * 81 + 9 * i + j + 1;
			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 5 * 81 + 9 * (i + 1) + j + 1;
		}
	}

	for (int k = 0; k < 4000; k++)
	{
		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.ByteWidth = sizeof(Vertex) * VertexCount;
		BufferDesc.CPUAccessFlags = 0;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

		D3D11_SUBRESOURCE_DATA SubResourceData;
		SubResourceData.pSysMem = Vertices;
		SubResourceData.SysMemPitch = 0;
		SubResourceData.SysMemSlicePitch = 0;

		hr = Device->CreateBuffer(&BufferDesc, &SubResourceData, &VertexBuffers[k]);

		BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;
		BufferDesc.ByteWidth = sizeof(WORD) * IndexCount;
		BufferDesc.CPUAccessFlags = 0;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

		SubResourceData.pSysMem = Indices;
		SubResourceData.SysMemPitch = 0;
		SubResourceData.SysMemSlicePitch = 0;

		hr = Device->CreateBuffer(&BufferDesc, &SubResourceData, &IndexBuffers[k]);		
	}

	D3D11_INPUT_ELEMENT_DESC InputElementDescs[2];
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

	const char *VertexShaderSourceCode = R"(
	
			struct VSInput
			{
				float3 Position : POSITION;
				float2 TexCoord : TEXCOORD;
			};

			struct VSOutput
			{
				float4 Position : SV_Position;
				float2 TexCoord : TEXCOORD;
			};

			struct VSConstants
			{
				float4x4 WVPMatrix;
			};

			cbuffer cb0 : register(b0)
			{
				VSConstants VertexShaderConstants;
			};

			VSOutput VS(VSInput VertexShaderInput)
			{
				VSOutput VertexShaderOutput;

				VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), VertexShaderConstants.WVPMatrix);
				VertexShaderOutput.TexCoord = VertexShaderInput.TexCoord;

				return VertexShaderOutput;
			}

		)";

	ID3DBlob *ErrorBlob = nullptr;

	ID3DBlob *VertexShaderBlob;

	hr = D3DCompile(VertexShaderSourceCode, strlen(VertexShaderSourceCode), "VertexShader", nullptr, nullptr, "VS", "vs_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &VertexShaderBlob, &ErrorBlob);

	hr = Device->CreateInputLayout(InputElementDescs, 2, VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), &InputLayout);

	RefCount = VertexShaderBlob->Release();

	D3D11_RASTERIZER_DESC RasterizerDesc;
	ZeroMemory(&RasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	RasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
	RasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;

	hr = Device->CreateRasterizerState(&RasterizerDesc, &RasterizerState);

	D3D11_BLEND_DESC BlendDesc;
	ZeroMemory(&BlendDesc, sizeof(D3D11_BLEND_DESC));
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = Device->CreateBlendState(&BlendDesc, &BlendState);

	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
	ZeroMemory(&DepthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	DepthStencilDesc.DepthEnable = TRUE;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;

	hr = Device->CreateDepthStencilState(&DepthStencilDesc, &DepthStencilState);

	for (int k = 0; k < 4000; k++)
	{
		const char *VertexShaderSourceCode = R"(
	
			struct VSInput
			{
				float3 Position : POSITION;
				float2 TexCoord : TEXCOORD;
			};

			struct VSOutput
			{
				float4 Position : SV_Position;
				float2 TexCoord : TEXCOORD;
			};

			struct VSConstants
			{
				float4x4 WVPMatrix;
			};

			cbuffer cb0 : register(b0)
			{
				VSConstants VertexShaderConstants;
			};

			VSOutput VS(VSInput VertexShaderInput)
			{
				VSOutput VertexShaderOutput;

				VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), VertexShaderConstants.WVPMatrix);
				VertexShaderOutput.TexCoord = VertexShaderInput.TexCoord;

				return VertexShaderOutput;
			}

		)";

		const char *PixelShaderSourceCode = R"(

			struct PSInput
			{
				float4 Position : SV_Position;
				float2 TexCoord : TEXCOORD;
			};

			Texture2D Texture : register(t0);
			SamplerState Sampler : register(s0);

			float4 PS(PSInput PixelShaderInput) : SV_Target
			{
				return float4(Texture.Sample(Sampler, PixelShaderInput.TexCoord).rgb, 1.0f);
			}

		)";

		ID3DBlob *ErrorBlob = nullptr;

		ID3DBlob *VertexShaderBlob, *PixelShaderBlob;

		hr = D3DCompile(VertexShaderSourceCode, strlen(VertexShaderSourceCode), "VertexShader", nullptr, nullptr, "VS", "vs_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &VertexShaderBlob, &ErrorBlob);
		hr = D3DCompile(PixelShaderSourceCode, strlen(PixelShaderSourceCode), "PixelShader", nullptr, nullptr, "PS", "ps_5_0", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &PixelShaderBlob, &ErrorBlob);

		hr = Device->CreateVertexShader(VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), nullptr, &VertexShaders[k]);
		hr = Device->CreatePixelShader(PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize(), nullptr, &PixelShaders[k]);

		RefCount = VertexShaderBlob->Release();
		RefCount = PixelShaderBlob->Release();
	}

	Texel *Texels[8];

	Texels[0] = new Texel[512 * 512];
	Texels[1] = new Texel[256 * 256];
	Texels[2] = new Texel[128 * 128];
	Texels[3] = new Texel[64 * 64];
	Texels[4] = new Texel[32 * 32];
	Texels[5] = new Texel[16 * 16];
	Texels[6] = new Texel[8 * 8];
	Texels[7] = new Texel[4 * 4];

	for (int y = 0; y < 512; y++)
	{
		for (int x = 0; x < 512; x++)
		{
			Texels[0][y * 512 + x].R = ((x / 64) + (y / 64)) % 2 ? 0 : 255;
			Texels[0][y * 512 + x].G = ((x / 64) + (y / 64)) % 2 ? 0 : 255;
			Texels[0][y * 512 + x].B = ((x / 64) + (y / 64)) % 2 ? 192 : 255;
			Texels[0][y * 512 + x].A = 255;
		}
	}

	for (int k = 1; k < 8; k++)
	{
		int MIPSize = 512 >> k;

		for (int y = 0; y < MIPSize; y++)
		{
			for (int x = 0; x < MIPSize; x++)
			{
				Texels[k][y * MIPSize + x].R = BYTE(0.25f * ((float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x)].R + (float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x + 1)].R + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x)].R + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x + 1)].R));
				Texels[k][y * MIPSize + x].G = BYTE(0.25f * ((float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x)].G + (float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x + 1)].G + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x)].G + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x + 1)].G));
				Texels[k][y * MIPSize + x].B = BYTE(0.25f * ((float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x)].B + (float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x + 1)].B + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x)].B + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x + 1)].B));
				Texels[k][y * MIPSize + x].A = 255;
			}
		}
	}

	for (int k = 0; k < 4000; k++)
	{
		TextureDesc.ArraySize = 1;
		TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		TextureDesc.CPUAccessFlags = 0;
		TextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		TextureDesc.Height = 512;
		TextureDesc.MipLevels = 8;
		TextureDesc.MiscFlags = 0;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
		TextureDesc.Width = 512;

		D3D11_SUBRESOURCE_DATA SubResourceData[8];
		SubResourceData[0].pSysMem = Texels[0];
		SubResourceData[0].SysMemPitch = 2048;
		SubResourceData[0].SysMemSlicePitch = 0;
		SubResourceData[1].pSysMem = Texels[1];
		SubResourceData[1].SysMemPitch = 1024;
		SubResourceData[1].SysMemSlicePitch = 0;
		SubResourceData[2].pSysMem = Texels[2];
		SubResourceData[2].SysMemPitch = 512;
		SubResourceData[2].SysMemSlicePitch = 0;
		SubResourceData[3].pSysMem = Texels[3];
		SubResourceData[3].SysMemPitch = 256;
		SubResourceData[3].SysMemSlicePitch = 0;
		SubResourceData[4].pSysMem = Texels[4];
		SubResourceData[4].SysMemPitch = 128;
		SubResourceData[4].SysMemSlicePitch = 0;
		SubResourceData[5].pSysMem = Texels[5];
		SubResourceData[5].SysMemPitch = 64;
		SubResourceData[5].SysMemSlicePitch = 0;
		SubResourceData[6].pSysMem = Texels[6];
		SubResourceData[6].SysMemPitch = 32;
		SubResourceData[6].SysMemSlicePitch = 0;
		SubResourceData[7].pSysMem = Texels[7];
		SubResourceData[7].SysMemPitch = 16;
		SubResourceData[7].SysMemSlicePitch = 0;

		hr = Device->CreateTexture2D(&TextureDesc, SubResourceData, &Textures[k]);

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		SRVDesc.Texture2D.MipLevels = 8;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

		hr = Device->CreateShaderResourceView(Textures[k], &SRVDesc, &TextureSRVs[k]);
	}

	delete[] Texels[0];
	delete[] Texels[1];
	delete[] Texels[2];
	delete[] Texels[3];
	delete[] Texels[4];
	delete[] Texels[5];
	delete[] Texels[6];
	delete[] Texels[7];

	for (int k = 0; k < 20000; k++)
	{
		int i = ((k / 2) / 100) - 50;
		int j = ((k / 2) % 100) - 50;

		RenderObjects[k].Location = XMFLOAT3(i * 5.0f + 2.5f, -0.0f, j * 5.0f + 2.5f);
		RenderObjects[k].Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		RenderObjects[k].Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
		RenderObjects[k].VertexBuffer = VertexBuffers[k % 4000];
		RenderObjects[k].IndexBuffer = IndexBuffers[k % 4000];
		RenderObjects[k].VertexShader = VertexShaders[k % 4000];
		RenderObjects[k].PixelShader = PixelShaders[k % 4000];
		RenderObjects[k].TextureSRV = TextureSRVs[k % 4000];

		k++;

		RenderObjects[k].Location = XMFLOAT3(i * 10.0f + 5.0f, -2.0f, j * 10.0f + 5.0f);
		RenderObjects[k].Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		RenderObjects[k].Scale = XMFLOAT3(5.0f, 1.0f, 5.0f);
		RenderObjects[k].VertexBuffer = VertexBuffers[k % 4000];
		RenderObjects[k].IndexBuffer = IndexBuffers[k % 4000];
		RenderObjects[k].VertexShader = VertexShaders[k % 4000];
		RenderObjects[k].PixelShader = PixelShaders[k % 4000];
		RenderObjects[k].TextureSRV = TextureSRVs[k % 4000];
	}
}

void RenderSystem::ShutdownSystem()
{
	ULONG RefCount;

	for (int k = 0; k < 4000; k++)
	{
		RefCount = VertexBuffers[k]->Release();
		RefCount = IndexBuffers[k]->Release();

		RefCount = VertexShaders[k]->Release();
		RefCount = PixelShaders[k]->Release();

		RefCount = TextureSRVs[k]->Release();
		RefCount = Textures[k]->Release();
	}

	for (int k = 0; k < 20000; k++)
	{
		RefCount = ConstantBuffers[k]->Release();
	}

	RefCount = InputLayout->Release();
	RefCount = RasterizerState->Release();
	RefCount = BlendState->Release();
	RefCount = DepthStencilState->Release();

	RefCount = Sampler->Release();

	RefCount = BackBufferRTV->Release();
	RefCount = BackBufferTexture->Release();

	RefCount = DepthBufferDSV->Release();
	RefCount = DepthBufferTexture->Release();

	RefCount = SwapChain->Release();

	RefCount = DeviceContext->Release();
	RefCount = Device->Release();
}

void RenderSystem::TickSystem(float DeltaTime)
{
	HRESULT hr;

	DeviceContext->ClearState();

	XMMATRIX ViewProjMatrix = XMMatrixLookToLH(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)) * XMMatrixPerspectiveFovLH(3.14f / 2.0f, 16.0f / 9.0f, 0.01f, 1000.0f);

	D3D11_MAPPED_SUBRESOURCE MappedSubResource;

	for (int k = 0; k < 20000; k++)
	{
		hr = DeviceContext->Map(ConstantBuffers[k], 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource);

		XMMATRIX WorldMatrix = XMMatrixRotationRollPitchYaw(RenderObjects[k].Rotation.x, RenderObjects[k].Rotation.y, RenderObjects[k].Rotation.z) * XMMatrixScaling(RenderObjects[k].Scale.x, RenderObjects[k].Scale.y, RenderObjects[k].Scale.z) * XMMatrixTranslation(RenderObjects[k].Location.x, RenderObjects[k].Location.y, RenderObjects[k].Location.z);
		XMMATRIX WVPMatrix = WorldMatrix * ViewProjMatrix;

		memcpy(MappedSubResource.pData, &WVPMatrix, sizeof(XMMATRIX));

		DeviceContext->Unmap(ConstantBuffers[k], 0);
	}

	DeviceContext->OMSetRenderTargets(1, &BackBufferRTV, DepthBufferDSV);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
	DeviceContext->OMSetBlendState(BlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
	DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);

	DeviceContext->PSSetSamplers(0, 1, &Sampler);

	float ClearColor[4] = { 0.0f, 0.5f, 1.0f, 1.0f };
	DeviceContext->ClearRenderTargetView(BackBufferRTV, ClearColor);
	DeviceContext->ClearDepthStencilView(DepthBufferDSV, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL, 1.0f, 0);

	for (int k = 0; k < 20000; k++)
	{
		UINT Stride = sizeof(Vertex), Offset = 0;

		DeviceContext->IASetVertexBuffers(0, 1, &RenderObjects[k].VertexBuffer, &Stride, &Offset);
		DeviceContext->IASetIndexBuffer(RenderObjects[k].IndexBuffer, DXGI_FORMAT::DXGI_FORMAT_R16_UINT, 0);

		DeviceContext->VSSetShader(RenderObjects[k].VertexShader, nullptr, 0);
		DeviceContext->PSSetShader(RenderObjects[k].PixelShader, nullptr, 0);

		DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffers[k]);
		DeviceContext->PSSetShaderResources(0, 1, &RenderObjects[k].TextureSRV);

		DeviceContext->DrawIndexed(8 * 8 * 6 * 6, 0, 0);
	}

	hr = SwapChain->Present(0, 0);
}