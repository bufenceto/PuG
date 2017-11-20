struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
	float2 uv  : TEXCOORD;
};

Texture2D fontTexture : register(t0);
SamplerState linearSampler : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 tex_col = fontTexture.Sample(linearSampler, input.uv).xxxx;
	float4 out_col = input.col * tex_col;
	return out_col;
}