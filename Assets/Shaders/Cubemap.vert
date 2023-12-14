#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 a_Position;

layout(location = 1) uniform mat4 projection;
layout(location = 2) uniform mat4 view;

out VS_out {
    vec3 FragPos;
}
vs_out;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vs_out.FragPos = a_Position;
    gl_Position = projection * view * vec4(a_Position, 1.0);
}