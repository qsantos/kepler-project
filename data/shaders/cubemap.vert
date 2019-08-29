#version 330 core
#
uniform mat4 model_view_projection_matrix;
uniform mat4 cubemap_matrix;

in vec3 v_position;

out vec3 f_cubemap_position;

void cubemap() {
    gl_Position = model_view_projection_matrix * vec4(v_position, 1.0);
    f_cubemap_position = vec3(cubemap_matrix * vec4(v_position, 1.));
}
