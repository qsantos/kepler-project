#version 330 core

uniform bool picking_active;
uniform vec3 picking_name;

out vec4 o_color;

void picking() {
    if (picking_active) {
        if (o_color.a != 0.) {
            o_color = vec4(picking_name, 1.);
        }
    }
}
