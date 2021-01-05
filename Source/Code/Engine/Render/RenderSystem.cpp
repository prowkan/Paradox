#include "RenderSystem.h"

#include <Core/Application.h>

#include <Engine/Engine.h>

#include <Game/GameObjects/Render/Meshes/StaticMeshObject.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

void RenderSystem::InitSystem()
{
	HRESULT hr;

	UINT DeviceCreationFlags = 0;
	ULONG RefCount;

#ifdef _DEBUG
	DeviceCreationFlags |= D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
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
}

void RenderSystem::ShutdownSystem()
{
	ULONG RefCount;

	for (int k = 0; k < 20000; k++)
	{
		RefCount = ConstantBuffers[k]->Release();
	}

	RefCount = InputLayout->Release();
	RefCount = RasterizerState->Release();
	RefCount = BlendState->Release();
	RefCount = DepthStencilState->Release();

	for (RenderMesh* renderMesh : RenderMeshDestructionQueue)
	{
		RefCount = renderMesh->VertexBuffer->Release();
		RefCount = renderMesh->IndexBuffer->Release();

		delete renderMesh;
	}

	RenderMeshDestructionQueue.clear();

	for (RenderMaterial* renderMaterial : RenderMaterialDestructionQueue)
	{
		RefCount = renderMaterial->VertexShader->Release();
		RefCount = renderMaterial->PixelShader->Release();

		delete renderMaterial;
	}

	RenderMaterialDestructionQueue.clear();

	for (RenderTexture* renderTexture : RenderTextureDestructionQueue)
	{
		RefCount = renderTexture->TextureSRV->Release();
		RefCount = renderTexture->Texture->Release();

		delete renderTexture;
	}

	RenderTextureDestructionQueue.clear();

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

	XMMATRIX ViewProjMatrix = Engine::GetEngine().GetGameFramework().GetCamera().GetViewProjMatrix();

	D3D11_MAPPED_SUBRESOURCE MappedSubResource;

	vector<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
	vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

	OPTICK_EVENT("Draw Calls")

	hr = CPUConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData);

	for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
	{
		XMMATRIX WorldMatrix = VisbleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
		XMMATRIX WVPMatrix = WorldMatrix * ViewProjMatrix;
		
		hr = DeviceContext->Map(ConstantBuffers[k], 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource);
		
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

	UINT Stride = sizeof(Vertex), Offset = 0;
	
	for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
	{
		StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

		RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
		RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
		RenderTexture *renderTexture = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();

		DeviceContext->IASetVertexBuffers(0, 1, &renderMesh->VertexBuffer, &Stride, &Offset);
		DeviceContext->IASetIndexBuffer(renderMesh->IndexBuffer, DXGI_FORMAT::DXGI_FORMAT_R16_UINT, 0);

		DeviceContext->VSSetShader(renderMaterial->VertexShader, nullptr, 0);
		DeviceContext->PSSetShader(renderMaterial->PixelShader, nullptr, 0);

		DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffers[k]);
		DeviceContext->PSSetShaderResources(0, 1, &renderTexture->TextureSRV);

		DeviceContext->DrawIndexed(8 * 8 * 6 * 6, 0, 0);
	}

	hr = SwapChain->Present(0, 0);
}

RenderMesh* RenderSystem::CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo)
{
	RenderMesh *renderMesh = new RenderMesh();

	HRESULT hr;

	D3D11_BUFFER_DESC BufferDesc;
	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = sizeof(Vertex) * renderMeshCreateInfo.VertexCount;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA SubResourceData;
	SubResourceData.pSysMem = renderMeshCreateInfo.VertexData;
	SubResourceData.SysMemPitch = 0;
	SubResourceData.SysMemSlicePitch = 0;

	hr = Device->CreateBuffer(&BufferDesc, &SubResourceData, &renderMesh->VertexBuffer);

	BufferDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;
	BufferDesc.ByteWidth = sizeof(WORD) * renderMeshCreateInfo.IndexCount;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;

	SubResourceData.pSysMem = renderMeshCreateInfo.IndexData;
	SubResourceData.SysMemPitch = 0;
	SubResourceData.SysMemSlicePitch = 0;

	hr = Device->CreateBuffer(&BufferDesc, &SubResourceData, &renderMesh->IndexBuffer);

	return renderMesh;
}

RenderTexture* RenderSystem::CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo)
{
	RenderTexture *renderTexture = new RenderTexture();

	HRESULT hr;

	D3D11_TEXTURE2D_DESC TextureDesc;
	TextureDesc.ArraySize = 1;
	TextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.Format = renderTextureCreateInfo.SRGB ? DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	TextureDesc.Height = renderTextureCreateInfo.Height;
	TextureDesc.MipLevels = renderTextureCreateInfo.MIPLevels;
	TextureDesc.MiscFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
	TextureDesc.Width = renderTextureCreateInfo.Width;

	D3D11_SUBRESOURCE_DATA SubResourceData[16];

	for (UINT i = 0; i < renderTextureCreateInfo.MIPLevels; i++)
	{
		SubResourceData[i].pSysMem = (i == 0) ? renderTextureCreateInfo.TexelData : (BYTE*)SubResourceData[i - 1].pSysMem + 4 * (renderTextureCreateInfo.Width >> (i - 1)) * (renderTextureCreateInfo.Height >> (i - 1));
		SubResourceData[i].SysMemPitch = 4 * (renderTextureCreateInfo.Width >> i);
		SubResourceData[i].SysMemSlicePitch = 0;
	}

	hr = Device->CreateTexture2D(&TextureDesc, SubResourceData, &renderTexture->Texture);

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = renderTextureCreateInfo.SRGB ? DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.Texture2D.MipLevels = renderTextureCreateInfo.MIPLevels;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

	hr = Device->CreateShaderResourceView(renderTexture->Texture, &SRVDesc, &renderTexture->TextureSRV);

	return renderTexture;
}

RenderMaterial* RenderSystem::CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo)
{
	RenderMaterial *renderMaterial = new RenderMaterial();

	HRESULT hr;

	hr = Device->CreateVertexShader(renderMaterialCreateInfo.VertexShaderByteCodeData, renderMaterialCreateInfo.VertexShaderByteCodeLength, nullptr, &renderMaterial->VertexShader);
	hr = Device->CreatePixelShader(renderMaterialCreateInfo.PixelShaderByteCodeData, renderMaterialCreateInfo.PixelShaderByteCodeLength, nullptr, &renderMaterial->PixelShader);

	return renderMaterial;
}

void RenderSystem::DestroyRenderMesh(RenderMesh* renderMesh)
{
	RenderMeshDestructionQueue.push_back(renderMesh);
}

void RenderSystem::DestroyRenderTexture(RenderTexture* renderTexture)
{
	RenderTextureDestructionQueue.push_back(renderTexture);
}

void RenderSystem::DestroyRenderMaterial(RenderMaterial* renderMaterial)
{
	RenderMaterialDestructionQueue.push_back(renderMaterial);
}