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

struct VSObjectDataIndicesConstants
{
	uint ObjectIndex;
};

struct VSMaterialDataIndicesConstants
{
	uint TextureIndex;
};

struct VSCameraConstants
{
	float4x4 ViewProjMatrix;
};

struct VSObjectConstants
{
	float4x4 WorldMatrix;
};

ConstantBuffer<VSObjectDataIndicesConstants> VertexShaderObjectDataIndicesConstants : register(b0, space0);
ConstantBuffer<VSMaterialDataIndicesConstants> VertexShaderMaterialDataIndicesConstants : register(b0, space1);
ConstantBuffer<VSCameraConstants> VertexShaderCameraConstants : register(b0, space2);

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	ConstantBuffer<VSObjectConstants> VertexShaderObjectsConstants = ResourceDescriptorHeap[1 + VertexShaderObjectDataIndicesConstants.ObjectIndex];

	float4x4 WVPMatrix = mul(VertexShaderObjectsConstants.WorldMatrix, VertexShaderCameraConstants.ViewProjMatrix);
	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), WVPMatrix);
	VertexShaderOutput.TexCoord = VertexShaderInput.TexCoord;

	return VertexShaderOutput;
}