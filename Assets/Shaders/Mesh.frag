#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_bindless_texture : enable

const float PI = 3.14159265359;

struct PBRMetallicRoughnessMaterial {
    vec4 baseColor;
    float metallicFactor;
    float roughnessFactor;
    uint baseColorTexture;
    uint metallicRoughnessTexture;
};

struct Material {
    PBRMetallicRoughnessMaterial pbrMetallicRoughness;
    uint normalTexture;
    uint occlusionTexture;
    uint emissiveTexture;
    float padding;
    vec3 emissiveFactor;
    float emissiveStrength;
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

// IBL
layout(location = 2) uniform bool computeIBL;
layout(binding = 10) uniform samplerCube irradianceMap;
layout(binding = 11) uniform samplerCube prefilterMap;
layout(binding = 12) uniform sampler2D brdfLUT;

in VS_out {
    mat3 TBN;
    vec3 FragPos;
    // vec3 Normal;
    vec2 TexCoord;
    flat uint drawID;
}
fs_in;

layout(location = 0) out vec4 FragColor;

vec3 getNormalFromMap(vec3 worldPos, vec3 normal, uint material_index) {
    sampler2D normalMap = sampler2D(textureHandles[materials[material_index].normalTexture]);
    vec3 tangentNormal = texture(normalMap, fs_in.TexCoord).xyz * 2.0 - 1.0;

    vec3 Q1 = dFdx(worldPos);
    vec3 Q2 = dFdy(worldPos);
    vec2 st1 = dFdx(fs_in.TexCoord);
    vec2 st2 = dFdy(fs_in.TexCoord);

    vec3 N = normalize(normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return max(F0 + (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta), 0.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalculateDirectionalLightRadiance(Light light, vec3 albedo, float metallic, float roughness, vec3 F0, vec3 fragPos, vec3 N, vec3 V) {
    vec3 L = normalize(-light.position);
    vec3 H = normalize(V + L);
    vec3 radiance = light.color * light.intensity;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 CalculatePointLightRadiance(vec3 albedo, float metallic, float roughness, vec3 F0, vec3 fragPos, vec3 N, vec3 V) {
    vec3 Lo = vec3(0.0);

    return Lo;
}

vec3 sRGB_to_Linear(vec3 sRGBColor) {
    // Apply the inverse gamma correction for each channel
    return mix(pow(sRGBColor.rgb * 0.9478672986 + 0.0521327014, vec3(2.4)), sRGBColor.rgb * 0.04045 / 12.92,
        lessThanEqual(sRGBColor.rgb, vec3(0.04045)));
    // return pow(sRGBColor, vec3(2.2));
}

void main() {
    uint material_index = drawables[fs_in.drawID].x;

    sampler2D baseColorMap = sampler2D(textureHandles[materials[material_index].pbrMetallicRoughness.baseColorTexture]);
    sampler2D metallicRoughnessMap = sampler2D(textureHandles[materials[material_index].pbrMetallicRoughness.metallicRoughnessTexture]);
    sampler2D occlusionMap = sampler2D(textureHandles[materials[material_index].occlusionTexture]);
    sampler2D emissiveMap = sampler2D(textureHandles[materials[material_index].emissiveTexture]);

    vec3 albedo = texture(baseColorMap, fs_in.TexCoord).rgb;
    float roughness = texture(metallicRoughnessMap, fs_in.TexCoord).g;
    float metallic = texture(metallicRoughnessMap, fs_in.TexCoord).b;
    float occlusion = texture(occlusionMap, fs_in.TexCoord).r;
    vec3 emission = materials[material_index].emissiveFactor * sRGB_to_Linear(texture(emissiveMap, fs_in.TexCoord).rgb)
        * materials[material_index].emissiveStrength;

    // vec3 N = normalize(fs_in.Normal);
    // vec3 N = getNormalFromMap(fs_in.FragPos, fs_in.Normal, material_index);
    sampler2D normalMap = sampler2D(textureHandles[materials[material_index].normalTexture]);
    vec3 tangentNormal = texture(normalMap, fs_in.TexCoord).xyz * 2.0 - 1.0;

    vec3 N = normalize(fs_in.TBN * tangentNormal);
    vec3 V = normalize(viewPos - fs_in.FragPos);
    vec3 R = reflect(-V, N);

    vec3 L = normalize(-lights[0].position);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    Lo += CalculateDirectionalLightRadiance(lights[0], albedo, metallic, roughness, F0, fs_in.FragPos, N, V);
    Lo += CalculatePointLightRadiance(albedo, metallic, roughness, F0, fs_in.FragPos, N, V);

    vec3 Ia = vec3(0.05) * albedo * occlusion;
    if (computeIBL) {
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;

        vec3 Id = texture(irradianceMap, N).rgb * albedo * kD;

        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;

        vec3 Is = prefilteredColor * (F * brdf.x + brdf.y);

        Ia = (Id + Is) * occlusion;
    }

    // FragColor = vec4(albedo, 1.0);
    // FragColor = vec4(vec3(roughness), 1.0);
    // FragColor = vec4(vec3(metallic), 1.0);
    // FragColor = vec4(vec3(occlusion), 1.0);
    // FragColor = vec4(N, 1.0);
    // FragColor = vec4(F0, 1.0);
    // FragColor = vec4(Lo, 1.0);
    // FragColor = vec4(Ia, 1.0);
    FragColor = vec4(Lo + Ia + emission, 1.0);
}