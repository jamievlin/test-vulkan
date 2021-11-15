#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1


struct Light
{
    uint lightType;
    float intensity;

    float4 color;
    float4 position;
    float4 parameters;
};

[[vk::binding(0,2)]]
tbuffer lights
{
    uint lightCount;
    Light lightsObj[];
};