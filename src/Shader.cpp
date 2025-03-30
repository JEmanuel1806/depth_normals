#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader(const char* vertex_source, const char* fragment_source) {
    std::string vertexCode = ReadFile(vertex_source);
    std::string fragmentCode = ReadFile(fragment_source);

    const char* vertex_shader_code = vertexCode.c_str();
    const char* fragment_shader_code = fragmentCode.c_str();

    GLuint vertex_shader = CompileShader(vertex_shader_code, GL_VERTEX_SHADER);
    GLuint fragment_shader = CompileShader(fragment_shader_code, GL_FRAGMENT_SHADER);

    m_shaderID = glCreateProgram();
    glAttachShader(m_shaderID, vertex_shader);
    glAttachShader(m_shaderID, fragment_shader);
    glLinkProgram(m_shaderID);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

Shader::Shader(const char* vertex_source, const char* geometry_source, const char* fragment_source) {
    std::string vertexCode = ReadFile(vertex_source);
    std::string geometryCode = ReadFile(geometry_source);
    std::string fragmentCode = ReadFile(fragment_source);

    const char* vertex_shader_code = vertexCode.c_str();
    const char* geometry_shader_code = geometryCode.c_str();
    const char* fragment_shader_code = fragmentCode.c_str();

    GLuint vertex_shader = CompileShader(vertex_shader_code, GL_VERTEX_SHADER);
    GLuint geometry_shader = CompileShader(geometry_shader_code, GL_GEOMETRY_SHADER);
    GLuint fragment_shader = CompileShader(fragment_shader_code, GL_FRAGMENT_SHADER);

    m_shaderID = glCreateProgram();
    glAttachShader(m_shaderID, vertex_shader);
    glAttachShader(m_shaderID, geometry_shader);
    glAttachShader(m_shaderID, fragment_shader);
    glLinkProgram(m_shaderID);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

Shader::~Shader() {
    glDeleteProgram(m_shaderID);
}

void Shader::Use() {
    glUseProgram(m_shaderID);
}

std::string Shader::ReadFile(const std::string& shader_path) {
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

GLuint Shader::CompileShader(const char* source, GLenum type) {
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
