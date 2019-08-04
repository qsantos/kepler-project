#version 120  // mat3

attribute vec3 v_position;
attribute vec3 v_normal;

varying vec3 lighting_vertex;
varying vec3 lighting_normal;
uniform mat4 model_view_matrix;

void lighting() {
    mat3 m = mat3(model_view_matrix);
    lighting_vertex = vec3(m * v_position);
    lighting_normal = normalize(m * v_normal);
}
