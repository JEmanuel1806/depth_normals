#version 440 core
layout(points) in;
layout(line_strip, max_vertices = 6) out;

in vec3 vNormal[];  
in vec3 vPosition[];

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

out vec3 fNormal;

float scaleFactor = 0.1;

// for each point generate a line starting from the point and ending with a scale factor of the normal
void main()
{
    vec3 start = vPosition[0];
    vec3 end = start + normalize(vNormal[0]) * scaleFactor;
    vec3 up = vec3(0,1,0);

    vec3 arrow_end_left = end - normalize(vNormal[0])*0.1 + cross(normalize(vNormal[0]), up)*0.1;
    vec3 arrow_end_right = end - normalize(vNormal[0])*0.1 - cross(normalize(vNormal[0]), up)*0.1;

    // Base
    fNormal = normalize(vNormal[0]);
    gl_Position = proj * view * model * vec4(start, 1.0);
    EmitVertex();

    fNormal = normalize(vNormal[0]);
    gl_Position = proj * view * model * vec4(end, 1.0);
    EmitVertex();

    EndPrimitive();

    // Arrow 1 left
    fNormal = normalize(vNormal[0]);
    gl_Position = proj * view * model * vec4(end, 1.0);
    EmitVertex();

    fNormal = normalize(vNormal[0]);
    gl_Position = proj * view * model * vec4(arrow_end_left, 1.0);
    EmitVertex();

    EndPrimitive();

    // Arrow 2 right
    fNormal = normalize(vNormal[0]);
    gl_Position = proj * view * model * vec4(end, 1.0);
    EmitVertex();

    fNormal = normalize(vNormal[0]);
    gl_Position = proj * view * model * vec4(arrow_end_right, 1.0);
    EmitVertex();

    EndPrimitive();
}