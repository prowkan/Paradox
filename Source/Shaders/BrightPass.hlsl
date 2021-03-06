struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2D HDRFrameBufferTexture : register(t0);
Texture2D<float> SceneLuminanceTexture : register(t1);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = PixelShaderInput.Position.xy - 0.5f;

	float3 HDRColor = HDRFrameBufferTexture.Load(int3(Coords, 0)).rgb;
	float PixelLuminance = SceneLuminanceTexture.Load(int3(Coords, 0)).r;

	if (PixelLuminance < 10.0f) return float4(0.0f, 0.0f, 0.0f, 1.0f);

	return float4(0.25f * HDRColor, 1.0f);
}