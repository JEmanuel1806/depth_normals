#version 440 core

uniform sampler2D depthTex;
uniform isampler2D idTex;
uniform vec2 iResolution;
uniform mat4 invProj;
uniform mat4 view;

uniform mat4 invView;

uniform float zNear;
uniform float zFar;

in vec2 texCoords;

layout(location = 0) out vec4 FragColor;

vec3 getPos(ivec2 fragCoord, float depth) {
        
    vec2 ndc = (vec2(fragCoord) / iResolution) * 2.0 - 1.0;
    float ndcDepth = depth * 2.0 - 1.0; // - 1 to 1
    
    vec4 clipSpace = vec4(ndc, ndcDepth, 1.0);
    vec4 viewSpace = invProj * clipSpace;
    viewSpace /= viewSpace.w;

    vec4 worldSpace = invView * viewSpace;
    return worldSpace.xyz;
}

// linearize depth because depth buffer precision
float linearizeDepth(float z, float near, float far) {
    float ndc = z * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - ndc * (far - near));
    //return z;
}

// debug
vec3 computeNormalNaive(const sampler2D depthTex, ivec2 p) {
    ivec2 size = textureSize(depthTex, 0);
    ivec2 left   = (p - ivec2(1, 0)); 
    ivec2 right  = (p + ivec2(1, 0));
    ivec2 top    = (p - ivec2(0, 1));
    ivec2 bottom = (p + ivec2(0, 1));

    float dCenter = texelFetch(depthTex, p, 0).r;
    if (dCenter >= 1.0 || dCenter <= 0.0) return vec3(0);

    // get depth from the 4 neighbors
    float dL = texelFetch(depthTex, left, 0).r;
    float dR = texelFetch(depthTex, right, 0).r;
    float dT = texelFetch(depthTex, top, 0).r;
    float dB = texelFetch(depthTex, bottom, 0).r;

    // if any of the neighbors depth = 0 or > 1 then return 0 vector
    int valid = 0;
    valid += (dL < 1.0 && dL > 0.0) ? 1 : 0;
    valid += (dR < 1.0 && dR > 0.0) ? 1 : 0;
    valid += (dT < 1.0 && dT > 0.0) ? 1 : 0;
    valid += (dB < 1.0 && dB > 0.0) ? 1 : 0;
    if (valid < 1) 
        return vec3(0);

    vec3 l1 = getPos(left, dL);
    vec3 r1 = getPos(right, dR);
    vec3 t1 = getPos(top, dT);
    vec3 b1 = getPos(bottom, dB);

    vec3 dpdx = r1 - l1;
    vec3 dpdy = b1 - t1;

    vec3 normal = cross(dpdx, dpdy);

    if (normal.z > 0)
    normal *= -1;

    return normalize(normal);
}

/*
 * Computes a surface normal for a fragment using a triangle fan of neighbors
 * in the depth buffer. For each pair of neighboring points around the center,
 * it forms a triangle and accumulates the cross product. The final normal is
 * the normalized sum of all triangle normals.
 */
vec3 computeNormalTriangleWeighted(const sampler2D depthTex, ivec2 p) {
    float centerDepth = texelFetch(depthTex, p, 0).r;
    if (centerDepth == 1.0) return vec3(0,0,0);

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
        if (d1 == 1.0 || d2 == 1.0) return vec3(0,0,0);

        vec3 v1 = getPos(p1, d1);
        vec3 v2 = getPos(p2, d2);

        vec3 triNormal = cross(v2 - centerPos, v1 - centerPos);
        float weight = length(triNormal); // triangle area approximation

        normal += normalize(triNormal) * weight;
        totalWeight += weight;
    }

    if (totalWeight == 0.0) return vec3(0,0,0);

    return normalize(normal / totalWeight);
}



void main() {
    ivec2 fragCoord = ivec2(gl_FragCoord.xy);
     vec3 normal = computeNormalNaive(depthTex, fragCoord);

    FragColor = vec4(normal * 0.5 + 0.5, 1.0);
}
