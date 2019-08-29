#version 330 core

in float f_intensity;

out vec4 o_color;

void lens_flare() {
    o_color *= f_intensity;
}
