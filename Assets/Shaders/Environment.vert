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

    mat4 rotView = mat4(mat3(view));
    vec4 clipPos = projection * rotView * vec4(vs_out.FragPos, 1.0);

    gl_Position = clipPos.xyww;
}