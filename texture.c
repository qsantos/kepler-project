#include "util.h"

#include <GL/gl.h>
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

unsigned load_texture(const char* filename) {
    double start = real_clock();

    // load image
    stbi_set_flip_vertically_on_load(1);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0); 

    // make OpenGL texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenTextures(1, 0);

    // free temporary storage
    stbi_image_free(data);

    double elapsed = real_clock() - start;
    printf("%s loaded in %.3f s\n", filename, elapsed);
    return texture;
}
