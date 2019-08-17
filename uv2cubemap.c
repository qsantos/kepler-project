#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline double my_fmod(double x, double y) {
    double r = fmod(x, y);
    if (r < 0.) {
        r += y;
    }
    return r;
}

inline void uv_from_cubic(int* u, int* v, int x, int y, int z, int width, int height) {
    // find spherical coordinates
    double phi = atan2((double) z, (double) x);
    double theta = atan2(hypot((double) z, (double) x), (double) y);
    phi = my_fmod(phi, 2. * M_PI);
    theta = my_fmod(theta, 2. * M_PI);
    // find UV coordinates
    *u = (int) floor(phi / (2. * M_PI) * (double) width);
    *v = (int) floor(theta / M_PI * (double) height);
}

#define I_PIXEL(x, y) (input +  ((y) * width + (x)) * 4)
#define O_PIXEL(x, y) (output + ((y) * size + (x)) * 4)

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s filename size\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* filename = argv[1];
    int size = (int) strtol(argv[2], NULL, 0);
    if (size <= 0) {
        fprintf(stderr, "size must be a positive integer\n");
        exit(EXIT_FAILURE);
    }

    int width, height;

    fprintf(stderr, "Loading %s\n", filename);
    unsigned char* input = stbi_load(filename, &width, &height, NULL, 4);
    fprintf(stderr, "Loaded %i×%i image\n", width, height);

    fprintf(stderr, "Allocating %i×%i output image\n", size, size);
    unsigned char* output = malloc((size_t) (size * size * 4));
    if (output == NULL) {
        fprintf(stderr, "Not enough memory\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Starting conversion\n");
    int s = size / 2;
    int output_quality = 75;
    int u, v;

    // +X
    for (int z = -s; z < s; z += 1) {
        for (int y = -s; y < s; y += 1) {
            int x = -s;  // ???
            uv_from_cubic(&u, &v, x, y, z, width, height);
            memcpy(O_PIXEL(s - 1 - z, s - 1 - y), I_PIXEL(u, v), 4);
        }
    }
    stbi_write_jpg("PositiveX.jpg", size, size, 4, output, output_quality);

    // -X
    for (int z = -s; z < s; z += 1) {
        for (int y = -s; y < s; y += 1) {
            int x = +s;  // ???
            uv_from_cubic(&u, &v, x, y, z, width, height);
            memcpy(O_PIXEL(s + z, s - 1 - y), I_PIXEL(u, v), 4);
        }
    }
    stbi_write_jpg("NegativeX.jpg", size, size, 4, output, output_quality);

    // +Y
    for (int z = -s; z < s; z += 1) {
        for (int x = -s; x < s; x += 1) {
            int y = +s;
            uv_from_cubic(&u, &v, x, y, z, width, height);
            memcpy(O_PIXEL(s - 1 - z, s + x), I_PIXEL(u, v), 4);
        }
    }
    stbi_write_jpg("PositiveY.jpg", size, size, 4, output, output_quality);

    // -Y
    for (int z = -s; z < s; z += 1) {
        for (int x = -s; x < s; x += 1) {
            int y = -s;
            uv_from_cubic(&u, &v, x, y, z, width, height);
            memcpy(O_PIXEL(s - 1 - z, s - 1 - x), I_PIXEL(u, v), 4);
        }
    }
    stbi_write_jpg("NegativeY.jpg", size, size, 4, output, output_quality);

    // +Z
    for (int x = -s; x < s; x += 1) {
        for (int y = -s; y < s; y += 1) {
            int z = +s;
            uv_from_cubic(&u, &v, x, y, z, width, height);
            memcpy(O_PIXEL(s - 1 - x, s - 1 - y), I_PIXEL(u, v), 4);
        }
    }
    stbi_write_jpg("PositiveZ.jpg", size, size, 4, output, output_quality);

    // -Z
    for (int x = -s; x < s; x += 1) {
        for (int y = -s; y < s; y += 1) {
            int z = -s;
            uv_from_cubic(&u, &v, x, y, z, width, height);
            memcpy(O_PIXEL(s + x, s - 1 - y), I_PIXEL(u, v), 4);
        }
    }
    stbi_write_jpg("NegativeZ.jpg", size, size, 4, output, output_quality);

}
