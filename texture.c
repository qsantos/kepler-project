#include "texture.h"

#include "util.h"
#include "logging.h"

#ifdef MSYS2
#include <windef.h>
#endif
#include <GL/glew.h>
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

unsigned char* load_image(const char* filename, int* width, int* height) {
    return stbi_load(filename, width, height, NULL, 4);
}

unsigned load_texture(const char* filename) {
    DEBUG("Texture '%s' loading", filename);

    int width, height;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = load_image(filename, &width, &height);
    stbi_set_flip_vertically_on_load(0);
    if (data == NULL) {
        DEBUG("Failed to load texture '%s'\n", filename);
        return 0;
    }

    // make OpenGL texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    // free temporary storage
    stbi_image_free(data);

    DEBUG("Texture '%s' loaded", filename);
    return texture;
}

unsigned load_cubemap(const char* path_pattern) {
    DEBUG("Cubemap texture '%s' loading", path_pattern);

    unsigned texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    static const char* faces[] = {
        "PositiveX", "NegativeX",
        "PositiveY", "NegativeY",
        "PositiveZ", "NegativeZ",
    };

    for (GLenum i = 0; i < 6; i += 1) {
        char* path = replace(path_pattern, "{}", faces[i]);
        int width, height;
        unsigned char* data = load_image(path, &width, &height);
        if (data == NULL) {
            stbi_image_free(data);
            glDeleteTextures(1, &texture);
            DEBUG("Failed to load texture '%s' for cubemap\n", path);
            free(path);
            return 0;
        }
        free(path);
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
        );
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    DEBUG("Cubemap texture '%s' loaded", path_pattern);
    return texture;
}
