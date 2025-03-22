#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


class Shader
{
public:
    unsigned int ID;
    Shader(const char* vertex_source, const char* fragment_source);
    Shader(const char* vertex_source, const char* geometry_source, const char* fragment_source);
    ~Shader();
    void use();
private:
    std::string readFile(const std::string& shader_path);
    GLuint compileShader(const char* source, GLenum type);
};

#endif