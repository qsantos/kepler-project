#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <string>
#include <initializer_list>

#include <GL/glew.h>

GLuint make_program(std::initializer_list<std::string> shaders);

#endif
