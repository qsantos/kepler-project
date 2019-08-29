#version 330 core

uniform sampler2D Texture0;
uniform vec4 u_color;

in vec2 f_texcoord;

out vec4 o_color;

void base(void) {
    o_color = u_color * texture2D(Texture0, f_texcoord);
    if (o_color.a == 0.) {
        discard;
    }
}
