#version 110

uniform vec4 u_color;
uniform samplerCube cubemap_texture;
varying vec3 cubemap_dir;

void cubemap() {
    gl_FragColor = u_color * textureCube(cubemap_texture, cubemap_dir);
}
