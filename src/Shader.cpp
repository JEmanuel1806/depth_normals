#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader() {
    std::string vertexCode = readFile("src/shaders/triangle_test.vert");
    std::string fragmentCode = readFile("src/shaders/triangle_test.frag");

    const char* vertex_shader_code = vertexCode.c_str();
    const char* fragment_shader_code = fragmentCode.c_str();

    GLuint vertex_shader = compileShader(vertex_shader_code, GL_VERTEX_SHADER);
    GLuint fragment_shader = compileShader(fragment_shader_code, GL_FRAGMENT_SHADER);

    ID = glCreateProgram();
    glAttachShader(ID, vertex_shader);
    glAttachShader(ID, fragment_shader);
    glLinkProgram(ID);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

Shader::~Shader() {
    glDeleteProgram(ID);
}

void Shader::use() {
    glUseProgram(ID);
}

std::string Shader::readFile(const std::string& shader_path) {
    std::ifstream shader_file(shader_path);
    std::stringstream shader_content;

    if (shader_file.is_open()) {
        shader_content << shader_file.rdbuf();
        shader_file.close();
    }
    else {
        std::cerr << "Could not open file" << std::endl;
    }

    return shader_content.str();
}

GLuint Shader::compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    return shader;
}
