struct VSInput
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

ConstantBuffer<VSCameraConstants> VertexShaderCameraConstants : register(b0);
ConstantBuffer<VSObjectConstants> VertexShaderObjectConstants : register(b1);

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	float4x4 WVPMatrix = mul(VertexShaderObjectConstants.WorldMatrix, VertexShaderCameraConstants.ViewProjMatrix);

	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), WVPMatrix);

	return VertexShaderOutput;
}