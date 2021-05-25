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
};

struct VSConstants
{
	float4x4 ViewMatrix;
	float4x4 ProjMatrix;
	float3 SunPosition;
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

	float4 SunPosition = mul(float4(VertexShaderConstants.SunPosition, 1.0f), VertexShaderConstants.ViewMatrix);
	SunPosition.xyz += 50.0f * VertexShaderInput.Position;
	SunPosition = mul(SunPosition, VertexShaderConstants.ProjMatrix);

	VertexShaderOutput.Position = SunPosition;
	VertexShaderOutput.TexCoord = VertexShaderInput.TexCoord;

	return VertexShaderOutput;
}