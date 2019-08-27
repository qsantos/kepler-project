#version 330 core

in float f_intensity;

void lens_flare() {
    gl_FragColor *= f_intensity;
}
