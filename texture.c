#include "util.h"

#include <GL/gl.h>
#include <GL/glext.h>
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


unsigned char* load_image(const char* filename, int* width, int* height) {
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(filename, width, height, NULL, 3);
    if (data == NULL) {
        fprintf(stderr, "WARNING: failed to load image %s\n", filename);
        return 0;
    }
    return data;
}

unsigned load_texture(const char* filename) {
    double start = real_clock();

    int width, height;
    unsigned char* data = load_image(filename, &width, &height);
    if (data == NULL) {
        return 0;
    }

    // make OpenGL texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // free temporary storage
    stbi_image_free(data);

    double elapsed = real_clock() - start;
    if (data != NULL) {
        printf("%s loaded in %.3f s\n", filename, elapsed);
    }
    return texture;
}
