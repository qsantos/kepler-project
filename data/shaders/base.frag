#version 330 core

uniform sampler2D Texture0;
uniform vec4 u_color;

in vec2 f_texcoord;

void base(void) {
    gl_FragColor = u_color * texture2D(Texture0, f_texcoord);
    if (gl_FragColor.a == 0.) {
        discard;
    }
}
