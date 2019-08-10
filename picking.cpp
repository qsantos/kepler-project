#include "picking.hpp"

void set_picking_name(size_t name) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glUniform3f(
        glGetUniformLocation(program, "picking_name"),
        float((name >> 16) & 0xff) / 255.f,
        float((name >>  8) & 0xff) / 255.f,
        float((name >>  0) & 0xff) / 255.f
    );
}

void set_picking_object(RenderState* state, CelestialBody* object) {
    if (!state->picking_active) {
        return;
    }

    state->picking_objects.push_back(object);
    set_picking_name(state->picking_objects.size());
}

void clear_picking_object(RenderState* state) {
    if (!state->picking_active) {
        return;
    }
    set_picking_name(0);
}

CelestialBody* pick(RenderState* state) {
    // render with picking activated
    state->picking_active = true;
    glUseProgram(state->position_marker_shader);
    glUniform1i(glGetUniformLocation(state->position_marker_shader, "picking_active"), 1);
    glUseProgram(state->lighting_shader);
    glUniform1i(glGetUniformLocation(state->lighting_shader, "picking_active"), 1);
    glUseProgram(state->base_shader);
    glUniform1i(glGetUniformLocation(state->base_shader, "picking_active"), 1);

    glDisable(GL_MULTISAMPLE);
    render(state);
    glEnable(GL_MULTISAMPLE);

    glUseProgram(state->base_shader);
    glUniform1i(glGetUniformLocation(state->base_shader, "picking_active"), 0);
    glUseProgram(state->lighting_shader);
    glUniform1i(glGetUniformLocation(state->lighting_shader, "picking_active"), 0);
    glUseProgram(state->position_marker_shader);
    glUniform1i(glGetUniformLocation(state->position_marker_shader, "picking_active"), 0);
    state->picking_active = false;

    // search names in color components
    const int search_radius = 20;

    int cx = (int) state->cursor_x;
    int cy = state->viewport_height - (int) state->cursor_y;

    int min_x = std::max(cx - search_radius, 0);
    int max_x = std::min(cx + search_radius, state->viewport_width - 1);
    int min_y = std::max(cy - search_radius, 0);
    int max_y = std::min(cy + search_radius, state->viewport_height - 1);

    int w = max_x - min_x + 1;
    int h = max_y - min_y + 1;

    unsigned char* components = new unsigned char[h * w * 4];
    glReadPixels(min_x, min_y, w, h, GL_RGBA,  GL_UNSIGNED_BYTE, components);

    size_t name = 0;
    for (int y = 0; y < h; y += 1) {
        for (int x = 0; x < w; x += 1) {
            size_t candidate_name = 0;
            candidate_name |= components[(y*w + x)*4 + 0] << 16;
            candidate_name |= components[(y*w + x)*4 + 1] <<  8;
            candidate_name |= components[(y*w + x)*4 + 2] <<  0;
            if (candidate_name > 0) {
                name = candidate_name;
                break;
            }
        }
    }
    delete[] components;

    if (name == 0) {
        return NULL;
    }

    size_t n_objects = state->picking_objects.size();
    if (name > n_objects) {
        fprintf(stderr, "WARNING: picked %zu but only %zu known objects\n", name, n_objects);
        return NULL;
    } else {
        return state->picking_objects[name - 1];
    }
}
