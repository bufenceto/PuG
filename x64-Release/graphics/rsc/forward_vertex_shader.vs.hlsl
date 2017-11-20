cbuffer PerObject : register(b0)
{
	matrix worldMatrix;
	matrix vpMatrix;
	matrix inverseTransposeWorldMatrix;
	//-----------------------------------------------
	float3 cameraPosition;
	float padding1;
	//-----------------------------------------------
	float3 aabbColor;
	uint drawCallIndex;
	//-----------------------------------------------
	uint numDrawCalls;
	float3 padding2;
	//-----------------------------------------------
	float4 padding3;
};//64 floats : 256 bytes

static float3 triangleVertices[] =
{
    float3(  0.0f, 0.25f, 0.0f),
	float3( 0.25f,-0.25f, 0.0f),
	float3(-0.25f,-0.25f, 0.0f)
};

struct AppData
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent: TANGENT;
	float2 uv : UV;
};

struct VertexShaderOutput
{
	float4 position : SV_POSITION;
};

VertexShaderOutput main(AppData IN, uint vertexID : SV_VertexID)
{
	VertexShaderOutput OUT;

	matrix mvp = mul(vpMatrix, worldMatrix);
	OUT.position = mul(mvp, float4(triangleVertices[vertexID], 1.0f));

	return OUT;
}
