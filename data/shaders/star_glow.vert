#version 330 core

uniform mat4 model_view_projection_matrix;
uniform float visibility;
uniform vec2 star_glow_size;
uniform vec3 star_glow_position;
uniform vec3 camera_right;
uniform vec3 camera_up;

in vec3 v_position;

out vec2 f_position;

void star_glow(void) {
    gl_Position = model_view_projection_matrix * vec4(
        star_glow_position
        + v_position.x * star_glow_size.x * camera_right
        + v_position.y * star_glow_size.y * camera_up
    , 1.);
    f_position = v_position.xy;
}
