#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_bindless_texture : enable

layout(location = 0) uniform mat4 projection;
layout(location = 1) uniform mat4 view;

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;
layout(location = 3) in vec3 in_Tangent;
layout(location = 4) in mat4 in_model;

out gl_PerVertex {
    vec4 gl_Position;
};

out VS_out {
    mat3 TBN;
    vec3 FragPos;
    // vec3 Normal;
    vec2 TexCoord;
    flat uint drawID;
}
vs_out;

void main() {
    vec4 worldPos = in_model * vec4(in_Position, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(in_model)));

    vec3 T = normalize(normalMatrix * in_Tangent);
    vec3 N = normalize(normalMatrix * in_Normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    vs_out.TBN = mat3(T, B, N);
    vs_out.FragPos = worldPos.xyz;
    // vs_out.Normal = normalMatrix * in_Normal;
    vs_out.TexCoord = in_TexCoord;
    vs_out.drawID = gl_DrawID;

    gl_Position = projection * view * worldPos;
}