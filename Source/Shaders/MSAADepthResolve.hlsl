struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2DMS<float> DepthBufferTexture : register(t0);

float PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = PixelShaderInput.Position.xy - 0.5f;

	float Depth = DepthBufferTexture.Load(Coords, 0).x;
	
	[unroll]
	for (uint i = 0; i < 8; i++)
	{
		Depth = max(Depth, DepthBufferTexture.Load(Coords, i).x);
	}

	return Depth;
}