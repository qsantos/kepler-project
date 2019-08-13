#version 150 core

varying vec2 f_texcoord;

uniform sampler2D Texture0;
uniform vec4 u_color;

void base(void) {
    gl_FragColor = u_color * texture2D(Texture0, f_texcoord);
    if (gl_FragColor.a == 0.) {
        discard;
    }
}
