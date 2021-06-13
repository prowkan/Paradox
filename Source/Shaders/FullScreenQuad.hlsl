struct VSOutput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
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