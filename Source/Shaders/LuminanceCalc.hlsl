struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
}; 

Texture2D HDRSceneColorTexture : register(t0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = trunc(PixelShaderInput.Position.xy);

	float3 HDRColor = HDRSceneColorTexture[Coords].rgb;
	float Luminance = 0.2126 * HDRColor.r + 0.7152 * HDRColor.g + 0.0722 * HDRColor.b;
	return float4(Luminance, 0.0f, 0.0f, 0.0f);
}