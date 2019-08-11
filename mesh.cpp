#include "mesh.hpp"

extern "C" {
#include "util.h"
}

#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

#include <cmath>
#include <vector>

Mesh::Mesh(int mode_, int length_, bool is_3d_) :
    mode{mode_},
    length{length_},
    is_3d{is_3d_},
    vbo{0}
{
}

Mesh::~Mesh(void) {
    //glDeleteBuffers(1, &this->vbo);
}

void Mesh::bind(void) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);

    if (this->is_3d) {
        GLint var = glGetAttribLocation(program, "v_position");
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, 8 * (GLsizei) sizeof(float), NULL);

        var = glGetAttribLocation(program, "v_texcoord");
        if (var >= 0) {
            glEnableVertexAttribArray(var);
            glVertexAttribPointer(var, 2, GL_FLOAT, GL_FALSE, 8 * (GLsizei) sizeof(float), (GLvoid*)(3 * sizeof(float)));
        }

        var = glGetAttribLocation(program, "v_normal");
        if (var >= 0) {
            glEnableVertexAttribArray(var);
            glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, 8 * (GLsizei) sizeof(float), (GLvoid*)(5 * sizeof(float)));
        }
    } else {
        GLint var = glGetAttribLocation(program, "v_position");
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::draw(void) {
    this->bind();
    glDrawArrays(this->mode, 0, this->length);
}

SquareMesh::SquareMesh(double size) :
    Mesh(GL_TRIANGLE_STRIP, 4, true)
{
    float s = (float) size / 2.f;

    float data[] = {
        -s, -s, 0, 0, 0, 0, 0, 0,
        +s, -s, 0, 1, 0, 0, 0, 0,
        -s, +s, 0, 0, 1, 0, 0, 0,
        +s, +s, 0, 1, 1, 0, 0, 0,
    };

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

CubeMesh::CubeMesh(double size) :
    Mesh(GL_TRIANGLES, 36, false)
{
    float s = (float) size / 2.f;

    float data[] = {
        // each row is a full triangle, two rows make a face
        // +X
        +s, +s, +s,   +s, +s, -s,   +s, -s, +s,
        +s, -s, +s,   +s, +s, -s,   +s, -s, -s,
        // -X
        -s, -s, +s,   -s, -s, -s,   -s, +s, +s,
        -s, +s, +s,   -s, -s, -s,   -s, +s, -s,
        // +Y
        -s, +s, +s,   -s, +s, -s,   +s, +s, +s,
        +s, +s, +s,   -s, +s, -s,   +s, +s, -s,
        // -Y
        +s, -s, +s,   +s, -s, -s,   -s, -s, +s,
        -s, -s, +s,   +s, -s, -s,   -s, -s, -s,
        // +Z
        +s, +s, +s,   +s, -s, +s,   -s, +s, +s,
        -s, +s, +s,   +s, -s, +s,   -s, -s, +s,
        // -Z
        +s, +s, -s,   -s, +s, -s,   +s, -s, -s,
        +s, -s, -s,   -s, +s, -s,   -s, -s, -s,
    };

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void uvspheremesh_add_vertex(std::vector<float>& data, size_t& i, float radius, int stacks, int slices, int stack, int slice) {
    float slice_angle = (2.f * M_PIf32) * float(slice) / float(slices);
    float stack_angle = M_PIf32 * float(stack) / float(stacks);

    float nx = sinf(stack_angle) * sinf(slice_angle);
    float ny = sinf(stack_angle) * cosf(slice_angle);
    float nz = cosf(stack_angle);

    // position
    data[i++] = radius * nx;
    data[i++] = radius * ny;
    data[i++] = radius * nz;
    // texcoord
    data[i++] = 1 - float(slice) / float(slices);
    data[i++] = 1 - float(stack) / float(stacks);
    // normal
    data[i++] = nx;
    data[i++] = ny;
    data[i++] = nz;
}

UVSphereMesh::UVSphereMesh(float radius, GLsizei stacks, GLsizei slices) :
    Mesh(GL_TRIANGLE_STRIP, 2 * (slices + 2) * stacks, true)
{
    std::vector<float> data;
    data.resize(8 * this->length);

    size_t i = 0;
    for (GLsizei stack = 0; stack < stacks ; stack += 1) {
        uvspheremesh_add_vertex(data, i, radius, stacks, slices, stack, 0);
        for (GLsizei slice = 0; slice <= slices; slice += 1) {
            uvspheremesh_add_vertex(data, i, radius, stacks, slices, stack, slice);
            uvspheremesh_add_vertex(data, i, radius, stacks, slices, stack + 1, slice);
        }
        uvspheremesh_add_vertex(data, i, radius, stacks, slices, stack + 1, slices);
    }

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, i * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void quadspheremesh_add_vertex(std::vector<float>& data, size_t& i, float radius, int divisions, int face, int xdivision, int ydivision) {
    float s = 2.f / (float) divisions;
    float a = -1.f + (float) xdivision * s;
    float b = -1.f + (float) ydivision * s;

    // NOTE: here, u and v refer to the outputs of the warp function
    // u and v are *NOT* UV coordinates

    // for more information, see http://jcgt.org/published/0007/02/01/paper.pdf
    // Cube-to-sphere Projections for Procedural Texturing and Beyond
    // by Matt Zucker and Yosuke Higashi from Swarthmore College
    // Journal of Computer Graphics Techniques Vol. 7, No. 2, 2018
    // section 3, starting page 6 and especially pages 9 through 11

    // identity function (subsection 3.1)
    //float u = a;
    //float v = b;

    // tangent warp function (subsection 3.2)
    //static const float theta = M_PIf32 / 4.f;  // Brown 2017
    //static const float theta = 0.8687f;  // near optimal
    //float u = tan(a * theta) / tan(theta);
    //float v = tan(b * theta) / tan(theta);

    // Everitt's univariate invertible warp function (subsection 3.3)
    //static const float eps = 1.5f;  // 2014
    //static const float eps = 1.375f;  // 2016
    //static const float eps = 1.4511f;  // near optimal
    //float u = copysign(1.f, a) * ((eps - sqrtf(eps * eps - 4.f * (eps - 1) * fabsf(a))) / 2.f / (eps - 1));
    //float v = copysign(1.f, b) * ((eps - sqrtf(eps * eps - 4.f * (eps - 1) * fabsf(b))) / 2.f / (eps - 1));

    // COBE quadrilateralized spherical cube (subsection 3.5)
    // TODO: see https://github.com/cix/QuadSphere

    // Arvo's exact equal-area method (subsection 3.6)
    float tu = tanf(a * (M_PIf32 / 6.f));
    float u = sqrtf(2.f) * tu / sqrtf(1.f -tu * tu);
    float v = b / sqrtf(1.f + (1.f - b * b) * cos(a * (M_PIf32 / 3.f)));

    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    switch (face) {
    case 0: { x =  u; y =  v; z = +1; break; }
    case 1: { x =  v; y =  u; z = -1; break; }
    case 2: { x =  u; y = -1; z =  v; break; }
    case 3: { x =  v; y = +1; z =  u; break; }
    case 4: { x = +1; y =  u; z =  v; break; }
    case 5: { x = -1; y =  v; z =  u; break; }
    default: assert(0);
    }

    float n = sqrt(x*x + y*y + z*z);
    x /= n;
    y /= n;
    z /= n;

    // position
    data[i++] = x * radius;
    data[i++] = y * radius;
    data[i++] = z * radius;
    // texcoord
    data[i++] = 0.f;  // atan2(y, x) / (2.f * M_PIf32) + .5f;
    data[i++] = 0.f;  // asin(z) / M_PIf32 + .5f;
    // normal
    data[i++] = x;
    data[i++] = y;
    data[i++] = z;
}

QuadSphereMesh::QuadSphereMesh(float radius, int divisions) :
    Mesh(GL_TRIANGLE_STRIP, 6 * 2 * (divisions + 2) * divisions, true)
{
    // make a face of divisions×divisions squares
    std::vector<float> data;
    data.resize(8 * this->length);
    size_t i = 0;

    for (int face = 0; face < 6; face += 1) {
        for (int ydivision = 0; ydivision < divisions; ydivision += 1) {
            quadspheremesh_add_vertex(data, i, radius, divisions, face, 0, ydivision);

            for (int xdivision = 0; xdivision <= divisions; xdivision += 1) {
                quadspheremesh_add_vertex(data, i, radius, divisions, face, xdivision, ydivision);
                quadspheremesh_add_vertex(data, i, radius, divisions, face, xdivision, ydivision + 1);
            }

            quadspheremesh_add_vertex(data, i, radius, divisions, face, divisions, ydivision + 1);
        }
    }

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, i * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

OrbitMesh::OrbitMesh(Orbit* orbit) :
    Mesh(GL_LINE_LOOP, 256, false)
{
    auto transform = glm::mat4(1.0f);
    transform = glm::rotate(transform, float(orbit->longitude_of_ascending_node), glm::vec3(0.f, 0.f, 1.f));
    transform = glm::rotate(transform, float(orbit->inclination),                 glm::vec3(1.f, 0.f, 0.f));
    transform = glm::rotate(transform, float(orbit->argument_of_periapsis),       glm::vec3(0.f, 0.f, 1.f));
    transform = glm::translate(transform, glm::vec3(-orbit->focus, 0.f, 0.f));
    transform = glm::scale(transform, glm::vec3(orbit->semi_major_axis, orbit->semi_minor_axis, 1.0f));

    std::vector<float> data;
    data.resize(3 * this->length);
    size_t i = 0;
    for (int j = 0; j < this->length; j += 1) {
        float theta = M_PIf32 * (2.f * float(j) / float(this->length) - 1.f);
        auto v = glm::vec4{cosf(theta), sinf(theta), 0.f, 1.f};
        v = transform * v;
        data[i++] = v[0];
        data[i++] = v[1];
        data[i++] = v[2];
    }

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, i * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

OrbitApsesMesh::OrbitApsesMesh(Orbit* orbit) :
    Mesh(GL_POINTS, 2, false)
{
    auto transform = glm::mat4(1.0f);
    transform = glm::rotate(transform, float(orbit->longitude_of_ascending_node), glm::vec3(0.f, 0.f, 1.f));
    transform = glm::rotate(transform, float(orbit->inclination),                 glm::vec3(1.f, 0.f, 0.f));
    transform = glm::rotate(transform, float(orbit->argument_of_periapsis),       glm::vec3(0.f, 0.f, 1.f));
    transform = glm::translate(transform, glm::vec3(-orbit->focus, 0.f, 0.f));
    transform = glm::scale(transform, glm::vec3(orbit->semi_major_axis, orbit->semi_minor_axis, 1.0f));

    auto periapsis = transform * glm::vec4(+1.f, 0.f, 0.f, 1.f);
    auto apoapsis  = transform * glm::vec4(-1.f, 0.f, 0.f, 1.f);

    float data[] = {
        periapsis[0], periapsis[1], periapsis[2],
         apoapsis[0],  apoapsis[1],  apoapsis[2],
    };

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

FocusedOrbitMesh::FocusedOrbitMesh(Orbit* orbit, double time) :
    // TODO: mode = GL_LINE_LOOP if orbit.eccentricity < 1. else GL_LINE_STRIP
    Mesh(GL_LINE_LOOP, 256, false)
{
    std::vector<float> data;
    data.resize(3 * this->length);
    size_t i = 0;

    // issues when drawing the orbit of a focused body:
    // 1. moving to system center and back close to camera induces
    //    loss of significance and produces jitter
    // 2. drawing the orbit as segments may put the body visibly out
    //    of the line when zooming in
    // 3. line breaks may be visible close to the camera

    // draw the orbit from the body rather than from the orbit focus (1.)
    if (orbit->eccentricity >= 1.) {  // open orbits
        // TODO
    } else {  // closed orbits
        // TODO: use circle symetry hack to avoid regenerating the mesh
        // TODO: can the hack be adapted for parabolic and hyperbolic orbits?

        // make tilted ellipse from a circle; and rotate at current anomaly
        double mean_anomaly = orbit_mean_anomaly_at_time(orbit, time);
        double eccentric_anomaly = orbit_eccentric_anomaly_at_mean_anomaly(orbit, mean_anomaly);

        auto transform = glm::mat4(1.0f);
        transform = glm::rotate(transform, float(orbit->longitude_of_ascending_node), glm::vec3(0.f, 0.f, 1.f));
        transform = glm::rotate(transform, float(orbit->inclination),                 glm::vec3(1.f, 0.f, 0.f));
        transform = glm::rotate(transform, float(orbit->argument_of_periapsis),       glm::vec3(0.f, 0.f, 1.f));
        // NOTE: do not translate
        transform = glm::scale(transform, glm::vec3(orbit->semi_major_axis, orbit->semi_minor_axis, 1.0f));
        transform = glm::rotate(transform, float(eccentric_anomaly - M_PI), glm::vec3(0.f, 0.f, 1.f));

        // the first point of circle_through_origin is (0,0) (2.)
        // more points are located near the origin (3.)
        for (int point = 0; point < this->length; point += 1) {
            // TODO: use lerp
            float x = 2.f * float(point) / float(this->length) - 1.f;
            float theta = M_PIf32 * powf(x, 3.f);
            glm::vec3 pos = transform * glm::vec4(1.f - cosf(theta), sinf(theta), 0.f, 0.f);
            data[i++] = pos[0];
            data[i++] = pos[1];
            data[i++] = pos[2];
        }
    }

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, i * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

FocusedOrbitApsesMesh::FocusedOrbitApsesMesh(Orbit* orbit, double time) :
    Mesh(GL_POINTS, 2, false)
{
    auto focus_offset = orbit_position_at_time(orbit, time);
    auto periapsis = orbit_position_at_true_anomaly(orbit, 0.) - focus_offset;
    auto apoapsis  = orbit_position_at_true_anomaly(orbit, M_PI) - focus_offset;

    float data[] = {
        (float) periapsis[0], (float) periapsis[1], (float) periapsis[2],
        (float)  apoapsis[0], (float)  apoapsis[1], (float)  apoapsis[2],
    };

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

}

static void append_object_and_children_coordinates(std::vector<float>& positions, const vec3& scene_origin, double time, CelestialBody* body) {
    auto pos = body_global_position_at_time(body, time) - scene_origin;
    positions.push_back((float) pos[0]);
    positions.push_back((float) pos[1]);
    positions.push_back((float) pos[2]);
    for (size_t i = 0; i < body->n_satellites; i += 1) {
        append_object_and_children_coordinates(positions, scene_origin, time, body->satellites[i]);
    }
}

OrbitSystem::OrbitSystem(CelestialBody* root, const vec3& scene_origin, double time) :
    // TODO: mode = GL_LINE_LOOP if orbit.eccentricity < 1. else GL_LINE_STRIP
    Mesh(GL_POINTS, 0, false)
{
    std::vector<float> data;
    append_object_and_children_coordinates(data, scene_origin, time, root);

    this->length = (int) data.size();

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
