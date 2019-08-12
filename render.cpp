#include "render.hpp"

extern "C" {
#include "version.h"
#include "util.h"
}

#include "cubemap.hpp"
#include "text_panel.hpp"
#include "picking.hpp"

#include <glm/gtc/type_ptr.hpp>

struct RenderState {
    Cubemap skybox = Cubemap(10, "data/textures/skybox/GalaxyTex_{}.jpg");
    TextPanel hud = TextPanel(5.f, 5.f);
    TextPanel help = TextPanel(5.f, 119.f);
    UVSphereMesh uv_sphere = UVSphereMesh(1, 64, 64);

    glm::mat4 model_matrix;
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
};

RenderState* make_render_state() {
    auto render_state = new RenderState;

    char* help = load_file("data/help.txt");
    render_state->help.print("%s", help);
    free(help);

    return render_state;
}

void delete_render_state(RenderState* render_state) {
    delete render_state;
}

const time_t J2000 = 946728000UL;  // 2000-01-01T12:00:00Z

void update_matrices(GlobalState* state) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto model_view = state->render_state->view_matrix * state->render_state->model_matrix;
    GLint var = glGetUniformLocation(program, "model_view_matrix");
    if (var >= 0) {
        glUniformMatrix4fv(var, 1, GL_FALSE, glm::value_ptr(model_view));
    }

    auto model_view_projection = state->render_state->projection_matrix * model_view;
    var = glGetUniformLocation(program, "model_view_projection_matrix");
    if (var >= 0) {
        glUniformMatrix4fv(var, 1, GL_FALSE, glm::value_ptr(model_view_projection));
    }
}

void reset_matrices(GlobalState* state, bool zoom) {
    state->render_state->model_matrix = glm::mat4(1.0f);

    glm::mat4 view = glm::mat4(1.0f);
    if (zoom) {
        view = glm::translate(view, glm::vec3(0.f, 0.f, -1.f / state->view_zoom));
    }
    view = glm::rotate(view, float(glm::radians(state->view_phi)), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, float(glm::radians(state->view_theta)), glm::vec3(0.0f, 0.0f, 1.0f));
    state->render_state->view_matrix = view;

    float aspect = float(state->viewport_width) / float(state->viewport_height);
    state->render_state->projection_matrix = glm::perspective(glm::radians(45.0f), aspect, .1f, 1e7f);

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
    state->render_state->skybox.draw();
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

    state->render_state->model_matrix = model;
    update_matrices(state);

    auto texture = state->body_textures.at(body->name);
    glBindTexture(GL_TEXTURE_2D, texture);
    state->render_state->uv_sphere.draw();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void render_star(GlobalState* state, const vec3& scene_origin) {
    glUseProgram(state->base_shader);
    reset_matrices(state);

    set_picking_object(state, state->root);
    render_body(state, state->root, scene_origin);
    clear_picking_object(state);
}

static void render_bodies(GlobalState* state, const vec3& scene_origin) {
    glUseProgram(state->lighting_shader);
    reset_matrices(state);
    auto pos = body_global_position_at_time(state->root, state->time) - scene_origin;
    auto pos2 = state->render_state->view_matrix * state->render_state->model_matrix * glm::vec4(pos[0], pos[1], pos[2], 1.0f);
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

    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto model = glm::mat4(1.f);
    auto position = body_global_position_at_time(state->rocket.orbit->primary, state->time)
        - scene_origin
        + state->rocket.state.position();
    model = glm::translate(model, glm::vec3(position[0], position[1], position[2]));
    model = glm::scale(model, glm::vec3(1e3f));
    state->render_state->model_matrix = model;
    update_matrices(state);

    state->render_state->uv_sphere.draw();
}

double glow_size(double radius, double temperature, double distance) {
    // from https://www.seedofandromeda.com/blogs/51-procedural-star-rendering
    static const double sun_radius = 696e6;
    static const double sun_surface_temperature = 5778.0;

    double luminosity = (radius / sun_radius) * pow(temperature / sun_surface_temperature, 4.);
    return 1e17 * pow(luminosity, 0.25) / pow(distance, 0.5);
}

GLuint init_lens_flare(void) {
    struct Sprite {
        float offset;
        float size;
        int texture_index;
    } sprites[] = {
        { 1.00f, 1.30f, 1 },
        { 1.25f, 1.00f, 1 },
        { 1.10f, 1.75f, 0 },
        { 1.50f, 0.65f, 0 },
        { 1.60f, 0.90f, 0 },
        { 1.70f, 0.45f, 0 },
    };
    size_t n_sprites = sizeof(sprites) / sizeof(sprites[0]);

    size_t n = sizeof(float) * 6 * 6 * n_sprites;
    float* data = (float*) MALLOC(n);

    size_t k = 0;
    for (size_t i = 0; i < n_sprites; i += 1) {
        float o = sprites[i].offset;
        float s = sprites[i].size;
        int t = sprites[i].texture_index;

        float l = t == 0 ? 0.f : .5f;

        data[k++] = -s; data[k++] = -s; data[k++] = 0.f; data[k++] = l + 0.0f; data[k++] = 0.f; data[k++] = o;
        data[k++] = +s; data[k++] = -s; data[k++] = 0.f; data[k++] = l + 0.5f; data[k++] = 0.f; data[k++] = o;
        data[k++] = -s; data[k++] = +s; data[k++] = 0.f; data[k++] = l + 0.0f; data[k++] = 1.f; data[k++] = o;

        data[k++] = -s; data[k++] = +s; data[k++] = 0.f; data[k++] = l + 0.0f; data[k++] = 1.f; data[k++] = o;
        data[k++] = +s; data[k++] = -s; data[k++] = 0.f; data[k++] = l + 0.5f; data[k++] = 0.f; data[k++] = o;
        data[k++] = +s; data[k++] = +s; data[k++] = 0.f; data[k++] = l + 0.5f; data[k++] = 1.f; data[k++] = o;
    }

    /*
    for (size_t i = 0; i < n / sizeof(float); ) {
        for (size_t x = 0; x < 6; x += 1) {
            for (size_t j = 0; j < 6; j += 1) {
                printf("%20.9g", data[i++]);
            }
            printf("\n");
        }
        printf("\n");
    }
    */

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, n, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    free(data);

    return vbo;
}

void render_lens_flare(GlobalState* state, const vec3& scene_origin, float visibility) {
    static GLuint lens_flare_vbo = 0;
    if (lens_flare_vbo == 0) {
        lens_flare_vbo = init_lens_flare();
    }

    auto position = body_global_position_at_time(state->root, state->time) - scene_origin;
    glm::vec3 light_source = {position[0], position[1], position[2]};

    //glm::vec2 light_source = {.5f, .5f};

    //glm::mat4 view = state->render_state->view_matrix;
    //glm::vec4 z = {view * glm::vec4(0.f, 0.f, 1.f, 1.f)};
    //glm::vec3 light_source = {z[0], z[1], z[2]};

    float size = .1f;
    float aspect = float(state->viewport_width) / float(state->viewport_height);
    glm::vec2 dims(size, size * aspect);

    float intensity = .2f * visibility;

    glUseProgram(state->lens_flare_shader);
    reset_matrices(state);

    glBindBuffer(GL_ARRAY_BUFFER, lens_flare_vbo);

    GLint var = glGetAttribLocation(state->lens_flare_shader, "v_position");
    glEnableVertexAttribArray(var);
    glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, 6 * (GLsizei) sizeof(float), NULL);

    var = glGetAttribLocation(state->lens_flare_shader, "v_texcoord");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 2, GL_FLOAT, GL_FALSE, 6 * (GLsizei) sizeof(float), (GLvoid*)(3 * sizeof(float)));
    }

    var = glGetAttribLocation(state->lens_flare_shader, "v_offset");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 1, GL_FLOAT, GL_FALSE, 6 * (GLsizei) sizeof(float), (GLvoid*)(5 * sizeof(float)));
    }

    glBindTexture(GL_TEXTURE_2D, state->lens_flare_texture);
    glUniform2fv(glGetUniformLocation(state->lens_flare_shader, "u_dims"), 1, glm::value_ptr(dims));
    glUniform3fv(glGetUniformLocation(state->lens_flare_shader, "u_light_source"), 1, glm::value_ptr(light_source));
    glUniform1f(glGetUniformLocation(state->lens_flare_shader, "u_intensity"), intensity);

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void render_star_glow(GlobalState* state, const vec3& scene_origin) {
    if (state->picking_active) {
        return;
    }

    // Update queries from previous frame, or create the query objects if needed
    float visibility = 1.f;
    static GLuint occlusionQuery[2] = {0, 0};
    if (occlusionQuery[0] == 0) {
        glGenQueries(2, occlusionQuery);
    } else {
        int totalSamples = 0;
        int passedSamples= 0;
        glGetQueryObjectiv(occlusionQuery[0], GL_QUERY_RESULT, &totalSamples );
        glGetQueryObjectiv(occlusionQuery[1], GL_QUERY_RESULT, &passedSamples);
        if (passedSamples == 0) {
            visibility = 0.0f;
        } else {
            float param = (float) passedSamples/ (float) totalSamples;
            visibility = 1.f - expf(-4.f * param);
        }
    }

    auto position = body_global_position_at_time(state->root, state->time) - scene_origin;
    glm::vec3 star_glow_position = {position[0], position[1], position[2]};
    glm::mat4 view = state->render_state->view_matrix;
    glm::vec3 camera_right = {view[0][0], view[1][0], view[2][0]};
    glm::vec3 camera_up = {view[0][1], view[1][1], view[2][1]};

    glm::vec4 z = {view * glm::vec4(0.f, 0.f, 1.f, 1.f)};
    glm::vec3 camera_z = {z[0], z[1], z[2]};

    // double d = state->focus->orbit->semi_major_axis;
    double d = glm::length(star_glow_position - camera_z);
    double s = glow_size(state->root->radius, 5778., d);
    glm::vec2 star_glow_size = {s, s};

    glUseProgram(state->star_glow_shader);
    reset_matrices(state);

    float unNoiseZ = (float) abs(
            glm::dot(camera_right, glm::vec3(1.f, 3.f, 6.f))
            + glm::dot(camera_up, glm::vec3(1.f, 3.f, 6.f))
    );

    glBindTexture(GL_TEXTURE_2D, state->star_glow_texture);
    glUniform1f(glGetUniformLocation(state->star_glow_shader, "visibility"), visibility);
    glUniform2fv(glGetUniformLocation(state->star_glow_shader, "star_glow_size"), 1, glm::value_ptr(star_glow_size));
    glUniform3fv(glGetUniformLocation(state->star_glow_shader, "star_glow_position"), 1, glm::value_ptr(star_glow_position));
    glUniform3fv(glGetUniformLocation(state->star_glow_shader, "camera_right"), 1, glm::value_ptr(camera_right));
    glUniform3fv(glGetUniformLocation(state->star_glow_shader, "camera_up"), 1, glm::value_ptr(camera_up));
    glUniform1f(glGetUniformLocation(state->star_glow_shader, "unNoiseZ"), unNoiseZ);

    glDepthMask(GL_FALSE);
    s = (float) state->root->radius;
    star_glow_size = {s, s};
    glUniform1f(glGetUniformLocation(state->star_glow_shader, "visibility"), -1.f);
    glUniform2fv(glGetUniformLocation(state->star_glow_shader, "star_glow_size"), 1, glm::value_ptr(star_glow_size));

    // Query for passed samples
    glBeginQuery(GL_SAMPLES_PASSED, occlusionQuery[0]);
    glDisable(GL_DEPTH_TEST);
    SquareMesh(1.f).draw();
    glEnable(GL_DEPTH_TEST);
    glEndQuery(GL_SAMPLES_PASSED);

    // Query for total samples
    glBeginQuery(GL_SAMPLES_PASSED, occlusionQuery[1]);
    SquareMesh(1.f).draw();
    glEndQuery(GL_SAMPLES_PASSED);

    s = glow_size(state->root->radius, 5778., d);
    star_glow_size = {s, s};
    glUniform1f(glGetUniformLocation(state->star_glow_shader, "visibility"), visibility);
    glUniform2fv(glGetUniformLocation(state->star_glow_shader, "star_glow_size"), 1, glm::value_ptr(star_glow_size));
    SquareMesh(1.f).draw();
    glDepthMask(GL_TRUE);

    glBindTexture(GL_TEXTURE_2D, 0);

    render_lens_flare(state, scene_origin, visibility);
}

static void render_position_markers(GlobalState* state, const vec3& scene_origin) {
    // draw circles around celestial bodies when from far away
    glPointSize(20);
    glUseProgram(state->position_marker_shader);
    reset_matrices(state);
    GLint colorUniform = glGetUniformLocation(state->position_marker_shader, "u_color");
    glUniform4f(colorUniform, 1.0f, 0.0f, 0.0f, 0.5f);
    OrbitSystem(state->root, scene_origin, state->time).draw();
}

static void render_orbits(GlobalState* state, const vec3& scene_origin) {
    glUseProgram(state->base_shader);
    GLint colorUniform = glGetUniformLocation(state->position_marker_shader, "u_color");
    reset_matrices(state);

    glPointSize(5);

    // unfocused orbits
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
        state->render_state->model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(position[0], position[1], position[2]));
        update_matrices(state);

        set_picking_object(state, body);
        state->orbit_meshes.at(body->name).draw();
        state->apses_meshes.at(body->name).draw();
        clear_picking_object(state);
    }

    // focused orbits
    glUniform4f(colorUniform, 1.0f, 1.0f, 0.0f, 1.0f);
    for (auto key_value_pair : state->bodies) {
        auto body = key_value_pair.second;
        if (body == state->root || !is_ancestor_of(body, state->focus)) {
            continue;
        }

        auto position = body_global_position_at_time(body, state->time) - scene_origin;
        state->render_state->model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(position[0], position[1], position[2]));
        update_matrices(state);

        set_picking_object(state, body);
        FocusedOrbitMesh(body->orbit, state->time).draw();
        FocusedOrbitApsesMesh(body->orbit, state->time).draw();
        clear_picking_object(state);
    }

    CelestialBody* body = &state->rocket;
    auto position = body_global_position_at_time(body, state->time) - scene_origin;
    state->render_state->model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(position[0], position[1], position[2]));
    OrbitMesh(body->orbit).draw();
    OrbitApsesMesh(body->orbit).draw();
}

static void render_helpers(GlobalState* state, const vec3& scene_origin) {
    if (!state->show_helpers) {
        return;
    }

    render_position_markers(state, scene_origin);
    render_orbits(state, scene_origin);
}

static void fill_hud(GlobalState* state) {
    // time warp
    state->render_state->hud.print("Time x%g\n", state->timewarp);

    // local time
    if (string(state->root->name) == "Sun") {
        time_t simulation_time = J2000 + (time_t) state->time;
        struct tm* t = localtime(&simulation_time);
        char buffer[512];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %z", t);
        state->render_state->hud.print("Date: %s\n", buffer);
    }

    // focus
    state->render_state->hud.print("Focus: %s\n", state->focus->name);

    // FPS
    double now = real_clock();
    double fps = (double) state->n_frames_since_last / (now - state->last_fps_measure);
    state->render_state->hud.print("%.1f FPS\n", fps);

    // update FPS measure every second
    if (now - state->last_fps_measure > 1.) {
        state->n_frames_since_last = 0;
        state->last_fps_measure = now;
    }

    // zoom
    state->render_state->hud.print("Zoom: %g\n", state->view_zoom);

    // version
    state->render_state->hud.print("Version " VERSION "\n");
}

static void render_hud(GlobalState* state) {
    if (!state->show_hud) {
        return;
    }
    if (state->picking_active) {
        return;
    }

    state->render_state->hud.clear();
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

    state->render_state->hud.draw();
    if (state->show_help) {
        state->render_state->help.draw();
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
    render_star_glow(state, scene_origin);
    render_helpers(state, scene_origin);
    render_star(state, scene_origin);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    render_hud(state);
}
