struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
}; 

Texture2D<float> InputLuminanceTexture : register(t0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = trunc(PixelShaderInput.Position.xy);

	float SummedLuminance = InputLuminanceTexture[int2(2 * Coords.x, 2 * Coords.y)].r;
	SummedLuminance += InputLuminanceTexture[int2(2 * Coords.x + 1, 2 * Coords.y)].r;
	SummedLuminance += InputLuminanceTexture[int2(2 * Coords.x, 2 * Coords.y + 1)].r;
	SummedLuminance += InputLuminanceTexture[int2(2 * Coords.x + 1, 2 * Coords.y + 1)].r;

	return float4(SummedLuminance, 0.0f, 0.0f, 0.0f);
}