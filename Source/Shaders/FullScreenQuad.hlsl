#ifdef SPIRV
#define VK_LOCATION(Index) [[vk::location(Index)]]
#define VK_BINDING(Binding, Set) [[vk::binding(Binding, Set)]]
#else
#define VK_LOCATION(Index)
#define VK_BINDING(Binding, Set)
#endif

struct VSOutput
{
	float4 Position : SV_Position;
	VK_LOCATION(0) float2 TexCoord : TEXCOORD;
};

VSOutput VS(uint VertexID : SV_VertexID)
{
	VSOutput VertexShaderOutput;

	VertexShaderOutput.Position = float4(0.0f, 0.0f, 0.0f, 0.0f);
	VertexShaderOutput.TexCoord = float2(0.0f, 0.0f);

	if (VertexID == 0)
	{
		VertexShaderOutput.Position = float4(-1.0f, 1.0f, 0.0f, 1.0f);
		VertexShaderOutput.TexCoord = float2(0.0f, 0.0f);
	}
	else if (VertexID == 1)
	{
		VertexShaderOutput.Position = float4(1.0f, 1.0f, 0.0f, 1.0f);
		VertexShaderOutput.TexCoord = float2(1.0f, 0.0f);
	}
	else if (VertexID == 2)
	{
		VertexShaderOutput.Position = float4(-1.0f, -1.0f, 0.0f, 1.0f);
		VertexShaderOutput.TexCoord = float2(0.0f, 1.0f);
	}
	else if (VertexID == 3)
	{
		VertexShaderOutput.Position = float4(1.0f, -1.0f, 0.0f, 1.0f);
		VertexShaderOutput.TexCoord = float2(1.0f, 1.0f);
	}

	return VertexShaderOutput;
}