#version 150 core

in vec3 v_position;
in vec2 v_texcoord;

uniform mat4 model_view_projection_matrix;
uniform mat4 projection_matrix;

void billboard() {
	vec4 position = model_view_projection_matrix * vec4(v_position, 1.);
	vec4 offset = projection_matrix * vec4(v_texcoord, 0, 0);
	gl_Position = position + offset / 10.;
}