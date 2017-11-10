#include "../inc/gpu_defines.inc"

struct PointLight
{
	float3 color;
	float intensity;
	//16 bytes
	float3 position;
	float paddingPL;
	//32 bytes
};//------------------------32 bytes

struct DirectionalLight
{
	float3 color;
	float intensity;
	//16 bytes
	float3 direction;
	float paddingDL;
	//32 bytes
	//float4 paddingDL[14];
	//matrix lightProjectionMatrix;
	////160 bytes x.x
};

struct LightResult
{
	float3 diffuse;
	float3 specular;
};

cbuffer PointLights : register(b1)
{
	uint pointLightCount;
	float3 paddingPL;
	//16 bytes
	PointLight pointLights[MAX_POINT_LIGHTS];
}

cbuffer DirectionalLights : register(b2)
{
	uint directionalLightCount;
	float3 paddingDL;
	//16 bytes
	DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
}

Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);

SamplerState anisotropicSampler : register(s0);

struct PixelShaderInput
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

struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
};

float3 expand(float3 v) { return (v * 2.0f) - 1.0f; }

float CalcFallOffCoefficient(float l)
{
	return 1 / (1 + 3.0f * l * l);
}

LightResult GetPointLightContribution(PointLight light, float3 pixelPosition, float3 normal, float3 viewDir)
{
	float3 lightPos = light.position;//mul(view, float4(light.position, 1.0f)).xyz;
	float3 lightDir = normalize(lightPos - pixelPosition);
	float3 reflectDir = reflect(-lightDir, normal);

	float fallOff = CalcFallOffCoefficient(length(lightPos - pixelPosition));

	float diff = max(0, dot(normal, lightDir));
	float3 diffuse = light.color * light.intensity * diff * fallOff;

	float spec = pow(max(dot(viewDir, reflectDir), 0), 2048.0f); //2048
	float3 specular = float3(0, 0, 0);
	if (any(diffuse))
	{
		specular = light.color * spec;
	}

	//float3 specular = light.color * light.intensity * spec * fallOff;

	LightResult OUT;
	OUT.diffuse = diffuse;
	OUT.specular = specular;
	return OUT;
}

LightResult GetDirectionalLightContribution(DirectionalLight light, float3 pixelPosition, float3 normal, float3 viewDir)
{
	float3 lightDir = -light.direction;//mul(view, float4(-light.direction, 0.0f)).xyz;
	float3 reflectDir = reflect(-lightDir, normal);

	float diff = max(0, dot(normal, lightDir));
	float3 diffuse = light.color * diff;

	float spec = pow(max(dot(viewDir, reflectDir), 0), 2048.0f); //2048
	float3 specular = float3(0, 0, 0);
	if (any(diffuse))
	{
		specular = light.color * spec;
	}

	LightResult OUT;
	OUT.diffuse = diffuse;
	OUT.specular = specular;
	return OUT;
}

PixelShaderOutput main(PixelShaderInput IN) //: SV_TARGET
{
	float3 lightPos = (float3)0;// = lights[0].position;
	float3 lightColor = (float3)0;// = lights[0].color;
	float4 totalLightContribution = (float4)0;

	float3 normalTex = float3(0, 0, 0);// normalTexture.Sample(anisotropicSampler, IN.uv).xyz;
	float4 albedo = float4(0.8f, 0.8f, 0.8f, 0.0f);// albedoTexture.Sample(anisotropicSampler, IN.uv);

	float3 viewDir = normalize(IN.cameraPosition - IN.positionWS);
	float specular = 0;
	//float4 ambient = float4(0.2f, 0.2f, 0.2f, 0.0f);
	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float3x3 tangentMatrix = float3x3(IN.tangentWS, IN.binormalWS, IN.normalWS);
	float3 normal = IN.normal;
	if (any(normalTex))
	{
		normal = expand(normalTex);
		normal = mul(normal, tangentMatrix);
	}

	normal = normalize(normal);

	for (uint i = 0; i < pointLightCount; ++i)
	{
		LightResult contribution = GetPointLightContribution(pointLights[i], IN.positionWS, normal, viewDir);
		totalLightContribution += float4((contribution.diffuse + contribution.specular), 1.0f);
	}

	for (uint j = 0; j < directionalLightCount; ++j)
	{
		LightResult contribution = GetDirectionalLightContribution(directionalLights[j], IN.positionWS, normal, viewDir);
		totalLightContribution += float4((contribution.diffuse + contribution.specular), 1.0f);
	}

	PixelShaderOutput OUT;
	OUT.color = (totalLightContribution + ambient) * albedo;

	return OUT;
}
