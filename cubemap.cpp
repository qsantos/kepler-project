extern "C" {
#include "texture.h"
#include "util.h"
}
#include "cubemap.hpp"

#include <cstring>

static size_t count(const char* s, const char* pattern) {
    size_t matches = 0;
    if (s == NULL || pattern == NULL) {
        return 0;
    }

    size_t n_pattern = strlen(pattern);
    while (1) {
        s = strstr(s, pattern);
        if (s == NULL) {
            break;
        }

        matches += 1;
        s += n_pattern;;
    }

    return matches;
}

static char* replace(const char* s, const char* pattern, const char* replacement) {
    count(NULL, pattern);

    size_t n_s = strlen(s);
    size_t n_pattern = strlen(pattern);
    size_t n_replacement = strlen(replacement);

    size_t n = n_s + count(s, pattern) * (n_replacement - n_pattern) + 1;
    char* ret = (char*) MALLOC(n);

    char* cur = ret;
    while (1) {
        // find pattern
        const char* match = strstr(s, pattern);
        if (match == NULL) {
            break;
        }

        // copy s until pattern
        size_t n_prefix = match - s;
        memcpy(cur, s, n_prefix);
        s = match;
        cur += n_prefix;

        // copy replacement
        memcpy(cur, replacement, n_replacement);
        s += n_pattern;
        cur += n_replacement;
    }

    // copy rest of s
    memcpy(cur, s, strlen(s));
    ret[n - 1] = '\0';

    return ret;
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
        char* path = replace(texture_path, "{}", faces[i]);
        int width, height;
        unsigned char* data = load_image(path, &width, &height);
        free(path);
        if (data == NULL) {
            continue;
        }
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
        );
        // stb_image_free(data);  // TODO
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
