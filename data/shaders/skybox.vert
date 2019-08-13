#version 110

attribute vec3 v_position;

varying vec3 cubemap_dir;

void cubemap() {
    cubemap_dir = normalize(v_position);
}
