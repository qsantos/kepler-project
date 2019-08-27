#version 330 core

uniform vec4 u_color;
uniform samplerCube skybox_texture;

in vec3 cubemap_dir;

void skybox() {
    gl_FragColor = u_color * texture(skybox_texture, cubemap_dir);
}
