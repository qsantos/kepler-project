#ifndef MESH_HPP
#define MESH_HPP

#include "orbit.hpp"

struct UVSphereMesh {
    UVSphereMesh(float radius, int stacks, int slices);
    void bind(void);
    void draw(void);

    int components;
    int length;
    unsigned vbo;
};

struct OrbitMesh {
    OrbitMesh(Orbit* orbit);
    void bind(void);
    void draw(void);

    int components;
    int length;
    unsigned vbo;
};

#endif
