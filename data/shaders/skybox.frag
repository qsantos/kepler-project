#version 330 core

uniform vec4 u_color;
uniform samplerCube skybox_texture;

in vec3 cubemap_dir;

out vec4 o_color;

void skybox() {
    o_color = u_color * texture(skybox_texture, cubemap_dir);
}
