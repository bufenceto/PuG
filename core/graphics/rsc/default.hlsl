struct VS_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Color : COLOR;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Pos = float4(input.position.xyz, 1.0f);
	output.Color = input.color;

	return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
	return input.Color;
}