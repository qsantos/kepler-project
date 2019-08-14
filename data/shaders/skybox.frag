#version 110

uniform vec4 u_color;
uniform samplerCube skybox_texture;
varying vec3 cubemap_dir;

void skybox() {
    gl_FragColor = u_color * textureCube(skybox_texture, cubemap_dir);
}
