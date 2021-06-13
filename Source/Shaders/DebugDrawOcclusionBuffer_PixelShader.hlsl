struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2D<float> OcclusionBufferTexture : register(t0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = trunc(PixelShaderInput.Position.xy);
	return float4(pow(OcclusionBufferTexture.Load(int3(Coords, 0)), 1.0f / 5.0f).xxx, 1.0f);
}