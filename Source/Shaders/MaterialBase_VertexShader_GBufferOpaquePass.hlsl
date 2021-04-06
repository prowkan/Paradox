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
	VK_LOCATION(1) float3 Normal : NORMAL;
	VK_LOCATION(2) float3 Tangent : TANGENT;
	VK_LOCATION(3) float3 Binormal : BINORMAL;
};

struct VSConstants
{
	float4x4 WVPMatrix;
	float4x4 WorldMatrix;
	float3x3 VectorTransformMatrix;
};

VK_BINDING(0, 0) ConstantBuffer<VSConstants> VertexShaderConstants : register(b0);

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