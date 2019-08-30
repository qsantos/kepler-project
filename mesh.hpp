#ifndef MESH_HPP
#define MESH_HPP

#include "orbit.hpp"

struct Mesh {
    Mesh(unsigned mode, int length, bool is_3d);
    ~Mesh(void);
    void bind(void);
    void draw(void);

    unsigned mode;
    int length;
    bool is_3d;
    unsigned vbo;
};

struct RectMesh : public Mesh {
    RectMesh(double width, double height);
};

struct CubeMesh : public Mesh {
    CubeMesh(double size);
};

struct UVSphereMesh : public Mesh {
    UVSphereMesh(float radius, int lod);
};

struct QuadSphereMesh : public Mesh {
    QuadSphereMesh(float radius, int lod);
};

struct IcoSphereMesh : public Mesh {
    IcoSphereMesh(float radius, int lod);
};

struct OrbitMesh : public Mesh {
    OrbitMesh(Orbit* orbit, double time=0., bool focused=false);
};

struct OrbitApsesMesh : public Mesh {
    OrbitApsesMesh(Orbit* orbit, double time=0., bool focused=false);
};

struct OrbitSystem : public Mesh {
    OrbitSystem(CelestialBody* root, const glm::dvec3& scene_origin, double time);
};

#endif
