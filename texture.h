#ifndef TEXTURE_H
#define TEXTURE_H

unsigned char* load_image(const char* filename, int* width, int* height);
unsigned load_texture(const char* filename);
unsigned load_cubemap(const char* path_pattern);

#endif
