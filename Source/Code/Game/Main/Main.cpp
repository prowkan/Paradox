// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "D:/D3D12SDK/d3d12.h"
#include "D:/DXCompiler/inc/dxcapi.h"

#include <dxgi1_6.h>

#include <iostream>

using namespace std;

#pragma comment(lib, "D:/DXCompiler/lib/x64/dxcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 4; }
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = u8".\\"; }

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	IDxcCompiler3 *Compiler;

	HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void**)&Compiler);

	const char* VertexShaderGBufferOpaquePass = R"(struct VSInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
};


struct VSOutput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
};

struct VSCameraConstants
{
	float4x4 ViewProjMatrix;
};

struct VSObjectConstants
{
	float4x4 WorldMatrix;
	float3x3 VectorTransformMatrix;
};

cbuffer DrawData : register(b0, space0)
{
	uint4 DataIndices0;
	uint DataIndices1;
}

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	ConstantBuffer<VSCameraConstants> VertexShaderCameraConstants = ResourceDescriptorHeap[DataIndices0.x];
	ConstantBuffer<VSObjectConstants> VertexShaderObjectConstants = ResourceDescriptorHeap[DataIndices0.y];

	float4x4 WVPMatrix = mul(VertexShaderObjectConstants.WorldMatrix, VertexShaderCameraConstants.ViewProjMatrix);

	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), WVPMatrix);
	VertexShaderOutput.TexCoord = VertexShaderInput.TexCoord;
	VertexShaderOutput.Normal = normalize(mul(VertexShaderInput.Normal, VertexShaderObjectConstants.VectorTransformMatrix));
	VertexShaderOutput.Tangent = normalize(mul(VertexShaderInput.Tangent, VertexShaderObjectConstants.VectorTransformMatrix));
	VertexShaderOutput.Binormal = normalize(mul(VertexShaderInput.Binormal, VertexShaderObjectConstants.VectorTransformMatrix));

	return VertexShaderOutput;
})";

	DxcBuffer ShaderSource;
	ShaderSource.Encoding = 0;
	ShaderSource.Ptr = VertexShaderGBufferOpaquePass;
	ShaderSource.Size = strlen(VertexShaderGBufferOpaquePass);

	const char16_t* CompilerArgs[] =
	{
		u"-E VS",
		u"-T vs_6_6",
		u"-Zpr"
	};

	IDxcOperationResult *CompilationResult;

	hr = Compiler->Compile(&ShaderSource, (const wchar_t**)CompilerArgs, 3, NULL, __uuidof(IDxcOperationResult), (void**)&CompilationResult);

	HRESULT CompilationStatus;

	hr = CompilationResult->GetStatus(&CompilationStatus);

	if (FAILED(CompilationStatus))
	{
		IDxcBlobEncoding *ErrorBuffer = nullptr;
		hr = CompilationResult->GetErrorBuffer(&ErrorBuffer);
		if (ErrorBuffer)
		{
			OutputDebugStringA((const char*)ErrorBuffer->GetBufferPointer());
		}
		ExitProcess(0);
	}

	IDxcBlob *VertexShader1Blob;

	hr = CompilationResult->GetResult(&VertexShader1Blob);

	const char* VertexShaderShadowMapPass = R"(struct VSInput
{
	float3 Position : POSITION;
};

struct VSOutput
{
	float4 Position : SV_Position;
};

struct VSCameraConstants
{
	float4x4 ViewProjMatrix;
};

struct VSObjectConstants
{
	float4x4 WorldMatrix;
	float3x3 VectorTransformMatrix;
};

cbuffer DrawData : register(b0)
{
	uint4 DataIndices0;
	uint DataIndices1;
}

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	ConstantBuffer<VSCameraConstants> VertexShaderCameraConstants = ResourceDescriptorHeap[DataIndices0.x];
	ConstantBuffer<VSObjectConstants> VertexShaderObjectConstants = ResourceDescriptorHeap[DataIndices0.y];

	float4x4 WVPMatrix = mul(VertexShaderObjectConstants.WorldMatrix, VertexShaderCameraConstants.ViewProjMatrix);

	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), WVPMatrix);

	return VertexShaderOutput;
})";

	ShaderSource.Encoding = 0;
	ShaderSource.Ptr = VertexShaderShadowMapPass;
	ShaderSource.Size = strlen(VertexShaderShadowMapPass);

	CompilerArgs[0] = u"-E VS";
	CompilerArgs[1] = u"-T vs_6_6";
	CompilerArgs[2] = u"-Zpr";

	hr = Compiler->Compile(&ShaderSource, (const wchar_t**)CompilerArgs, 3, NULL, __uuidof(IDxcOperationResult), (void**)&CompilationResult);

	hr = CompilationResult->GetStatus(&CompilationStatus);

	if (FAILED(CompilationStatus))
	{
		IDxcBlobEncoding *ErrorBuffer = nullptr;
		hr = CompilationResult->GetErrorBuffer(&ErrorBuffer);
		if (ErrorBuffer)
		{
			OutputDebugStringA((const char*)ErrorBuffer->GetBufferPointer());
		}
		ExitProcess(0);
	}

	IDxcBlob *VertexShader2Blob;

	hr = CompilationResult->GetResult(&VertexShader2Blob);

	const char* PixelShaderGBufferOpaquePass = R"(struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
};

struct PSOutput
{
	float4 GBuffer0 : SV_Target0;
	float4 GBuffer1 : SV_Target1;
};

cbuffer DrawData : register(b0, space0)
{
	uint4 DataIndices0;
	uint DataIndices1;
}

PSOutput PS(PSInput PixelShaderInput)
{
	PSOutput PixelShaderOutput;

	Texture2D<float4> DiffuseMap = ResourceDescriptorHeap[DataIndices0.z];
	Texture2D<float4> NormalMap = ResourceDescriptorHeap[DataIndices0.w];

	SamplerState Sampler = SamplerDescriptorHeap[DataIndices1];

	float3 BaseColor = DiffuseMap.Sample(Sampler, PixelShaderInput.TexCoord).rgb;
	float3 Normal;
	Normal.xy = 2.0f * NormalMap.Sample(Sampler, PixelShaderInput.TexCoord).xy - 1.0f;
	Normal.z = sqrt(max(0.0f, 1.0f - Normal.x * Normal.x - Normal.y * Normal.y));
	Normal = normalize(Normal.x * normalize(PixelShaderInput.Tangent) + Normal.y * normalize(PixelShaderInput.Binormal) + Normal.z * normalize(PixelShaderInput.Normal));

	PixelShaderOutput.GBuffer0 = float4(BaseColor, 0.0f);
	PixelShaderOutput.GBuffer1 = float4(Normal * 0.5f + 0.5f, 0.0f);

	return PixelShaderOutput;
})";

	ShaderSource.Encoding = 0;
	ShaderSource.Ptr = PixelShaderGBufferOpaquePass;
	ShaderSource.Size = strlen(PixelShaderGBufferOpaquePass);

	CompilerArgs[0] = u"-E PS";
	CompilerArgs[1] = u"-T ps_6_6";
	CompilerArgs[2] = u"-Zpr";

	hr = Compiler->Compile(&ShaderSource, (const wchar_t**)CompilerArgs, 3, NULL, __uuidof(IDxcOperationResult), (void**)&CompilationResult);

	hr = CompilationResult->GetStatus(&CompilationStatus);

	if (FAILED(CompilationStatus))
	{
		IDxcBlobEncoding *ErrorBuffer = nullptr;
		hr = CompilationResult->GetErrorBuffer(&ErrorBuffer);
		if (ErrorBuffer)
		{
			OutputDebugStringA((const char*)ErrorBuffer->GetBufferPointer());
		}
		ExitProcess(0);
	}

	IDxcBlob *PixelShaderBlob;

	hr = CompilationResult->GetResult(&PixelShaderBlob);

	UINT FactoryCreationFlags = 0;

#ifdef _DEBUG
	ID3D12Debug3 *Debug;
	D3D12GetDebugInterface(__uuidof(ID3D12Debug3), (void**)&Debug);
	Debug->EnableDebugLayer();
	Debug->SetEnableGPUBasedValidation(TRUE);
	FactoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	IDXGIFactory7 *Factory;

	CreateDXGIFactory2(FactoryCreationFlags, __uuidof(IDXGIFactory7), (void**)&Factory);

	IDXGIAdapter *Adapter;

	Factory->EnumAdapters(0, &Adapter);

	IDXGIOutput *Monitor;

	Adapter->EnumOutputs(0, &Monitor);

	UINT DisplayModesCount;
	Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, nullptr);
	DXGI_MODE_DESC *DisplayModes = new DXGI_MODE_DESC[(size_t)DisplayModesCount];
	Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, DisplayModes);

	ID3D12Device *Device;

	D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&Device);

	ID3DBlob *RootSignatureBlob;

	D3D12_ROOT_PARAMETER RootParameters[1];
	RootParameters[0].Descriptor.RegisterSpace = 0;
	RootParameters[0].Descriptor.ShaderRegister = 0;
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc;
	RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	RootSignatureDesc.NumParameters = 1;
	RootSignatureDesc.NumStaticSamplers = 0;
	RootSignatureDesc.pParameters = RootParameters;
	RootSignatureDesc.pStaticSamplers = nullptr;

	D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0, &RootSignatureBlob, nullptr);

	ID3D12RootSignature *SceneRenderRootSignature;

	Device->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&SceneRenderRootSignature);

	D3D12_INPUT_ELEMENT_DESC InputElementDescs[5];
	InputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[0].InputSlot = 0;
	InputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[0].InstanceDataStepRate = 0;
	InputElementDescs[0].SemanticIndex = 0;
	InputElementDescs[0].SemanticName = "POSITION";
	InputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[1].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	InputElementDescs[1].InputSlot = 1;
	InputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[1].InstanceDataStepRate = 0;
	InputElementDescs[1].SemanticIndex = 0;
	InputElementDescs[1].SemanticName = "TEXCOORD";
	InputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[2].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[2].InputSlot = 2;
	InputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[2].InstanceDataStepRate = 0;
	InputElementDescs[2].SemanticIndex = 0;
	InputElementDescs[2].SemanticName = "NORMAL";
	InputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[3].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[3].InputSlot = 2;
	InputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[3].InstanceDataStepRate = 0;
	InputElementDescs[3].SemanticIndex = 0;
	InputElementDescs[3].SemanticName = "TANGENT";
	InputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[4].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[4].InputSlot = 2;
	InputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[4].InstanceDataStepRate = 0;
	InputElementDescs[4].SemanticIndex = 0;
	InputElementDescs[4].SemanticName = "BINORMAL";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 5;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 2;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = SceneRenderRootSignature;
	//GraphicsPipelineStateDesc.pRootSignature = nullptr;
	GraphicsPipelineStateDesc.PS.BytecodeLength = PixelShaderBlob->GetBufferSize();
	GraphicsPipelineStateDesc.PS.pShaderBytecode = PixelShaderBlob->GetBufferPointer();
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	GraphicsPipelineStateDesc.RTVFormats[1] = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = VertexShader1Blob->GetBufferSize();
	GraphicsPipelineStateDesc.VS.pShaderBytecode = VertexShader1Blob->GetBufferPointer();

	ID3D12PipelineState *TestPipeline;

	Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&TestPipeline);

	/*ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 1;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = SceneRenderRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = renderMaterialCreateInfo.ShadowMapPassPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = renderMaterialCreateInfo.ShadowMapPassPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeLength;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(renderMaterial->ShadowMapPassPipelineState)));*/

	return 0;
}