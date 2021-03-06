struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2DMS<float> DepthBufferTexture : register(t0);

float4 PS(PSInput PixelShaderInput, uint SampleIndex : SV_SampleIndex) : SV_Target
{
	int2 Coords = trunc(PixelShaderInput.Position.xy);

	float Depth = DepthBufferTexture.Load(Coords, SampleIndex).x;

	if (Depth == 0.0f) return float4(0.0f, 0.0f, 0.0f, 0.0f);

	float Near = 1000.0f;
	float Far = 0.01f;

	float a = Far / (Far - Near);
	float b = (Far * Near) / (Far - Near);

	Depth = b / (a - Depth);

	return float4(0.0f, 0.2f, 1.0f, exp(-0.01f * Depth));
}