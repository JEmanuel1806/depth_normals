#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader(const char* vertex_source, const char* fragment_source) {
    std::string vertexCode = readFile(vertex_source);
    std::string fragmentCode = readFile(fragment_source);

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

Shader::Shader(const char* vertex_source, const char* geometry_source, const char* fragment_source) {
    std::string vertexCode = readFile(vertex_source);
    std::string geometryCode = readFile(geometry_source);
    std::string fragmentCode = readFile(fragment_source);

    const char* vertex_shader_code = vertexCode.c_str();
    const char* geometry_shader_code = geometryCode.c_str();
    const char* fragment_shader_code = fragmentCode.c_str();

    GLuint vertex_shader = compileShader(vertex_shader_code, GL_VERTEX_SHADER);
    GLuint geometry_shader = compileShader(geometry_shader_code, GL_GEOMETRY_SHADER);
    GLuint fragment_shader = compileShader(fragment_shader_code, GL_FRAGMENT_SHADER);

    ID = glCreateProgram();
    glAttachShader(ID, vertex_shader);
    glAttachShader(ID, geometry_shader);
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
        std::cout << "reading from: " + shader_path << std::endl;
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

    // If it didnt compile, check
    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") 
            << "\n" << infoLog << std::endl;
    }
    else {
        std::cout << "successfully compiled shader\n";
    }

    return shader;
}
