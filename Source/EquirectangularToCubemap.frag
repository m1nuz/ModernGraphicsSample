#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D equirectangularMap;

in VS_out {
    vec3 FragPos;
}
fs_in;

layout(location = 0, index = 0) out vec4 FragColor;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = SampleSphericalMap(normalize(fs_in.FragPos));
    vec3 color = texture(equirectangularMap, uv).rgb;

    FragColor = vec4(color, 1.0);
}