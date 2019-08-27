#version 330 core
#
uniform mat4 model_view_matrix;

in vec3 v_position;
in vec3 v_normal;

out vec3 lighting_vertex;
out vec3 lighting_normal;

void lighting() {
    lighting_vertex = vec3(model_view_matrix * vec4(v_position, 1.0));
    lighting_normal = normalize(mat3(model_view_matrix) * v_normal);
}
