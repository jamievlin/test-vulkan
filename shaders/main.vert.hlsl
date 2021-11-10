#include "vertexinput.hlsli"
#include "pixelshader.hlsli"
#include "ubo.hlsli"


PixelShaderInput main(VertexInput vi)
{
    PixelShaderInput psi;
    float4 inPos4 = float4(vi.inPosition,1);

    float4x4 MVP = mul(proj, mul(view, model));
    psi.scrPos = mul(MVP, inPos4);

    float4 inNormalZ = mul(modelInvDual, float4(vi.inNormal,0));
    psi.inNormal = inNormalZ.xyz;
    psi.outTexCoord = vi.texCoord;
    psi.worldPos = mul(model, inPos4).xyz;
    return psi;
}