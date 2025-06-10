#version 440 core

uniform sampler2D depthTex;
uniform isampler2D idTex;
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

// not used atm - debug
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
vec3 computeNormalTriangle(const sampler2D depthTex, ivec2 p) {
    // discard current pixel if depth == 1.0 
    float centerDepth = texelFetch(depthTex, p, 0).r;
    if (centerDepth == 1.0) discard;

    vec3 centerPos = getPos(p, centerDepth);
    vec3 normal = vec3(0.0);

    // array of offsets to choose different neighbors (8 in total) for the current point in next step 
    // buffer to store offset and upload to gpu
    ivec2 offsets[8] = ivec2[8](
        ivec2(-1, -1), 
        ivec2(0, -1), 
        ivec2(1, -1),
        ivec2(-1,  0),               
        ivec2(1,  0),
        ivec2(-1,  1), 
        ivec2(0,  1), 
        ivec2(1,  1)
    );

    // form triangles around the center 
    for (int i = 0; i < offsets.length(); i++) {
        // get position of two neighboring points
        // clamp between 0 and maximum textureSize to stay in boundary of texture
        ivec2 p1 = clamp(p + offsets[i], ivec2(0), textureSize(depthTex, 0) - ivec2(1));
        ivec2 p2 = clamp(p + offsets[(i + 1) % 8], ivec2(0), textureSize(depthTex, 0) - ivec2(1));

        // get depth of the two chosen points
        float d1 = texelFetch(depthTex, p1, 0).r;
        float d2 = texelFetch(depthTex, p2, 0).r;

        // skip if background
        if (d1 == 1.0 || d2 == 1.0) continue; 

        // reconstruct position of neighbors
        vec3 v1 = getPos(p1, d1);
        vec3 v2 = getPos(p2, d2);

        // calculate normal of the formed triangle and store it
        vec3 triNormal = cross(v2 - centerPos, v1 - centerPos );      
        normal += triNormal;
        
    }
    //  direction of sum
    return normalize(normal);
}



void main() {
    ivec2 fragCoord = ivec2(gl_FragCoord.xy);
    // vec3 normal = computeNormalNaive(depthTex, fragCoord);
    vec3 normal = computeNormalTriangle(depthTex, fragCoord);

    
    //convert to world space
    mat3 normalMatrix = mat3(transpose(inverse(view)));
    vec3 normal_world = normalize(normalMatrix * normal);

    FragColor = vec4(normal * 0.5 + 0.5, 1.0);
    // FragColor = vec4(1.0,1.0,1.0,1.0);
}


/*
//Debug stuff 
void main() {
ivec2 fragCoord = ivec2(gl_FragCoord.xy);
int id = texelFetch(idTex, fragCoord, 0).r;

if (id == -1) {
    FragColor = vec4(0.0, 0.0, 0.0, 1.0); // schwarz
} else {
    float hue = float(id % 256) / 255.0;
    FragColor = vec4(hue, hue * 0.5, 1.0 - hue, 1.0);
}
}
*/