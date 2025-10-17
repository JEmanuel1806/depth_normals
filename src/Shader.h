#pragma once

#include <GL/glew.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    GLuint m_shaderID;;

public:
    Shader(const char* vertexPath, const char* fragmentPath);
    Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath);
    Shader(const char* computePath);
    ~Shader();

    void Use();

private:

    std::string ReadFile(const std::string& shaderPath);

    GLuint CompileShader(const char* source, GLenum type);
};
