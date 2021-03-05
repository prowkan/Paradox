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

ConstantBuffer<VSConstants> VertexShaderConstants : register(b0);

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), VertexShaderConstants.WVPMatrix);
	VertexShaderOutput.TexCoord = VertexShaderInput.TexCoord;

	return VertexShaderOutput;
}