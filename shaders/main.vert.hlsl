#include "vertexinput.hlsli"
#include "pixelshader.hlsli"
#include "ubo.hlsli"


PixelShaderInput main(VertexInput vi)
{
    PixelShaderInput psi;
    float4x4 MVP = model * view * proj;
    psi.scrPos = float4(vi.inPosition,1) * MVP;

    float4 inNormalZ = float4(vi.inNormal,0) * modelInvDual;
    psi.inNormal = inNormalZ.xyz;
    psi.outTexCoord = vi.texCoord;
    return psi;
}