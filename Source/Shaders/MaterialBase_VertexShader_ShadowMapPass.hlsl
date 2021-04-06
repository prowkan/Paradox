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
};

struct VSOutput
{
	float4 Position : SV_Position;
};

struct VSConstants
{
	float4x4 WVPMatrix;
};

VK_BINDING(0, 0) ConstantBuffer<VSConstants> VertexShaderConstants : register(b0);

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), VertexShaderConstants.WVPMatrix);

	return VertexShaderOutput;
}