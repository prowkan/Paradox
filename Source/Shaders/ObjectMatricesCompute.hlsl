struct CSCameraConstants
{
	float4x4 ViewProjMatrix;
};

struct InputObjectMatrices
{
	float4x4 WorldMatrix;
	float3x4 VectorTransformMatrix;
};

struct OutputObjectMatrices
{
	float4x4 WVPMatrix;
	float4x4 WorldMatrix;
	float3x4 VectorTransformMatrix;
	float4 Pad[5];
};

ConstantBuffer<CSCameraConstants> ComputeShaderCameraConstants : register(b0);
StructuredBuffer<InputObjectMatrices> AllObjectsDataBuffer : register(t0);
Buffer<uint> VisibleObjectsIndicesBuffer : register(t1);
RWStructuredBuffer<OutputObjectMatrices> VisibleObjectsDataBuffer : register(u0);

[numthreads(256, 1, 1)]
void CS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	uint ObjectIndex = VisibleObjectsIndicesBuffer.Load(DispatchThreadID.x);

	InputObjectMatrices inputObjectMatrices = AllObjectsDataBuffer.Load(ObjectIndex);

	OutputObjectMatrices outputObjectMatrices = (OutputObjectMatrices)0;

	outputObjectMatrices.WVPMatrix = mul(inputObjectMatrices.WorldMatrix, ComputeShaderCameraConstants.ViewProjMatrix);
	outputObjectMatrices.WorldMatrix = inputObjectMatrices.WorldMatrix;
	outputObjectMatrices.VectorTransformMatrix = inputObjectMatrices.VectorTransformMatrix;

	VisibleObjectsDataBuffer[DispatchThreadID.x] = outputObjectMatrices;
}