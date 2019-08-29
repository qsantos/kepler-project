#version 330 core

uniform mat4 model_view_projection_matrix;

in vec3 v_position;

out vec3 cubemap_dir;

void skybox() {
    gl_Position = model_view_projection_matrix * vec4(v_position, 1.0);
    cubemap_dir = normalize(v_position);
}
