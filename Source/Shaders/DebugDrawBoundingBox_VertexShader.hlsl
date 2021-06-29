struct VSConstants
{
	float4x4 WVPMatrix;
	float3 Positions[8];
};

ConstantBuffer<VSConstants> VertexShaderConstants : register(b0);

float4 VS(uint VertexID : SV_VertexID) : SV_Position
{
	return mul(float4(VertexShaderConstants.Positions[VertexID], 1.0f), VertexShaderConstants.WVPMatrix);
}