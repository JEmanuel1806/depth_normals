#version 440 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec3 aColor;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

uniform float pointSize;

out vec3 vColor;

void main(){
	gl_Position = proj * view * model * vec4(aPos, 1.0);
	gl_PointSize = 3;
	vColor = aColor;
}