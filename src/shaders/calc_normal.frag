#version 440 core

uniform sampler2D depthTex;
uniform isampler2D idTex;
uniform vec2 iResolution;
uniform mat4 invProj;
uniform mat4 view;

uniform float zNear;
uniform float zFar;

in vec2 texCoords;

layout(location = 0) out vec4 FragColor;

vec3 getPos(ivec2 fragCoord, float depth) {
    
    //if (depth == 1.0) return vec3(0.0); // background
    
    float id = texelFetch(idTex, fragCoord, 0).r;
    //if (id == -1) discard; // background

    vec2 ndc = (vec2(fragCoord) / iResolution) * 2.0 - 1.0;
    float ndcDepth = depth * 2.0 - 1.0; 
    
    vec4 clipSpace = vec4(ndc, ndcDepth, 1.0);
    vec4 viewSpace = invProj * clipSpace;
    viewSpace /= viewSpace.w;
    return viewSpace.xyz;
}

// linearize depth because depth buffer precision
float linearizeDepth(float z, float near, float far) {
    float ndc = z * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - ndc * (far - near));
}

// not used atm - debug
vec3 computeNormalNaive(const sampler2D depthTex, ivec2 p) {
    ivec2 left   = clamp(p - ivec2(1, 0), ivec2(0), textureSize(depthTex, 0) - ivec2(1));
    ivec2 right  = clamp(p + ivec2(1, 0), ivec2(0), textureSize(depthTex, 0) - ivec2(1));
    ivec2 top    = clamp(p + ivec2(0, 1), ivec2(0), textureSize(depthTex, 0) - ivec2(1));
    ivec2 bottom = clamp(p - ivec2(0, 1), ivec2(0), textureSize(depthTex, 0) - ivec2(1));

    float dCenter = linearizeDepth(texelFetch(depthTex, p, 0).r, 0.1, 100);
    if (dCenter == 1.0) discard; // background

    vec3 l1 = getPos(left,   linearizeDepth(texelFetch(depthTex, left,   0).r, 0.1, 100));
    vec3 r1 = getPos(right,  linearizeDepth(texelFetch(depthTex, right,  0).r, 0.1, 100));
    vec3 t1 = getPos(top,    linearizeDepth(texelFetch(depthTex, top,    0).r, 0.1, 100));
    vec3 b1 = getPos(bottom, linearizeDepth(texelFetch(depthTex, bottom, 0).r ,0.1, 100));

    vec3 dpdx = l1 - r1;
    vec3 dpdy = t1 - b1;

    return normalize(cross(dpdx, dpdy));
}

/*
 * Computes a surface normal for a fragment using a triangle fan of neighbors
 * in the depth buffer. For each pair of neighboring points around the center,
 * it forms a triangle and accumulates the cross product. The final normal is
 * the normalized sum of all triangle normals.
 */
vec3 computeNormalTriangleWeighted(const sampler2D depthTex, ivec2 p) {
    float centerDepth = texelFetch(depthTex, p, 0).r;
    if (centerDepth == 1.0) discard;

    vec3 centerPos = getPos(p, linearizeDepth(centerDepth, zNear, zFar));
    vec3 normal = vec3(0.0);
    float totalWeight = 0.0;

    ivec2 offsets[8] = ivec2[8](
        ivec2(-1, -1), ivec2(0, -1), ivec2(1, -1),
        ivec2(-1,  0),               ivec2(1,  0),
        ivec2(-1,  1), ivec2(0,  1), ivec2(1,  1)
    );

    for (int i = 0; i < 8; ++i) {
        ivec2 p1 = clamp(p + offsets[i], ivec2(0), textureSize(depthTex, 0) - ivec2(1));
        ivec2 p2 = clamp(p + offsets[(i + 1) % 8], ivec2(0), textureSize(depthTex, 0) - ivec2(1));

        float d1 = linearizeDepth(texelFetch(depthTex, p1, 0).r, zNear, zFar);
        float d2 = linearizeDepth(texelFetch(depthTex, p2, 0).r, zNear, zFar);
        if (d1 == 1.0 || d2 == 1.0) continue;

        vec3 v1 = getPos(p1, d1);
        vec3 v2 = getPos(p2, d2);

        vec3 triNormal = cross(v2 - centerPos, v1 - centerPos);
        float weight = length(triNormal); // triangle area approximation

        normal += normalize(triNormal) * weight;
        totalWeight += weight;
    }

    if (totalWeight == 0.0) discard;

    return normalize(normal / totalWeight);
}



void main() {
    ivec2 fragCoord = ivec2(gl_FragCoord.xy);
     vec3 normal = computeNormalTriangleWeighted(depthTex, fragCoord);

    
    //convert to world space
    mat3 normalMatrix = mat3(transpose(inverse(view)));
    vec3 normal_world = normalize(normalMatrix * normal);

    FragColor = vec4(normal_world * 0.5 + 0.5, 1.0);
}
