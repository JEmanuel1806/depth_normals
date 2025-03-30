#version 440 core

uniform sampler2D depthTex;
uniform sampler2D idTex;
uniform vec2 iResolution;
in vec2 texCoords;
out vec4 FragColor;

uniform mat4 invProj;  // inverse prjection matrix for reconstructing view-space position
uniform mat4 view;     // view matrix to convert normals to world space

// convert depth + screen space coordinate to 3D position in view space
vec3 getPos(ivec2 fragCoord, float depth) {
    vec2 ndc = (vec2(fragCoord) / iResolution) * 2.0 - 1.0;
    vec4 clipSpace = vec4(ndc, depth, 1.0);
    vec4 viewSpace = invProj * clipSpace;
    viewSpace /= viewSpace.w;
    return viewSpace.xyz;
}

// calculate surface normal using central difference method (simple and fast)
vec3 computeNormalNaive(const sampler2D depthTex, ivec2 p) {
    ivec2 left   = clamp(p - ivec2(1, 0), ivec2(0), textureSize(depthTex, 0) - ivec2(1));
    ivec2 right  = clamp(p + ivec2(1, 0), ivec2(0), textureSize(depthTex, 0) - ivec2(1));
    ivec2 top    = clamp(p + ivec2(0, 1), ivec2(0), textureSize(depthTex, 0) - ivec2(1));
    ivec2 bottom = clamp(p - ivec2(0, 1), ivec2(0), textureSize(depthTex, 0) - ivec2(1));

    vec3 l1 = getPos(left,   texelFetch(depthTex, left,   0).r);
    vec3 r1 = getPos(right,  texelFetch(depthTex, right,  0).r);
    vec3 t1 = getPos(top,    texelFetch(depthTex, top,    0).r);
    vec3 b1 = getPos(bottom, texelFetch(depthTex, bottom, 0).r);

    vec3 dpdx = r1 - l1;
    vec3 dpdy = t1 - b1;

    return normalize(cross(dpdx, dpdy)); // normal in view space
}

void main() {
    ivec2 fragCoord = ivec2(gl_FragCoord.xy);
    vec3 normal = computeNormalNaive(depthTex, fragCoord);

    mat3 normalMatrix = mat3(transpose(inverse(view)));
    vec3 normal_world = normalize(normalMatrix * normal);

    // encode normal as RGB color for visualization
    vec3 color = normal_world * 0.5 + 0.5;
    FragColor = vec4(color, 1.0);
}
