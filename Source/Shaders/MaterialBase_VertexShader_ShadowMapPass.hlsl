struct VSInput
{
	[[vk::location(0)]] float3 Position : POSITION;
	float3 Position : POSITION;
};

struct VSOutput
{
	float4 Position : SV_Position;
};

struct VSConstants
{
	float4x4 WVPMatrix;
};

[[vk::binding(0, 0)]] ConstantBuffer<VSConstants> VertexShaderConstants : register(b0);

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), VertexShaderConstants.WVPMatrix);

	return VertexShaderOutput;
}