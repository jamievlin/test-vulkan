#include "vertexinput.hlsli"
#include "pixelshader.hlsli"

[[vk::binding(0,0)]]
cbuffer UBO
{
    float time;
    float4x4 proj;
    float4x4 view;
    float4x4 model;
};

PixelShaderInput main(VertexInput vi)
{
    PixelShaderInput psi;
    psi.scrPos = float4(vi.inPosition,1) * model * view * proj;
    psi.inColor = vi.inColor;
    psi.outTexCoord = vi.texCoord;
    return psi;
}