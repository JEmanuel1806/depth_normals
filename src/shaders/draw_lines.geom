#version 440 core
layout(points) in;
layout(line_strip, max_vertices = 2) out;

in vec3 vNormal[];  
in vec3 vPosition[];

uniform mat4 view;
uniform mat4 proj;

float scaleFactor = 0.6;

// for each point generate a line starting from the point and ending with a scale factor of the normal
void main()
{
    vec3 start = vPosition[0];
    vec3 end = start + normalize(vNormal[0]) * scaleFactor;

    gl_Position = proj * view * vec4(start, 1.0);
    EmitVertex();

    gl_Position = proj * view * vec4(end, 1.0);
    EmitVertex();

    EndPrimitive();
}