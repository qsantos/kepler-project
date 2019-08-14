#version 330 core

uniform vec4 u_color;
uniform samplerCube cubemap_texture;

varying vec3 f_cubemap_position;

void cubemap() {
    gl_FragColor = u_color * textureCube(cubemap_texture, f_cubemap_position);
}
