#include "pixelshader.hlsli"
#include "ubo.hlsli"
#include "lights.hlsli"

struct PixelShaderOutput
{
    [[vk::location(0)]]
    float4 fragColor : SV_TARGET;
};

[[vk::binding(1)]]
Texture2D<float4> tex;

[[vk::binding(1)]]
SamplerState sLinear;

float Fresnel(float3 ndoth)
{
    // schlick's approximation
    return f0 + (1-f0) * pow(1-ndoth, 5);
}

float DBeckmann(float ndoth, float r2)
{
    float ndoth2 = ndoth * ndoth;
    float r4 = r2 * r2;
    float num = exp((ndoth2 - 1) / (r4 * ndoth2));
    float denom = r4 * ndoth2 * ndoth2;
    return num * rcp(denom);
}

float GValdenom(float ndotV, float r2)
{
    float k = (r2 + 1) * (r2 + 1)/8;
    return ndotV * (1-k) + k;
}

float GVal(float ndotv, float ndotl, float r2)
{
    return rcp(GValdenom(ndotv,r2)*GValdenom(ndotl,r2));
}

float3 BRDF(float3 viewDir, float3 lightDir, float3 normal)
{
    float roughness2 = roughness * roughness;
    float3 halfvec = normalize(viewDir + lightDir);

    float ndotl = saturate(dot(normal, lightDir));
    float ndotv = saturate(dot(normal, viewDir));
    float ndoth = saturate(dot(normal, halfvec));

    float specularVal = DBeckmann(ndoth, roughness2) * Fresnel(ndoth) * GVal(ndotv, ndotl, roughness2) * 0.25;
    return baseColor + specularVal;
}

float3 compute_light_point(float3 viewVec, float3 normal, float3 worldPosition, Light lig)
{
    float3 lightDistance = lig.position.xyz - worldPosition;
    float attenuation = rcp(dot(lightDistance, lightDistance));

    float3 lightColor = lig.intensity * lig.color.rgb * attenuation;

    float lightDirDot = saturate(dot(normalize(lightDistance), normal));
    return BRDF(viewVec, normalize(lightDistance), normal) * lightColor * lightDirDot;
}

float3 compute_light_dir(float3 viewVec, float3 normal, Light lig)
{
    float3 lightDistance = normalize(lig.position.xyz);
    float3 lightColor = lig.intensity * lig.color.rgb;
    float lightDirDot = saturate(dot(lightDistance, normal));
    return BRDF(viewVec, lightDistance, normal) * lightColor * lightDirDot;
}

PixelShaderOutput main(PixelShaderInput psi)
{
    PixelShaderOutput pso;
    float3 normal = normalize(psi.inNormal);
    float3 viewVec = normalize(cameraPos.xyz - psi.worldPos);

    float3 outColor = float3(0,0,0);
    for (uint i=0; i < lightCount; ++i)
    {
        [branch] switch(lightsObj[i].lightType)
        {
            case POINT_LIGHT:
                outColor += compute_light_point(viewVec, normal, psi.worldPos, lightsObj[i]);
            break;

            case DIRECTIONAL_LIGHT:
                outColor += compute_light_dir(viewVec, normal, lightsObj[i]);
            break;
        }
    }

    pso.fragColor = float4(outColor, 1); // float4(multValue * psi.inColor,1);
    return pso;
}