extern "C" {
#include "texture.h"
}
#include "cubemap.hpp"

#include <string>

using std::string;

// TODO: duplicated code
// how is this not part of the STL?
static string replace(string s, string pattern, string replacement) {
    size_t pos = s.find(pattern);
    while (pos != string::npos) {
        s.replace(pos, pattern.size(), replacement);
        pos = s.find(pattern, pos + replacement.size());
    }
    return s;
}

Cubemap::Cubemap(double size, const char* texture_path) :
    cube{size}
{
    glGenTextures(1, &this->texture);

    glBindTexture(GL_TEXTURE_CUBE_MAP, this->texture);

    const char* faces[] = {
        "PositiveX", "NegativeX",
        "PositiveY", "NegativeY",
        "PositiveZ", "NegativeZ",
    };

    for (GLenum i = 0; i < 6; i += 1) {
        string path = replace(texture_path, "{}", faces[i]);
        int width, height;
        unsigned char* data = load_image(path.c_str(), &width, &height);
        if (data == NULL) {
            continue;
        }
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Cubemap::draw() {
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->texture);
    this->cube.draw();
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}
