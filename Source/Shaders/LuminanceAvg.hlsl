struct PSInput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
}; 

Texture2D<float> SummedLuminanceTexture : register(t0);

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	return float4(SummedLuminanceTexture[int2(0, 0)] / float(1280 * 720), 0.0f, 0.0f, 0.0f);
}