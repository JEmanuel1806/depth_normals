#version 440 core 

in vec3 vColor;

out vec4 FragColor;

void main(){
	FragColor = vec4(vColor,1.0);
	// FragColor = vec4(vec3(0.3),1.0);
}