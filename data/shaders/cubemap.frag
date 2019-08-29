#version 330 core

uniform vec4 u_color;
uniform samplerCube cubemap_texture;

in vec3 f_cubemap_position;

out vec4 o_color;

void cubemap() {
    o_color = u_color * texture(cubemap_texture, f_cubemap_position);
}
