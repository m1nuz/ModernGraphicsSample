#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_bindless_texture : enable

struct Material {
    uint Kd;
    uint Ks;
    float Ns;
    float d;
    uint MapKd;
};

struct Light {
    vec3 position;
    float intensity;
    vec3 color;
    float radius;
};

layout(std430, binding = 3) readonly buffer DrawablesBlock {
    uvec2 drawables[];
};

layout(std430, binding = 4) readonly buffer MaterialBlock {
    Material materials[];
};

layout(std430, binding = 5) readonly buffer TextureHandleBlock {
    uvec2 textureHandles[];
};

layout(std430, binding = 7) readonly buffer LightBuffer {
    Light lights[];
};

layout(std430, binding = 8) readonly buffer LightIndicesBlock {
    uint lightIndices[];
};

layout(location = 1) uniform vec3 viewPos;

in VS_out {
    smooth vec3 FragPos;
    smooth vec3 Normal;
    smooth vec2 TexCoord;
    flat uint drawID;
}
fs_in;

layout(location = 0) out vec4 FragColor;

void main() {
    uint material_index = drawables[fs_in.drawID].x;

    sampler2D MapKd = sampler2D(textureHandles[materials[material_index].MapKd]);

    vec3 N = normalize(fs_in.Normal);
    vec3 L = normalize(-lights[0].position);
    vec3 R = reflect(-L, N);
    vec3 V = normalize(viewPos - fs_in.FragPos);

    float NdotL = max(dot(N, L), 0.0);

    vec3 Kd = unpackUnorm4x8(materials[material_index].Kd).rgb * texture(MapKd, fs_in.TexCoord).rgb;
    vec3 Ks = unpackUnorm4x8(materials[material_index].Ks).rgb;
    float Ns = clamp(materials[material_index].Ns, 1.0, 1000.0);
    float d = clamp(materials[material_index].Ns, 0.0, 1.0);

    vec3 Ia = lights[0].color * lights[0].intensity * vec3(0.3);
    vec3 Id = lights[0].color * lights[0].intensity * NdotL;
    vec3 Is = lights[0].color * lights[0].intensity * max(0, pow(max(0, dot(R, V)), Ns));

    FragColor = vec4((Ia + Id) * Kd + Is * Ks, d);
}