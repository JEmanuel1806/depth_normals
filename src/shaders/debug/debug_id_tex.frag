#version 440 core
uniform isampler2D idTex;
out vec4 FragColor;

vec3 hsv2rgb(vec3 c){
    vec3 p = abs(fract(c.xxx + vec3(0, 2.0/3.0, 1.0/3.0))*6.0 - 3.0);
    return c.z * mix(vec3(1), clamp(p-1.0, 0.0, 1.0), c.y);
}

vec3 colorFromID(int id){
    // einfacher 32-bit Hash
    uint x = uint(id)*1664525u + 1013904223u;
    float h = float(x % 360u)/360.0;  // 0..1
    return hsv2rgb(vec3(h, 0.85, 1.0));
}

void main(){
    ivec2 p = ivec2(gl_FragCoord.xy);
    int id  = texelFetch(idTex, p, 0).r;

    if (id < 0) 
    { 
        FragColor = vec4(0,0,0,0); 
        return; 
    } 
    else if (id == 1041261120.000000) 
    {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    FragColor = vec4(colorFromID(id), 1.0);
}