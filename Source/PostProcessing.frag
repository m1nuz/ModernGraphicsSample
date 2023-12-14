#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D colorTexture;

layout(location = 1) uniform float exposure = 1.0;
layout(location = 2) uniform float gamma = 2.2;

in VS_out {
    vec2 TexCoord;
}
fs_in;

layout(location = 0) out vec4 FragColor;

vec3 ChromaticAberation() {
    float u_CAScale = 0.025;

    const float ChannelCount = 3.0f;
    float AberationScale = mix(0.0f, 0.5f, u_CAScale);
    vec2 DistanceWeight = fs_in.TexCoord - 0.5f;
    vec2 Aberrated = AberationScale * pow(DistanceWeight, vec2(3.0f, 3.0f));
    vec3 Final = vec3(0.0f);
    float TotalWeight = 0.01f;

    int Samples = 7;

    bool u_FXAAEnabled = true;

    vec3 CenterSample = texture(colorTexture, fs_in.TexCoord).xyz;

    for (int i = 1; i <= Samples; i++) {
        float wg = 1.0f / pow(2.0f, float(i));

        float x = 0.0f;

        if (fs_in.TexCoord - float(i) * Aberrated == clamp(fs_in.TexCoord - float(i) * Aberrated, 0.0001f, 0.9999f)) {
            Final.r += texture(colorTexture, fs_in.TexCoord - float(i) * Aberrated).r * wg;
        }

        else {
            Final.r += CenterSample.x * wg;
        }

        if (fs_in.TexCoord + float(i) * Aberrated == clamp(fs_in.TexCoord + float(i) * Aberrated, 0.0001f, 0.9999f)) {
            Final.b += texture(colorTexture, fs_in.TexCoord + float(i) * Aberrated).b * wg;
        }

        else {
            Final.b += CenterSample.y * wg;
        }

        TotalWeight += wg;
    }

    TotalWeight = 0.9961f; //(1.0 / pow(2.0f, float(i)) i = 1 -> 8
    Final.g = CenterSample.g;
    return max(Final, 0.0f);
}

vec3 Uncharted2Tonemap(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 Uncharted2Filmic(vec3 v) {
    float exposure_bias = 2.0;
    vec3 curr = Uncharted2Tonemap(v * exposure_bias);

    vec3 W = vec3(11.2);
    vec3 white_scale = vec3(1.0) / Uncharted2Tonemap(W);
    return curr * white_scale;
}

float vignette() {
    float intensity = 1.0 / 16.0;
    vec2 uv = fs_in.TexCoord;
    uv *= 1.0 - fs_in.TexCoord.xy;
    float vig = uv.x * uv.y * 15.0;
    vig = pow(vig, intensity * 5.0);

    return vig;
}

void main() {

    // vec3 color = texture(colorTexture, fs_in.TexCoord).rgb;
    vec3 color = ChromaticAberation();

    color = color * vec3(vignette());
    // color = Uncharted2Filmic(color);
    color = vec3(1.0) - exp(-color * exposure);
    color = pow(color, vec3(1.0 / gamma));

    FragColor = vec4(color, 1.0);
}