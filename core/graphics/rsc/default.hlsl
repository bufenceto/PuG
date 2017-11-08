struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
};

VS_OUTPUT VSMain()
{
	VS_OUTPUT output;
	output.Pos = float4(0.0f, 0.0f, 0.0f, 0.0f);

	return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}