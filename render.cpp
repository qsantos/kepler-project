#include "render.hpp"

extern "C" {
#include "version.h"
#include "util.h"
}

#include "picking.hpp"

const time_t J2000 = 946728000UL;  // 2000-01-01T12:00:00Z

void update_matrices(GlobalState* state) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto model_view = state->view_matrix * state->model_matrix;
    GLint var = glGetUniformLocation(program, "model_view_matrix");
    if (var >= 0) {
        glUniformMatrix4fv(var, 1, GL_FALSE, glm::value_ptr(model_view));
    }

    auto model_view_projection = state->projection_matrix * model_view;
    var = glGetUniformLocation(program, "model_view_projection_matrix");
    if (var >= 0) {
        glUniformMatrix4fv(var, 1, GL_FALSE, glm::value_ptr(model_view_projection));
    }
}

void reset_matrices(GlobalState* state, bool zoom) {
    state->model_matrix = glm::mat4(1.0f);

    glm::mat4 view = glm::mat4(1.0f);
    if (zoom) {
        view = glm::translate(view, glm::vec3(0.f, 0.f, -1.f / state->view_zoom));
    }
    view = glm::rotate(view, float(glm::radians(state->view_phi)), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, float(glm::radians(state->view_theta)), glm::vec3(0.0f, 0.0f, 1.0f));
    state->view_matrix = view;

    float aspect = float(state->viewport_width) / float(state->viewport_height);
    state->projection_matrix = glm::perspective(glm::radians(45.0f), aspect, .1f, 1e7f);

    update_matrices(state);
}

static bool is_ancestor_of(CelestialBody* candidate, CelestialBody* target) {
    if (candidate == target) {
        return true;
    }

    while (target->orbit != NULL) {
        target = target->orbit->primary;
        if (candidate == target) {
            return true;
        }
    }

    return false;
}

static void render_skybox(GlobalState* state) {
    if (state->picking_active) {
        return;
    }

    glUseProgram(state->cubemap_shader);
    reset_matrices(state, false);

    glDisable(GL_DEPTH_TEST);
    state->skybox.draw();
    glEnable(GL_DEPTH_TEST);
}

static void render_body(GlobalState* state, CelestialBody* body, const vec3& scene_origin) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto model = glm::mat4(1.f);
    auto position = body_global_position_at_time(body, state->time) - scene_origin;
    model = glm::translate(model, glm::vec3(position[0], position[1], position[2]));
    model = glm::scale(model, glm::vec3(float(body->radius)));

    // axial tilt
    if (body->north_pole != NULL) {
        double z_angle = body->north_pole->ecliptic_longitude - M_PI / 2.;
        model = glm::rotate(model, float(z_angle), glm::vec3(0.f, 0.f, 1.f));
        double x_angle = body->north_pole->ecliptic_latitude - M_PI / 2.;
        model = glm::rotate(model, float(x_angle), glm::vec3(1.f, 0.f, 0.f));
    }

    // OpenGL use single precision while Python has double precision
    // reducing modulo 2 PI in Python reduces loss of significance
    double turn_fraction = fmod(state->time / body->sidereal_day, 1.);
    model = glm::rotate(model, 2.f * M_PIf32 * float(turn_fraction), glm::vec3(0.f, 0.f, 1.f));

    state->model_matrix = model;
    update_matrices(state);

    auto texture = state->body_textures.at(body->name);
    glBindTexture(GL_TEXTURE_2D, texture);
    state->uv_sphere.draw();
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void render_bodies(GlobalState* state, const vec3& scene_origin) {
    glUseProgram(state->base_shader);
    reset_matrices(state);

    set_picking_object(state, state->root);
    render_body(state, state->root, scene_origin);
    clear_picking_object(state);

    // enable lighting
    glUseProgram(state->lighting_shader);
    reset_matrices(state);
    auto pos = body_global_position_at_time(state->root, state->time) - scene_origin;
    auto pos2 = state->view_matrix * state->model_matrix * glm::vec4(pos[0], pos[1], pos[2], 1.0f);
    GLint lighting_source = glGetUniformLocation(state->lighting_shader, "lighting_source");
    glUniform3fv(lighting_source, 1, glm::value_ptr(pos2));

    for (auto key_value_pair : state->bodies) {
        auto body = key_value_pair.second;
        if (body == state->root) {
            continue;
        }
        set_picking_object(state, body);
        render_body(state, body, scene_origin);
        clear_picking_object(state);
    }
}

static void render_helpers(GlobalState* state, const vec3& scene_origin) {
    if (!state->show_helpers) {
        return;
    }

    GLint colorUniform = glGetUniformLocation(state->base_shader, "u_color");

    // draw circles around celestial bodies when from far away
    glPointSize(20);
    glUseProgram(state->position_marker_shader);
    reset_matrices(state);
    glUniform4f(colorUniform, 1.0f, 0.0f, 0.0f, 0.5f);
    OrbitSystem(state->root, scene_origin, state->time).draw();

    glUseProgram(state->base_shader);
    reset_matrices(state);

    glPointSize(5);

    glUniform4f(colorUniform, 1.0f, 1.0f, 0.0f, 0.2f);
    for (auto key_value_pair : state->bodies) {
        auto body = key_value_pair.second;
        if (is_ancestor_of(body, state->focus)) {
            continue;
        }
        if (body->orbit == NULL) {
            continue;
        }

        auto position = body_global_position_at_time(body->orbit->primary, state->time) - scene_origin;
        state->model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(position[0], position[1], position[2]));
        update_matrices(state);

        set_picking_object(state, body);
        state->orbit_meshes.at(body->name).draw();
        state->apses_meshes.at(body->name).draw();
        clear_picking_object(state);
    }

    glUniform4f(colorUniform, 1.0f, 1.0f, 0.0f, 1.0f);
    for (auto key_value_pair : state->bodies) {
        auto body = key_value_pair.second;
        if (body == state->root || !is_ancestor_of(body, state->focus)) {
            continue;
        }

        auto position = body_global_position_at_time(body, state->time) - scene_origin;
        state->model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(position[0], position[1], position[2]));
        update_matrices(state);

        set_picking_object(state, body);
        FocusedOrbitMesh(body->orbit, state->time).draw();
        FocusedOrbitApsesMesh(body->orbit, state->time).draw();
        clear_picking_object(state);
    }
}

static void fill_hud(GlobalState* state) {
    // time warp
    state->hud.print("Time x%g\n", state->timewarp);

    // local time
    if (string(state->root->name) == "Sun") {
        time_t simulation_time = J2000 + (time_t) state->time;
        struct tm* t = localtime(&simulation_time);
        char buffer[512];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %z", t);
        state->hud.print("Date: %s\n", buffer);
    }

    // focus
    state->hud.print("Focus: %s\n", state->focus->name);

    // FPS
    double now = real_clock();
    double fps = (double) state->n_frames_since_last / (now - state->last_fps_measure);
    state->hud.print("%.1f FPS\n", fps);

    // update FPS measure every second
    if (now - state->last_fps_measure > 1.) {
        state->n_frames_since_last = 0;
        state->last_fps_measure = now;
    }

    // zoom
    state->hud.print("Zoom: %g\n", state->view_zoom);

    // version
    state->hud.print("Version " VERSION "\n");
}

static void render_hud(GlobalState* state) {
    if (!state->show_hud) {
        return;
    }
    if (state->picking_active) {
        return;
    }

    state->hud.clear();
    fill_hud(state);

    glUseProgram(state->base_shader);
    reset_matrices(state);

    // use orthographic projection
    auto model_view = glm::mat4(1.0f);
    GLint uniMV = glGetUniformLocation(state->base_shader, "model_view_matrix");
    glUniformMatrix4fv(uniMV, 1, GL_FALSE, glm::value_ptr(model_view));

    auto proj = glm::ortho(0.f, (float) state->viewport_width, (float) state->viewport_height, 0.f, -1.f, 1.f);
    GLint uniMVP = glGetUniformLocation(state->base_shader, "model_view_projection_matrix");
    glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(proj * model_view));

    GLint colorUniform = glGetUniformLocation(state->base_shader, "u_color");
    glUniform4f(colorUniform, 1.f, 1.f, 1.f, 1.f);

    state->hud.draw();
    if (state->show_help) {
        state->help.draw();
    }
}

void render(GlobalState* state) {
    if (state->picking_active) {
        state->picking_objects.clear();
    }

    glClearColor(0.f, 0.f, 0.f, .0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto scene_origin = body_global_position_at_time(state->focus, state->time);

    if (state->show_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    render_skybox(state);
    render_bodies(state, scene_origin);
    render_helpers(state, scene_origin);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    render_hud(state);
}
