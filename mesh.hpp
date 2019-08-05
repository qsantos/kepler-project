#ifndef MESH_HPP
#define MESH_HPP

#include "orbit.hpp"

struct Mesh {
    Mesh(int mode, int length, bool is_3d);
    ~Mesh(void);
    void bind(void);
    void draw(void);

    int mode;
    int length;
    bool is_3d;
    unsigned vbo;
};

struct CubeMesh : public Mesh {
    CubeMesh(double size);
};

struct UVSphereMesh : public Mesh {
    UVSphereMesh(float radius, int stacks, int slices);
};

struct OrbitMesh : public Mesh {
    OrbitMesh(Orbit* orbit);
};

struct OrbitApsesMesh : public Mesh {
    OrbitApsesMesh(Orbit* orbit);
};

struct FocusedOrbitMesh : public Mesh {
    FocusedOrbitMesh(Orbit* orbit, double time);
};

struct FocusedOrbitApsesMesh : public Mesh {
    FocusedOrbitApsesMesh(Orbit* orbit, double time);
};

struct OrbitSystem : public Mesh {
    OrbitSystem(CelestialBody* root, vec3 scene_origin, double time);
};

#endif
