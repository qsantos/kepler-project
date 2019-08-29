#version 330 core

in float flogz;

void logz(void) {
    // fix for depth buffer using logarithmic scale
    // http://outerra.blogspot.com/2013/07/logarithmic-depth-buffer-optimizations.html
    float farplane = 1e20;
    float Fcoef = 2.0 / log2(farplane + 1.0);
    float Fcoef_half = 0.5 * Fcoef;
    gl_FragDepth = log2(flogz) * Fcoef_half;
}
