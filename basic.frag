#version 150 core

uniform sampler2D Texture0;

varying vec2 f_texcoord;

uniform vec4 u_color;

void setdefaults(void) {
    gl_FragColor = u_color * texture2D(Texture0, f_texcoord);
}

varying float flogz;

void logdepth(void) {
    // fixed depth
    // http://outerra.blogspot.com/2013/07/logarithmic-depth-buffer-optimizations.html
    float farplane = 1e20;
    float Fcoef = 2.0 / log2(farplane + 1.0);
    float Fcoef_half = 0.5 * Fcoef;
    gl_FragDepth = log2(flogz) * Fcoef_half;
}

void main() {
    setdefaults();
    logdepth();
}
