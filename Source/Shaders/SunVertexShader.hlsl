#ifdef SPIRV
#define VK_LOCATION(Index) [[vk::location(Index)]]
#define VK_BINDING(Binding, Set) [[vk::binding(Binding, Set)]]
#else
#define VK_LOCATION(Index)
#define VK_BINDING(Binding, Set)
#endif

struct VSInput
{
	VK_LOCATION(0) float3 Position : POSITION;
	VK_LOCATION(1) float2 TexCoord : TEXCOORD;
	VK_LOCATION(2) float3 Normal : NORMAL;
	VK_LOCATION(3) float3 Tangent : TANGENT;
	VK_LOCATION(4) float3 Binormal : BINORMAL;
};


struct VSOutput
{
	float4 Position : SV_Position;
	VK_LOCATION(0) float2 TexCoord : TEXCOORD;
};

struct VSConstants
{
	float4x4 ViewMatrix;
	float4x4 ProjMatrix;
	float3 SunPosition;
};

VK_BINDING(0, 0) ConstantBuffer<VSConstants> VertexShaderConstants : register(b0);

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