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
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	float3 cameraPosition : CAM_POS_WS;
	float3 positionWS : POSITION_WS;
	float3 normalWS : NORMAL_WS;
	float3 tangentWS : TANGENT_WS;
	float3 binormalWS : BINORMAL_WS;
};

VertexShaderOutput main(AppData IN)
{
	VertexShaderOutput OUT;

	matrix mvp = mul(vpMatrix, worldMatrix);
	OUT.position = mul(mvp, float4(IN.position, 1.0f));
	OUT.normal = mul(worldMatrix, float4(IN.normal, 0.0f)).xyz;//output normal
	OUT.uv = IN.uv;
	OUT.cameraPosition = cameraPosition;
	OUT.positionWS = mul(worldMatrix, float4(IN.position, 1.0f)).xyz;

	float3 normalWS = mul(inverseTransposeWorldMatrix, float4(IN.normal, 0.0f)).xyz;
	float3 tangentWS = mul(inverseTransposeWorldMatrix, float4(IN.tangent, 0.0f)).xyz;
	float3 binormalWS = cross(tangentWS, normalWS).xyz;//TODO check if this is the correct order

	OUT.normalWS = normalWS;
	OUT.tangentWS = -tangentWS;
	OUT.binormalWS = binormalWS;

	return OUT;
}
