struct VSInput
{
	[[vk::location(0)]] float3 Position : POSITION;
	[[vk::location(1)]] float2 TexCoord : TEXCOORD;
	[[vk::location(2)]] float3 Normal : NORMAL;
	[[vk::location(3)]] float3 Tangent : TANGENT;
	[[vk::location(4)]] float3 Binormal : BINORMAL;
};


struct VSOutput
{
	float4 Position : SV_Position;
	[[vk::location(0)]] float2 TexCoord : TEXCOORD;
	[[vk::location(1)]] float3 Normal : NORMAL;
	[[vk::location(2)]] float3 Tangent : TANGENT;
	[[vk::location(3)]] float3 Binormal : BINORMAL;
};

struct VSConstants
{
	float4x4 WVPMatrix;
	float4x4 WorldMatrix;
	float3x3 VectorTransformMatrix;
};

[[vk::binding(0, 0)]] ConstantBuffer<VSConstants> VertexShaderConstants : register(b0);

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), VertexShaderConstants.WVPMatrix);
	VertexShaderOutput.TexCoord = VertexShaderInput.TexCoord;
	VertexShaderOutput.Normal = normalize(mul(VertexShaderInput.Normal, VertexShaderConstants.VectorTransformMatrix));
	VertexShaderOutput.Tangent = normalize(mul(VertexShaderInput.Tangent, VertexShaderConstants.VectorTransformMatrix));
	VertexShaderOutput.Binormal = normalize(mul(VertexShaderInput.Binormal, VertexShaderConstants.VectorTransformMatrix));

	return VertexShaderOutput;
}