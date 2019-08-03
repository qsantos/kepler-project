#version 150 core

in vec3 v_position;
in vec3 v_normal;
in vec2 v_texcoord;

uniform vec4 u_color;

varying vec2 f_texcoord;

uniform mat4 model_view_projection_matrix;
uniform mat4 model_view_matrix;

void setdefaults(void) {
    gl_Position = model_view_projection_matrix * vec4(v_position, 1.0);
    f_texcoord = v_texcoord;
}

varying float flogz; // fixed depth

void logdepth(void) {
    // fixed depth
    // http://outerra.blogspot.com/2013/07/logarithmic-depth-buffer-optimizations.html
    float farplane = 1e20;
    float Fcoef = 2.0 / log2(farplane + 1.0);
    gl_Position.z = log2(max(1e-6, 1.0 + gl_Position.w)) * Fcoef - 1.0;
    flogz = 1.0 + gl_Position.w;
}

void main() {
    setdefaults();
    logdepth();
}
