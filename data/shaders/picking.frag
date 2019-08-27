#version 330 core

uniform bool picking_active;
uniform vec3 picking_name;

void picking() {
    if (picking_active) {
        if (gl_FragColor.a != 0.) {
            gl_FragColor = vec4(picking_name, 1.);
        }
    }
}
