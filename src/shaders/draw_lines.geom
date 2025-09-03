#version 440 core
layout(points) in;
layout(triangle_strip, max_vertices = 2*(24+1) + 2*(24+1)) out;

in vec3 vNormal[];
in vec3 vPosition[];

uniform mat4 proj, view, model;
out vec3 fNormal;

void main() {
    vec3 p0 = vPosition[0];
    vec3 n  = normalize(vNormal[0]);

    vec3 a = (abs(n.z)<0.999)? vec3(0,0,1): vec3(0,1,0);
    vec3 t = normalize(cross(a,n));
    vec3 b = cross(n,t);
    mat3 M = mat3(t,b,n);

    float shaftLen=0.05;
    float headLen=0.02;
    float shaftR=0.005;
    float headR=0.01;
    
    int segments = 16;

    vec3 p1 = p0 + n * shaftLen;
    vec3 tip= p1 + n * headLen;

    for(int i = 0; i <= segments; i++){
        
        float ang = i * (2 * 3.14159/segments);
        vec3 ring = M * vec3(cos(ang),sin(ang),0);
        vec3 v0 = p0 + ring * shaftR;
        vec3 v1 = p1 + ring * shaftR;
        
        fNormal = normalize(ring);
        gl_Position = proj * view * model * vec4(v0,1); 
        EmitVertex();
        
        fNormal=normalize(ring);
        gl_Position= proj * view * model * vec4(v1,1); 
        EmitVertex();
    }
    EndPrimitive();

    for(int i = 0; i <= segments; i++) {
        
        float ang = i * (2 * 3.14159/segments);
        vec3 ring = M * vec3(cos(ang),sin(ang),0);
        vec3 rim = p1 + ring * headR;
        
        fNormal=normalize(ring * headR+ n*headLen);
        
        gl_Position= proj * view * model * vec4(rim,1); 
        EmitVertex();

        gl_Position= proj * view * model * vec4(tip,1); 
        EmitVertex();
    }
    EndPrimitive();
}
