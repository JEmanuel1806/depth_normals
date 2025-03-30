#pragma once

#include <GL/glew.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    // Shader program ID
    GLuint m_shaderID;;

public:
    Shader(const char* szVertexPath, const char* szFragmentPath);
    Shader(const char* szVertexPath, const char* szGeometryPath, const char* szFragmentPath);
    ~Shader();

    void Use();

private:

    std::string ReadFile(const std::string& shaderPath);

    GLuint CompileShader(const char* source, GLenum type);
};
