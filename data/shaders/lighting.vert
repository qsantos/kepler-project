#version 120  // mat3

attribute vec3 v_position;
attribute vec3 v_normal;

varying vec3 lighting_vertex;
varying vec3 lighting_normal;
uniform mat4 model_view_matrix;

void lighting() {
    lighting_vertex = vec3(model_view_matrix * vec4(v_position, 1.0));
    lighting_normal = normalize(mat3(model_view_matrix) * v_normal);
}
