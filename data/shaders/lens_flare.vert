#version 150 core

// Uniforms
uniform mat4 model_view_projection_matrix;
uniform vec3 u_light_source;
uniform vec2 u_dims;
uniform float u_intensity;

// Input
in vec3 v_position;
in float v_offset;

// Output
varying vec3 f_position;
varying float f_intensity;

void lens_flare() {
    f_position = v_position;

    // Fixed size billboard
    // Get the screen-space position of the light source
    gl_Position = model_view_projection_matrix * vec4(u_light_source, 1.);
    gl_Position /= gl_Position.w;

    // Get the vector from the light source to the center
    vec2 offsetVec = vec2(0.0) - gl_Position.xy;

    // Calculate the intensity, which is basically the alpha
    f_intensity = max(0.0, 1.0 - length(offsetVec) / 1.0) * u_intensity;

    // Move the vertex in screen space.
    gl_Position.xy += v_position.xy * u_dims + offsetVec * pow(v_offset, 2.0) * 0.5;
}
