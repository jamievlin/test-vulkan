#include "pixelshader.hlsli"
#include "ubo.hlsli"
#include "lights.hlsli"
#include "meshubo.hlsli"


struct PixelShaderOutput
{
    [[vk::location(0)]]
    float4 fragColor : SV_TARGET;
};

float Fresnel(float ndoth)
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
    return baseColor.rgb + (specularVal * float3(1,1,1));
}

float3 compute_light_point(float3 viewVec, float3 normal, float3 worldPosition, Light lig)
{
    float3 lightDistance = lig.position.xyz - worldPosition;
    float attenuation = rcp(dot(lightDistance, lightDistance));

    float3 lightColor = lig.intensity * lig.color.rgb * attenuation;
    float lightDirDot = saturate(dot(normalize(lightDistance), normal));
    return max(BRDF(viewVec, normalize(lightDistance), normal) * lightColor * lightDirDot, 0);
}

float3 compute_light_dir(float3 viewVec, float3 normal, Light lig)
{
    float3 lightDistance = -normalize(lig.position.xyz);
    float3 lightColor = lig.intensity * lig.color.rgb;
    float lightDirDot = saturate(dot(lightDistance, normal));
    return max(BRDF(viewVec, lightDistance, normal) * lightColor * lightDirDot, 0);
}

PixelShaderOutput main(PixelShaderInput psi)
{
    PixelShaderOutput pso;
    float3 normal = normalize(psi.inNormal);
    float3 viewVec = normalize(cameraPos.xyz - psi.worldPos);
    float3 outColor = float3(0,0,0);

    float4 smapPos = mul(lightDirMatrix, float4(psi.worldPos, 1));
    float2 smapCoord = smapPos.xy / smapPos.w;
    smapCoord = 0.5f * (smapCoord + float2(1,1));
    float sampledVal = depthMap.Sample(depthMapSampler, smapCoord).r;

    float hit = smapPos.z / smapPos.w <= sampledVal + 0.0001 ? 1.f : 0.f;

    for (uint i=0; i < lightCount; ++i)
    {
        [branch] switch(lightsObj[i].lightType)
        {
            case POINT_LIGHT:
                outColor += compute_light_point(viewVec, normal, psi.worldPos, lightsObj[i]);
            break;

            case DIRECTIONAL_LIGHT:
                outColor += hit * compute_light_dir(viewVec, normal, lightsObj[i]);
            break;
        }
    }

    pso.fragColor = saturate(float4(outColor, 1)); // float4(multValue * psi.inColor,1);
    return pso;
}