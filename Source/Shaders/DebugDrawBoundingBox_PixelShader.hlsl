struct PSInput
{
	float4 Position : SV_Position;
};

float4 PS(PSInput PixelShaderInput) : SV_Target
{
	return float4(0.0f, 1.0f, 0.0f, 1.0f);
}