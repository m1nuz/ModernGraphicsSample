#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform samplerCube environmentMap;

in VS_out {
    vec3 FragPos;
}
fs_in;

layout(location = 0, index = 0) out vec4 FragColor;

void main() {
    vec3 envColor = textureLod(environmentMap, fs_in.FragPos, 0.0).rgb;

    FragColor = vec4(envColor, 1.0);
    // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}