#version 150 core

uniform sampler2D texture;

varying vec2 f_texcoord;

uniform vec3 f_color;

void setdefaults(void) {
    vec4 texColor = texture2D(texture, f_texcoord);
    gl_FragColor = vec4(f_color, 1.) + texColor;
    // gl_FragColor = gl_Color * texColor;
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
