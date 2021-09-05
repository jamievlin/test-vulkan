#version 460
#extension GL_ARB_separate_shader_objects : enable

// layout(location=0)
// in vec2 position;

vec2 pos[3] = vec2[](
    vec2(0.0,0.5),
    vec2(0.5,0.0),
    vec2(0.5,0.5)
);

void main()
{
    gl_Position=vec4(pos[gl_VertexIndex],0,1);
}