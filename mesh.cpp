#include "mesh.hpp"

extern "C" {
#include "util.h"
#include "logging.h"
}

#ifdef MSYS2
#include <windef.h>
#endif
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

#include <vector>

Mesh::Mesh(unsigned mode_, int length_, bool is_3d_) :
    mode{mode_},
    length{length_},
    is_3d{is_3d_}
{
    glGenBuffers(1, &this->vbo);
}

Mesh::~Mesh(void) {
    glDeleteBuffers(1, &this->vbo);
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

        var = glGetAttribLocation(program, "v_texcoord");
        if (var >= 0) {
            glDisableVertexAttribArray(var);
        }

        var = glGetAttribLocation(program, "v_normal");
        if (var >= 0) {
            glDisableVertexAttribArray(var);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::draw(void) {
    this->bind();
    glDrawArrays(this->mode, 0, this->length);
}

RectMesh::RectMesh(double width, double height) :
    Mesh(GL_TRIANGLE_STRIP, 4, true)
{
    float w = (float) width / 2.f;
    float h = (float) height / 2.f;

    float data[] = {
        -w, -h, 0, 0, 0, 0, 0, -1,
        +w, -h, 0, 1, 0, 0, 0, -1,
        -w, +h, 0, 0, 1, 0, 0, -1,
        +w, +h, 0, 1, 1, 0, 0, -1,
    };

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

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void uvspheremesh_add_vertex(std::vector<float>& data, float radius, int stacks, int slices, int stack, int slice) {
    float slice_angle = (2.f * M_PIf32) * float(slice) / float(slices);
    float stack_angle = M_PIf32 * float(stack) / float(stacks);

    float nx = sinf(stack_angle) * sinf(slice_angle);
    float ny = sinf(stack_angle) * cosf(slice_angle);
    float nz = cosf(stack_angle);

    // position
    data.push_back(radius * nx);
    data.push_back(radius * ny);
    data.push_back(radius * nz);
    // texcoord
    data.push_back(1 - float(slice) / float(slices));
    data.push_back(1 - float(stack) / float(stacks));
    // normal
    data.push_back(nx);
    data.push_back(ny);
    data.push_back(nz);
}

UVSphereMesh::UVSphereMesh(float radius, int lod) :
    Mesh(GL_TRIANGLE_STRIP, 0, true)
{
    int stacks = 2 << lod;
    int slices = 4 << lod;

    std::vector<float> data;
    for (GLsizei stack = 0; stack < stacks ; stack += 1) {
        uvspheremesh_add_vertex(data, radius, stacks, slices, stack, 0);
        for (GLsizei slice = 0; slice <= slices; slice += 1) {
            uvspheremesh_add_vertex(data, radius, stacks, slices, stack, slice);
            uvspheremesh_add_vertex(data, radius, stacks, slices, stack + 1, slice);
        }
        uvspheremesh_add_vertex(data, radius, stacks, slices, stack + 1, slices);
    }

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    this->length = (int) data.size() / 8;
}

static void quadspheremesh_add_vertex(std::vector<float>& data, float radius, int divisions, int face, int xdivision, int ydivision) {
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
    data.push_back(x * radius);
    data.push_back(y * radius);
    data.push_back(z * radius);
    // texcoord
    data.push_back(0.f);  // atan2(y, x) / (2.f * M_PIf32) + .5f
    data.push_back(0.f);  // asin(z) / M_PIf32 + .5f
    // normal
    data.push_back(x);
    data.push_back(y);
    data.push_back(z);
}

QuadSphereMesh::QuadSphereMesh(float radius, int lod) :
    Mesh(GL_TRIANGLE_STRIP, 0, true)
{
    int divisions = 1 << lod;

    // make a face of divisions×divisions squares
    std::vector<float> data;
    for (int face = 0; face < 6; face += 1) {
        for (int ydivision = 0; ydivision < divisions; ydivision += 1) {
            quadspheremesh_add_vertex(data, radius, divisions, face, 0, ydivision);
            for (int xdivision = 0; xdivision <= divisions; xdivision += 1) {
                quadspheremesh_add_vertex(data, radius, divisions, face, xdivision, ydivision);
                quadspheremesh_add_vertex(data, radius, divisions, face, xdivision, ydivision + 1);
            }
            quadspheremesh_add_vertex(data, radius, divisions, face, divisions, ydivision + 1);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    this->length = (int) data.size() / 8;
}

glm::vec3 midpoint(glm::vec3 a, glm::vec3 b) {
    return glm::normalize((a + b) / 2.f);
}

IcoSphereMesh::IcoSphereMesh(float radius, int lod) :
    Mesh(GL_TRIANGLES, 0, true)
{
    float t = (1.f + sqrtf(5.f)) / 2.f;

    static const glm::vec3 base_vertices[12] = {
        {-1,  t,  0,},
        { 1,  t,  0,},
        {-1, -t,  0,},
        { 1, -t,  0,},

        { 0, -1,  t,},
        { 0,  1,  t,},
        { 0, -1, -t,},
        { 0,  1, -t,},

        { t,  0, -1,},
        { t,  0,  1,},
        {-t,  0, -1,},
        {-t,  0,  1,},
    };

    static const int base_triangles[20][3] = {
        { 0, 11,  5},
        { 0,  5,  1},
        { 0,  1,  7},
        { 0,  7, 10},
        { 0, 10, 11},

        { 1,  5,  9},
        { 5, 11,  4},
        {11, 10,  2},
        {10,  7,  6},
        { 7,  1,  8},

        { 3,  9,  4},
        { 3,  4,  2},
        { 3,  2,  6},
        { 3,  6,  8},
        { 3,  8,  9},

        { 4,  9,  5},
        { 2,  4, 11},
        { 6,  2, 10},
        { 8,  6,  7},
        { 9,  8,  1},
    };

    std::vector<glm::mat3> triangles;
    for (int i = 0; i < 20; i += 1) {
        glm::mat3 triangle;
        for (int j = 0; j < 3; j += 1) {
            triangle[j] = glm::normalize(base_vertices[base_triangles[i][j]]);
        }
        triangles.push_back(triangle);
    }

    std::vector<glm::mat3> new_triangles;
    for (int level = 0; level < lod; level += 1) {
        for (auto triangle : triangles) {
            auto a = midpoint(triangle[0], triangle[1]);
            auto b = midpoint(triangle[1], triangle[2]);
            auto c = midpoint(triangle[2], triangle[0]);

            new_triangles.push_back(glm::mat3(triangle[0], a, c));
            new_triangles.push_back(glm::mat3(triangle[1], b, a));
            new_triangles.push_back(glm::mat3(triangle[2], c, b));
            new_triangles.push_back(glm::mat3(a, b, c));
        }
        triangles.swap(new_triangles);
        new_triangles.clear();
    }

    std::vector<float> data;
    for (auto triangle : triangles) {
        for (int k = 0; k < 3; k += 1) {
            auto vertex = triangle[k];
            // position
            data.push_back(vertex[0] * radius);
            data.push_back(vertex[1] * radius);
            data.push_back(vertex[2] * radius);
            // tex coord
            data.push_back(0.f);
            data.push_back(0.f);
            // normal
            data.push_back(vertex[0]);
            data.push_back(vertex[1]);
            data.push_back(vertex[2]);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    this->length = (int) data.size() / 8;
}

OrbitMesh::OrbitMesh(Orbit* orbit, double time, bool focused) :
    Mesh(GL_LINE_STRIP, 0, false)
{
    // issues when drawing the orbit of a focused body:
    // 1. moving to system center and back close to camera induces
    //    loss of significance and produces jitter
    // 2. drawing the orbit as segments may put the body visibly out
    //    of the line when zooming in
    // 3. line breaks may be visible close to the camera

    // draw the orbit from the body rather than from the orbit focus (1.)
    glm::dvec3 offset_from_focus{0, 0, 0};
    if (focused) {
        offset_from_focus = orbit_position_at_time(orbit, time);
    }

    std::vector<float> data;

    if (orbit->apoapsis > orbit->primary->sphere_of_influence || orbit->eccentricity > 1.) {  // escaping orbit
        double object_mean_anomaly = orbit_mean_anomaly_at_time(orbit, time);
        double object_eccentric_anomaly = orbit_eccentric_anomaly_at_mean_anomaly(orbit, object_mean_anomaly);
        double object_true_anomaly = orbit_true_anomaly_at_eccentric_anomaly(orbit, object_eccentric_anomaly);

        // bring to [-PI, PI]
        object_true_anomaly = fmod2(object_true_anomaly, 2 * M_PI);
        if (object_true_anomaly > M_PI) {
            object_true_anomaly -= 2 * M_PI;
        }

        // stop drawing at SoI
        // TODO: handle no sphere of influence
        double escape_true_anomaly = orbit_true_anomaly_at_escape(orbit);

        // point at object
        {
            // TODO: we can probably do better precision-wise
            auto pos = orbit_position_at_true_anomaly(orbit, object_true_anomaly) - offset_from_focus;
            data.push_back((float) pos[0]);
            data.push_back((float) pos[1]);
            data.push_back((float) pos[2]);
        }

        // ensure the body will be on the line (2.)
        // more points close to the camera (3.)
        size_t n_points = 64;
        for (size_t i = 1; i < n_points; i += 1) {
            double t = (double) i / (double) n_points;
            double true_anomaly = lerp(object_true_anomaly, escape_true_anomaly, t);

            auto pos = orbit_position_at_true_anomaly(orbit, true_anomaly) - offset_from_focus;
            data.push_back((float) pos[0]);
            data.push_back((float) pos[1]);
            data.push_back((float) pos[2]);
        }

        // point at escape
        {
            auto pos = orbit_position_at_escape(orbit) - offset_from_focus;
            data.push_back((float) pos[0]);
            data.push_back((float) pos[1]);
            data.push_back((float) pos[2]);
        }

        this->mode = GL_LINE_STRIP;
    } else {  // non-escaping closed orbit
        double mean_anomaly = orbit_mean_anomaly_at_time(orbit, time);
        double eccentric_anomaly = orbit_eccentric_anomaly_at_mean_anomaly(orbit, mean_anomaly);

        auto transform = glm::mat4(1.0f);
        transform = glm::rotate(transform, float(orbit->longitude_of_ascending_node), glm::vec3(0.f, 0.f, 1.f));
        transform = glm::rotate(transform, float(orbit->inclination),                 glm::vec3(1.f, 0.f, 0.f));
        transform = glm::rotate(transform, float(orbit->argument_of_periapsis),       glm::vec3(0.f, 0.f, 1.f));
        if (!focused) {
            transform = glm::translate(transform, glm::vec3(-orbit->focus, 0.f, 0.f));
        }
        transform = glm::scale(transform, glm::vec3(orbit->semi_major_axis, orbit->semi_minor_axis, 1.0f));
        if (focused) {
            transform = glm::rotate(transform, float(eccentric_anomaly - M_PI), glm::vec3(0.f, 0.f, 1.f));
        }

        size_t n_points = 256;
        for (size_t j = 0; j < n_points; j += 1) {
            glm::vec4 v;
            if (focused) {
                // the first point of circle_through_origin is (0,0) (2.)
                // more points are located near the origin (3.)
                float x = 2.f * float(j) / float(n_points) - 1.f;
                float theta = M_PIf32 * powf(x, 3.f);
                v = glm::vec4(1.f - cosf(theta), sinf(theta), 0.f, 0.f);
            } else {
                float theta = M_PIf32 * (2.f * float(j) / float(n_points) - 1.f);
                v = glm::vec4{cosf(theta), sinf(theta), 0.f, 1.f};
            }

            v = transform * v;
            data.push_back(v[0]);
            data.push_back(v[1]);
            data.push_back(v[2]);
        }

        this->mode = GL_LINE_LOOP;
    }

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    this->length = (int) data.size() / 3;
}

OrbitApsesMesh::OrbitApsesMesh(Orbit* orbit, double time, bool focused) :
    Mesh(GL_POINTS, 0, false)
{
    if (orbit->eccentricity < 5e-4) {  // almost circular orbit
        return;
    }

    auto periapsis = orbit_position_at_true_anomaly(orbit, 0.);
    auto apoapsis  = orbit_position_at_true_anomaly(orbit, M_PI);

    if (focused) {
        auto offset_from_focus = orbit_position_at_time(orbit, time);
        periapsis -= offset_from_focus;
        apoapsis -= offset_from_focus;
    }

    std::vector<float> data;

    if (orbit->apoapsis > orbit->primary->sphere_of_influence || orbit->eccentricity > 1.) {  // escaping orbit
        double mean_anomaly = orbit_mean_anomaly_at_time(orbit, time);
        double eccentric_anomaly = orbit_eccentric_anomaly_at_mean_anomaly(orbit, mean_anomaly);
        double true_anomaly = orbit_true_anomaly_at_eccentric_anomaly(orbit, eccentric_anomaly);

        // only show periapsis if not reached yet
        if (true_anomaly < 0.) {
            data.push_back((float) periapsis[0]);
            data.push_back((float) periapsis[1]);
            data.push_back((float) periapsis[2]);
        }
    } else {  // non-escaping closed orbit
        data.push_back((float) periapsis[0]);
        data.push_back((float) periapsis[1]);
        data.push_back((float) periapsis[2]);

        data.push_back((float) apoapsis[0]);
        data.push_back((float) apoapsis[1]);
        data.push_back((float) apoapsis[2]);
    }

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    this->length = (int) data.size() / 3;
}

static void append_object_and_children_coordinates(std::vector<float>& positions, const glm::dvec3& scene_origin, double time, CelestialBody* body) {
    auto pos = body_global_position_at_time(body, time) - scene_origin;
    positions.push_back((float) pos[0]);
    positions.push_back((float) pos[1]);
    positions.push_back((float) pos[2]);
    for (size_t i = 0; i < body->n_satellites; i += 1) {
        if (body->satellites[i] == body) {
            CRITICAL("'%s' is its own satellite", body->name);
            exit(EXIT_FAILURE);
        }
        append_object_and_children_coordinates(positions, scene_origin, time, body->satellites[i]);
    }
}

OrbitSystem::OrbitSystem(CelestialBody* root, const glm::dvec3& scene_origin, double time) :
    // TODO: mode = GL_LINE_LOOP if orbit.eccentricity < 1. else GL_LINE_STRIP
    Mesh(GL_POINTS, 0, false)
{
    std::vector<float> data;
    append_object_and_children_coordinates(data, scene_origin, time, root);

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    this->length = (int) data.size() / 3;
}
