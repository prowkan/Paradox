struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
}; 

Texture2D<float> InputLuminanceTexture : register(t0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = trunc(PixelShaderInput.Position.xy);

	float SummedLuminance = 0.0f;

	[unroll]
	for (int i = 0; i < 16; i++)
	{
		[unroll]
		for (int j = 0; j < 16; j++)
		{
			SummedLuminance += InputLuminanceTexture[int2(Coords.x * 16 + i, Coords.y * 16 + j)];
		}
	}

	return float4(SummedLuminance, 0.0f, 0.0f, 0.0f);
}