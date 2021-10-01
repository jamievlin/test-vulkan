#include "pixelshader.hlsli"
#include "ubo.hlsli"

struct PixelShaderOutput
{
    [[vk::location(0)]]
    float4 fragColor : SV_TARGET;
};

[[vk::binding(1)]]
Texture2D<float4> tex;

[[vk::binding(1)]]
SamplerState sLinear;

PixelShaderOutput main(PixelShaderInput psi)
{
    float3 normal = normalize(psi.inNormal);
    PixelShaderOutput pso;
    float multValue = 0.5 + (sin(1.1 * time)) * 0.5;
    pso.fragColor = float4(normal,1); // float4(multValue * psi.inColor,1);
    return pso;
}