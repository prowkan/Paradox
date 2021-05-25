struct VSInput
{
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

#if HLSL_SHADER_MODEL == 50
cbuffer cb0 : register(b0)
{
	VSConstants VertexShaderConstants;
};
#else
ConstantBuffer<VSConstants> VertexShaderConstants : register(b0);
#endif

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), VertexShaderConstants.WVPMatrix);

	return VertexShaderOutput;
}