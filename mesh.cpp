#include "mesh.hpp"

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
            float angle_i = (2.f * M_PIf32) * float(slice) / float(slices);
            float angle_j = M_PIf32 * float(stack) / float(stacks);
            // position
            data[i++] = radius * sinf(angle_j) * sinf(angle_i);
            data[i++] = radius * sinf(angle_j) * cosf(angle_i);
            data[i++] = radius * cosf(angle_j);
            // texcoord
            data[i++] = 1 - float(slice) / float(slices);
            data[i++] = float(stack) / float(stacks);
            // normal
            data[i++] = sinf(angle_i) * sinf(angle_j);
            data[i++] = cosf(angle_i) * sinf(angle_j);
            data[i++] = cosf(angle_j);
        }

        for (GLsizei slice = 0; slice <= slices; slice += 1) {
            float angle_i = (2.f * M_PIf32) * float(slice) / float(slices);

            {
                float angle_j = M_PIf32 * float(stack) / float(stacks);
                // position
                data[i++] = radius * sinf(angle_j) * sinf(angle_i);
                data[i++] = radius * sinf(angle_j) * cosf(angle_i);
                data[i++] = radius * cosf(angle_j);
                // texcoord
                data[i++] = 1 - float(slice) / float(slices);
                data[i++] = 1 - float(stack) / float(stacks);
                // normal
                data[i++] = sinf(angle_i) * sinf(angle_j);
                data[i++] = cosf(angle_i) * sinf(angle_j);
                data[i++] = cosf(angle_j);
            }

            {
                float angle_j = M_PIf32 * float(stack+1) / float(stacks);
                // position
                data[i++] = radius * sinf(angle_j) * sinf(angle_i);
                data[i++] = radius * sinf(angle_j) * cosf(angle_i);
                data[i++] = radius * cosf(angle_j);
                // texcoord
                data[i++] = 1 - float(slice) / float(slices);
                data[i++] = 1 - float(stack+1) / float(stacks);
                //normal
                data[i++] = sinf(angle_i) * sinf(angle_j);
                data[i++] = cosf(angle_i) * sinf(angle_j);
                data[i++] = cosf(angle_j);
            }
        }

        {
            GLsizei slice = slices;
            float angle_i = (2.f * M_PIf32) * float(slice) / float(slices);
            float angle_j = M_PIf32 * float(stack+1) / float(stacks);
            // position
            data[i++] = radius * sinf(angle_j) * sinf(angle_i);
            data[i++] = radius * sinf(angle_j) * cosf(angle_i);
            data[i++] = radius * cosf(angle_j);
            // texcoord
            data[i++] = 1 - float(slice) / float(slices);
            data[i++] = 1 - float(stack+1) / float(stacks);
            // normal
            data[i++] = sinf(angle_i) * sinf(angle_j);
            data[i++] = cosf(angle_i) * sinf(angle_j);
            data[i++] = cosf(angle_j);
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
