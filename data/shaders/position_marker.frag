#version 330 core

void position_marker() {
    // draw a circle around center
    vec2 center = vec2(.5, .5);
    float d = distance(gl_PointCoord, center);
    if (!(.4 <= d && d <= .5)) {
        discard;
    }
}
