#version 450 core
#extension GL_ARB_separate_shader_objects : enable

out VS_out {
    vec2 TexCoord;
}
vs_out;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 vertices[4] = vec4[4](vec4(-1.0, -1.0, 0.0, 1.0), vec4(1.0, -1.0, 0.0, 1.0), vec4(-1.0, 1.0, 0.0, 1.0), vec4(1.0, 1.0, 0.0, 1.0));
    vec2 texCoord[4] = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));

    gl_Position = vertices[gl_VertexID];
    vs_out.TexCoord = texCoord[gl_VertexID];
}