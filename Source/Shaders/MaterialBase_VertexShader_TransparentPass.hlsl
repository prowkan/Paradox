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

ConstantBuffer<VSCameraConstants> VertexShaderCameraConstants : register(b0);
ConstantBuffer<VSObjectConstants> VertexShaderObjectConstants : register(b1);

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	float4x4 WVPMatrix = mul(VertexShaderObjectConstants.WorldMatrix, VertexShaderCameraConstants.ViewProjMatrix);

	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), WVPMatrix);
	VertexShaderOutput.TexCoord = VertexShaderInput.TexCoord;
	VertexShaderOutput.Normal = normalize(mul(VertexShaderInput.Normal, VertexShaderObjectConstants.VectorTransformMatrix));
	VertexShaderOutput.Tangent = normalize(mul(VertexShaderInput.Tangent, VertexShaderObjectConstants.VectorTransformMatrix));
	VertexShaderOutput.Binormal = normalize(mul(VertexShaderInput.Binormal, VertexShaderObjectConstants.VectorTransformMatrix));

	return VertexShaderOutput;
}