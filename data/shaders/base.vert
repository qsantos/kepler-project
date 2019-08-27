#version 330 core

uniform vec4 u_color;
uniform mat4 model_view_projection_matrix;

in vec3 v_position;
in vec3 v_normal;
in vec2 v_texcoord;

out vec2 f_texcoord;

void base(void) {
    gl_Position = model_view_projection_matrix * vec4(v_position, 1.0);
    f_texcoord = v_texcoord;
}
