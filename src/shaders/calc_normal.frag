#version 440 core

uniform sampler2D depthTex;
uniform sampler2D idTex;
uniform vec2 iResolution;
uniform mat4 invProj;
uniform mat4 view;

in vec2 texCoords;
layout(location = 0) out vec4 FragColor;


vec3 getPos(ivec2 fragCoord, float depth) {
    if (depth == 1.0) return vec3(0.0); // background
    float id = texelFetch(idTex, fragCoord, 0).r;
    if (id == -1) discard; // background

    vec2 ndc = (vec2(fragCoord) / iResolution) * 2.0 - 1.0;
    float ndcDepth = depth * 2.0 - 1.0; // FIXED
    vec4 clipSpace = vec4(ndc, ndcDepth, 1.0);
    vec4 viewSpace = invProj * clipSpace;
    viewSpace /= viewSpace.w;
    return viewSpace.xyz;
}

vec3 computeNormalNaive(const sampler2D depthTex, ivec2 p) {
    ivec2 left   = clamp(p - ivec2(1, 0), ivec2(0), textureSize(depthTex, 0) - ivec2(1));
    ivec2 right  = clamp(p + ivec2(1, 0), ivec2(0), textureSize(depthTex, 0) - ivec2(1));
    ivec2 top    = clamp(p + ivec2(0, 1), ivec2(0), textureSize(depthTex, 0) - ivec2(1));
    ivec2 bottom = clamp(p - ivec2(0, 1), ivec2(0), textureSize(depthTex, 0) - ivec2(1));

    float dCenter = texelFetch(depthTex, p, 0).r;
    if (dCenter == 1.0) discard; // background

    vec3 l1 = getPos(left,   texelFetch(depthTex, left,   0).r);
    vec3 r1 = getPos(right,  texelFetch(depthTex, right,  0).r);
    vec3 t1 = getPos(top,    texelFetch(depthTex, top,    0).r);
    vec3 b1 = getPos(bottom, texelFetch(depthTex, bottom, 0).r);

    vec3 dpdx = r1 - l1;
    vec3 dpdy = t1 - b1;

    return normalize(cross(dpdx, dpdy));
}

void main() {
    ivec2 fragCoord = ivec2(gl_FragCoord.xy);
    vec3 normal = computeNormalNaive(depthTex, fragCoord);

    // convert to world space
    mat3 normalMatrix = mat3(transpose(inverse(view)));
    vec3 normal_world = normalize(normalMatrix * normal);

    // Store as RGB color (or output to float32 texture)
    FragColor = vec4(normal_world * 0.5 + 0.5, 1.0);
}
