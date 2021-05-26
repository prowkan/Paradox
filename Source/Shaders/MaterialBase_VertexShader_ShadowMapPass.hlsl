struct VSInput
{
	float3 Position : POSITION;
};

struct VSOutput
{
	float4 Position : SV_Position;
};

struct VSCameraConstants
{
	float4x4 ViewProjMatrix;
};

struct VSObjectConstants
{
	float4x4 WorldMatrix;
	float3x3 VectorTransformMatrix;
};

cbuffer DrawData : register(b0)
{
	uint4 DataIndices0;
	uint DataIndices1;
}

VSOutput VS(VSInput VertexShaderInput)
{
	VSOutput VertexShaderOutput;

	ConstantBuffer<VSCameraConstants> VertexShaderCameraConstants = ResourceDescriptorHeap[DataIndices0.x];
	ConstantBuffer<VSObjectConstants> VertexShaderObjectConstants = ResourceDescriptorHeap[DataIndices0.y];

	float4x4 WVPMatrix = mul(VertexShaderObjectConstants.WorldMatrix, VertexShaderCameraConstants.ViewProjMatrix);

	VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), WVPMatrix);

	//VertexShaderOutput.Position = float4(VertexShaderInput.Position, 1.0f);

	return VertexShaderOutput;
}