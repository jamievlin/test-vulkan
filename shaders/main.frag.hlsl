#include "pixelshader.hlsli"

struct PixelShaderOutput
{
    [[vk::location(0)]]
    float4 fragColor : SV_TARGET;
};

[[vk::binding(0)]]
cbuffer UBO
{
    float time;
    float4x4 proj;
    float4x4 view;
    float4x4 model;
};

[[vk::binding(1)]]
Texture2D<float4> tex;

[[vk::binding(1)]]
SamplerState sLinear;

PixelShaderOutput main(PixelShaderInput psi)
{
    PixelShaderOutput pso;
    float multValue = 0.5 + (sin(1.1 * time)) * 0.5;
    pso.fragColor = tex.SampleLevel(sLinear, psi.outTexCoord, 0); // float4(multValue * psi.inColor,1);
    return pso;
}