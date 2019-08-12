#ifndef SHADERS_H
#define SHADERS_H

#ifdef MSYS2
#include <windef.h>
#endif
#include <GL/glew.h>

GLuint make_program(size_t n_shaders, ...);

#endif
