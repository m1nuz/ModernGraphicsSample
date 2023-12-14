#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out VS_out {
    vec2 TexCoord;
}
vs_out;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = vec4(a_Position, 1.0);
    vs_out.TexCoord = a_TexCoord;
}