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

struct VSConstants
{
	float4x4 WVPMatrix;
	float4x4 WorldMatrix;
	float3x3 VectorTransformMatrix;
};

ConstantBuffer<VSConstants> VertexShaderConstants : register(b0);

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