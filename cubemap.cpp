#include "cubemap.hpp"

extern "C" {
#include "texture.h"
#include "util.h"
}

#include <cstring>

Cubemap::Cubemap(double size, const char* texture_path) :
    cube{size},
    texture{load_cubemap(texture_path)}
{
}

void Cubemap::draw() {
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->texture);
    this->cube.draw();
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}
