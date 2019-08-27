#version 330 core

uniform mat4 cubemap_matrix;

in vec3 v_position;

out vec3 f_cubemap_position;

void cubemap() {
    f_cubemap_position = vec3(cubemap_matrix * vec4(v_position, 1.));
}
