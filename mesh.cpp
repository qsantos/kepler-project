#include "mesh.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

#include <cmath>
#include <vector>

UVSphereMesh::UVSphereMesh(float radius, GLsizei stacks, GLsizei slices) :
    components{8},
    length{2 * (slices + 2) * stacks}
{
    std::vector<float> data;
    data.resize(this->components * this->length);

    size_t i = 0;
    for (GLsizei stack = 0; stack < stacks ; stack += 1) {

        {
            GLsizei slice = 0;
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

        for (GLsizei slice = 0; slice <= slices; slice += 1) {
            float slice_angle = (2.f * M_PIf32) * float(slice) / float(slices);

            {
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

            {
                float stack_angle = M_PIf32 * float(stack+1) / float(stacks);
                float nx = sinf(stack_angle) * sinf(slice_angle);
                float ny = sinf(stack_angle) * cosf(slice_angle);
                float nz = cosf(stack_angle);
                // position
                data[i++] = radius * nx;
                data[i++] = radius * ny;
                data[i++] = radius * nz;
                // texcoord
                data[i++] = 1 - float(slice) / float(slices);
                data[i++] = 1 - float(stack+1) / float(stacks);
                //normal
                data[i++] = nx;
                data[i++] = ny;
                data[i++] = nz;
            }
        }

        {
            GLsizei slice = slices;
            float slice_angle = (2.f * M_PIf32) * float(slice) / float(slices);
            float stack_angle = M_PIf32 * float(stack+1) / float(stacks);
            float nx = sinf(stack_angle) * sinf(slice_angle);
            float ny = sinf(stack_angle) * cosf(slice_angle);
            float nz = cosf(stack_angle);
            // position
            data[i++] = radius * nx;
            data[i++] = radius * ny;
            data[i++] = radius * nz;
            // texcoord
            data[i++] = 1 - float(slice) / float(slices);
            data[i++] = 1 - float(stack+1) / float(stacks);
            //normal
            data[i++] = nx;
            data[i++] = ny;
            data[i++] = nz;
        }
    }

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, i * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void UVSphereMesh::bind(void) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);

    GLint var = glGetAttribLocation(program, "v_position");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, this->components * (GLsizei) sizeof(float), NULL);
    }

    var = glGetAttribLocation(program, "v_texcoord");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 2, GL_FLOAT, GL_FALSE, this->components * (GLsizei) sizeof(float), (GLvoid*)(3 * sizeof(float)));
    }

    var = glGetAttribLocation(program, "v_normal");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, this->components * (GLsizei) sizeof(float), (GLvoid*)(5 * sizeof(float)));
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void UVSphereMesh::draw(void) {
    this->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, this->length);
}

OrbitMesh::OrbitMesh(Orbit* orbit) :
    components{3},
    length{256}
{
    auto transform = glm::mat4(1.0f);
    transform = glm::rotate(transform, float(orbit->longitude_of_ascending_node), glm::vec3(0.f, 0.f, 1.f));
    transform = glm::rotate(transform, float(orbit->inclination),                 glm::vec3(1.f, 0.f, 0.f));
    transform = glm::rotate(transform, float(orbit->argument_of_periapsis),       glm::vec3(0.f, 0.f, 1.f));
    transform = glm::translate(transform, glm::vec3(-orbit->focus, 0.f, 0.f));
    transform = glm::scale(transform, glm::vec3(orbit->semi_major_axis, orbit->semi_minor_axis, 1.0f));

    std::vector<float> data;
    data.resize(this->components * this->length);
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

void OrbitMesh::bind(void) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);

    GLint var = glGetAttribLocation(program, "v_position");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, this->components * (GLsizei) sizeof(float), NULL);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OrbitMesh::draw(void) {
    this->bind();
    glDrawArrays(GL_LINE_LOOP, 0, this->length);
}

CubeMesh::CubeMesh(double size) :
    components{3},
    length{36}
{

    float s = (float) size;

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

void CubeMesh::bind(void) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);

    GLint var = glGetAttribLocation(program, "v_position");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, this->components * (GLsizei) sizeof(float), NULL);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CubeMesh::draw(void) {
    this->bind();
    glDrawArrays(GL_TRIANGLES, 0, this->length);
}
