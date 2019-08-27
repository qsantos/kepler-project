#version 330 core

in vec3 v_position;

out vec3 cubemap_dir;

void skybox() {
    cubemap_dir = normalize(v_position);
}
