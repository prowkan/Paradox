struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};

Texture2D<float> DepthBufferTexture : register(t0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	int2 Coords = PixelShaderInput.Position.xy - 0.5f;

	float Depth = DepthBufferTexture.Load(int3(Coords, 0)).x;

	if (Depth == 0.0f) return float4(0.0f, 0.0f, 0.0f, 0.0f);

	float Near = 1000.0f;
	float Far = 0.01f;

	float a = Far / (Far - Near);
	float b = (Far * Near) / (Far - Near);

	Depth = b / (a - Depth);

	return float4(0.0f, 0.2f, 1.0f, exp(-0.01f * Depth));
}