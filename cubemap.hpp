#ifndef CUBEMAP_HPP
#define CUBEMAP_HPP

#include "mesh.hpp"

#include <GL/glew.h>

struct Cubemap {
    Cubemap(double size, const char* texture_path);
    void draw();

    CubeMesh cube;
    GLuint texture;
};

#endif
