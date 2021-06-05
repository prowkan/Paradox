struct VSInput
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

[RootSignature("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED), CBV(b0, space = 0, visibility = SHADER_VISIBILITY_ALL)")]
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

	/*VertexShaderOutput.Position = float4(VertexShaderInput.Position, 1.0f);
	VertexShaderOutput.TexCoord = VertexShaderInput.TexCoord;
	VertexShaderOutput.Normal = VertexShaderInput.Normal;
	VertexShaderOutput.Tangent = VertexShaderInput.Tangent;
	VertexShaderOutput.Binormal = VertexShaderInput.Binormal;*/

	return VertexShaderOutput;
}