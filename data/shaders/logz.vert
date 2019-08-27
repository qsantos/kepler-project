#version 330 core

out float flogz;

void logz(void) {
    // fix for depth buffer using logarithmic scale
    // http://outerra.blogspot.com/2013/07/logarithmic-depth-buffer-optimizations.html
    float farplane = 1e20;
    float Fcoef = 2.0 / log2(farplane + 1.0);
    gl_Position.z = log2(max(1e-6, 1.0 + gl_Position.w)) * Fcoef - 1.0;
    gl_Position.z *= gl_Position.w;
    flogz = 1.0 + gl_Position.w;
}
