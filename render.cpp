#include "render.hpp"

extern "C" {
#include "version.h"
#include "util.h"
#include "texture.h"
#include "shaders.h"
}

#include "mesh.hpp"
#include "text_panel.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <vector>

using std::map;

struct RenderState {
    // matrices
    glm::mat4 model_matrix;
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;

    // shaders
    GLuint base_shader;
    GLuint skybox_shader;
    GLuint cubemap_shader;
    GLuint lighting_shader;
    GLuint position_marker_shader;
    GLuint star_glow_shader;
    GLuint lens_flare_shader;

    // VAOs
    GLuint vao;

    // meshes
    CubeMesh cube = CubeMesh(10.f);
    UVSphereMesh uv_sphere = UVSphereMesh(1, 4);
    SquareMesh square = SquareMesh(1.);
    map<CelestialBody*, OrbitMesh> orbit_meshes;
    map<CelestialBody*, OrbitApsesMesh> apses_meshes;

    // textures
    GLint star_glow_texture;
    GLint lens_flare_texture;
    GLint rocket_texture;
    GLint skybox_texture;

    map<CelestialBody*, GLuint> body_textures;
    map<CelestialBody*, GLuint> body_cubemaps;

    // models
    TextPanel hud = TextPanel(5.f, 5.f);
    TextPanel help = TextPanel(5.f, 157.f);

    bool picking_active = false;
    std::vector<CelestialBody*> picking_objects;
};

void use_program(GlobalState* state, GLuint program, bool zoom=true) {
    glUseProgram(program);
    reset_matrices(state, zoom);

    GLint var;

    // color
    var = glGetUniformLocation(program, "u_color");
    if (var >= 0) {
        glUniform4f(var, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    // lighting source
    var = glGetUniformLocation(program, "lighting_source");
    if (var >= 0) {
        auto scene_origin = body_global_position_at_time(state->focus, state->time);
        auto pos = body_global_position_at_time(state->root, state->time) - scene_origin;
        auto pos2 = state->render_state->view_matrix * state->render_state->model_matrix * glm::vec4(pos[0], pos[1], pos[2], 1.0f);
        glUniform3fv(var, 1, glm::value_ptr(pos2));
    }

    // picking
    var = glGetUniformLocation(program, "picking_active");
    if (var >= 0) {
        glUniform1i(var, state->render_state->picking_active);
        set_picking_name(state->render_state->picking_objects.size());
    }
}

RenderState* make_render_state(const map<std::string, CelestialBody*>& bodies) {
    auto render_state = new RenderState;

    // shaders
    render_state->skybox_shader = make_program(3, "base", "skybox", "logz");
    render_state->cubemap_shader = make_program(4, "base", "cubemap", "lighting", "logz");
    render_state->lighting_shader = make_program(4, "base", "lighting", "picking", "logz");
    render_state->position_marker_shader = make_program(4, "base", "position_marker", "picking", "logz");
    render_state->base_shader = make_program(3, "base", "picking", "logz");
    render_state->star_glow_shader = make_program(3, "base", "star_glow", "logz");
    render_state->lens_flare_shader = make_program(3, "base", "lens_flare", "logz");

    // TODO
    glUseProgram(render_state->skybox_shader);
    glUniform1i(glGetUniformLocation(render_state->skybox_shader, "skybox_texture"), 0);
    glUseProgram(render_state->cubemap_shader);
    glUniform1i(glGetUniformLocation(render_state->cubemap_shader, "cubemap_texture"), 0);

    // VAOs
    glGenVertexArrays(1, &render_state->vao);
    glBindVertexArray(render_state->vao);

    // meshes
    for (auto key_value_pair : bodies) {
        auto name = key_value_pair.first;
        auto body = key_value_pair.second;
        if (body->orbit == NULL) {
            continue;
        }
        render_state->orbit_meshes.emplace(body, body->orbit);
        render_state->apses_meshes.emplace(body, body->orbit);
    }

    // textures
    render_state->star_glow_texture = load_texture("data/textures/star_glow.png");
    render_state->lens_flare_texture = load_texture("data/textures/lens_flares.png");
    render_state->rocket_texture = load_texture("data/textures/rocket_on.png");
    render_state->skybox_texture = load_cubemap("data/textures/skybox/GalaxyTex_{}.jpg");

    for (auto key_value_pair : bodies) {
        auto body = key_value_pair.second;

        auto cubemap_path = "data/textures/solar/" + std::string(body->name) + "/{}.jpg";
        auto cubemap = load_cubemap(cubemap_path.c_str());
        if (cubemap != 0) {
            render_state->body_cubemaps[body] = cubemap;
            continue;
        }

        auto texture_path = "data/textures/solar/" + std::string(body->name) + ".jpg";
        auto texture = load_texture(texture_path.c_str());
        if (texture != 0) {
            render_state->body_textures[body] = texture;
            continue;
        }

        fprintf(stderr, "WARNING: failed to find texture or cubemap for %s\n", body->name);
    }

    // models
    char* help = load_file("data/help.txt");
    if (help == NULL) {
        render_state->help.print("COULD NOT LOAD HELP FILE\n");
    } else {
        render_state->help.print("%s", help);
        free(help);
    }

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
        double d = state->view_altitude;
        if (state->focus != NULL) {
            d += state->focus->radius;
        }
        view = glm::translate(view, glm::vec3(0.f, 0.f, -d));
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
    if (state->render_state->picking_active) {
        return;
    }

    use_program(state, state->render_state->skybox_shader, false);

    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_CUBE_MAP, state->render_state->skybox_texture);
    state->render_state->cube.draw();
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glEnable(GL_DEPTH_TEST);
}

static void set_body_matrices(GlobalState* state, CelestialBody* body, const vec3& scene_origin) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto model = glm::mat4(1.f);
    auto position = body_global_position_at_time(body, state->time) - scene_origin;
    model = glm::translate(model, glm::vec3(position[0], position[1], position[2]));
    model = glm::scale(model, glm::vec3(float(body->radius)));

    // axial tilt
    if (body->positive_pole != NULL) {
        double z_angle = body->positive_pole->ecliptic_longitude - M_PI / 2.;
        model = glm::rotate(model, float(z_angle), glm::vec3(0.f, 0.f, 1.f));
        double x_angle = body->positive_pole->ecliptic_latitude - M_PI / 2.;
        model = glm::rotate(model, float(x_angle), glm::vec3(1.f, 0.f, 0.f));
    }

    // OpenGL use single precision while Python has double precision
    // reducing modulo 2 PI in Python reduces loss of significance
    double turn_fraction = fmod(state->time / abs(body->sidereal_day), 1.);
    model = glm::rotate(model, 2.f * M_PIf32 * float(turn_fraction), glm::vec3(0.f, 0.f, 1.f));

    state->render_state->model_matrix = model;
    update_matrices(state);
}

static void render_body(GlobalState* state, CelestialBody* body, const vec3& scene_origin) {
    // draw sphere textured with cubemap (preferrably), or equirectangular texture
    auto& cubemaps = state->render_state->body_cubemaps;
    auto search = cubemaps.find(body);
    if (search != cubemaps.end()) {
        // cubemap
        auto cubemap = search->second;

        // prepare shader
        use_program(state, state->render_state->cubemap_shader);
        set_body_matrices(state, body, scene_origin);

        GLint program;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);

        glm::mat4 cubemap_matrix = glm::mat4(1.f);
        cubemap_matrix = glm::rotate(cubemap_matrix, -M_PIf32/2.f, glm::vec3(1.f, 0.f, 0.f));
        cubemap_matrix = glm::rotate(cubemap_matrix, M_PIf32/2.f, glm::vec3(0.f, 0.f, 1.f));
        GLint var = glGetUniformLocation(program, "cubemap_matrix");
        glUniformMatrix4fv(var, 1, GL_FALSE, glm::value_ptr(cubemap_matrix));

        auto pos = body_global_position_at_time(state->root, state->time) - scene_origin;
        auto pos2 = state->render_state->view_matrix * glm::vec4(pos[0], pos[1], pos[2], 1.0f);
        GLint lighting_source = glGetUniformLocation(program, "lighting_source");
        glUniform3fv(lighting_source, 1, glm::value_ptr(pos2));

        // bind cubemap texture
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

        // draw
        state->render_state->uv_sphere.draw();
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    } else {
        // equirectangular texture
        use_program(state, state->render_state->base_shader);
        set_body_matrices(state, body, scene_origin);

        // bind equirectangular texture if it exists
        auto& textures = state->render_state->body_textures;
        search = textures.find(body);
        if (search != textures.end()) {
            auto texture = search->second;
            glBindTexture(GL_TEXTURE_2D, texture);
        }

        // draw
        state->render_state->uv_sphere.draw();
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void render_star(GlobalState* state, const vec3& scene_origin) {
    use_program(state, state->render_state->base_shader);

    set_picking_object(state, state->root);
    render_body(state, state->root, scene_origin);
    clear_picking_object(state);
}

static void render_bodies(GlobalState* state, const vec3& scene_origin) {
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
    model = glm::rotate(model, M_PIf32/2, glm::vec3(0.f, 1.f, 0.f));
    model = glm::scale(model, glm::vec3(1e5f));
    state->render_state->model_matrix = model;
    update_matrices(state);

    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, state->render_state->rocket_texture);
    state->render_state->square.draw();
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_CULL_FACE);
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

    float size = .1f;
    float aspect = float(state->viewport_width) / float(state->viewport_height);
    glm::vec2 dims(size, size * aspect);

    float intensity = .2f * visibility;

    use_program(state, state->render_state->lens_flare_shader);

    glBindBuffer(GL_ARRAY_BUFFER, lens_flare_vbo);

    GLint var = glGetAttribLocation(state->render_state->lens_flare_shader, "v_position");
    glEnableVertexAttribArray(var);
    glVertexAttribPointer(var, 3, GL_FLOAT, GL_FALSE, 6 * (GLsizei) sizeof(float), NULL);

    var = glGetAttribLocation(state->render_state->lens_flare_shader, "v_texcoord");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 2, GL_FLOAT, GL_FALSE, 6 * (GLsizei) sizeof(float), (GLvoid*)(3 * sizeof(float)));
    }

    var = glGetAttribLocation(state->render_state->lens_flare_shader, "v_offset");
    if (var >= 0) {
        glEnableVertexAttribArray(var);
        glVertexAttribPointer(var, 1, GL_FLOAT, GL_FALSE, 6 * (GLsizei) sizeof(float), (GLvoid*)(5 * sizeof(float)));
    }

    glBindTexture(GL_TEXTURE_2D, state->render_state->lens_flare_texture);
    glUniform2fv(glGetUniformLocation(state->render_state->lens_flare_shader, "u_dims"), 1, glm::value_ptr(dims));
    glUniform3fv(glGetUniformLocation(state->render_state->lens_flare_shader, "u_light_source"), 1, glm::value_ptr(light_source));
    glUniform1f(glGetUniformLocation(state->render_state->lens_flare_shader, "u_intensity"), intensity);

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void render_star_glow(GlobalState* state, const vec3& scene_origin) {
    if (state->render_state->picking_active) {
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

    use_program(state, state->render_state->star_glow_shader);

    float unNoiseZ = (float) abs(
            glm::dot(camera_right, glm::vec3(1.f, 3.f, 6.f))
            + glm::dot(camera_up, glm::vec3(1.f, 3.f, 6.f))
    );

    glBindTexture(GL_TEXTURE_2D, state->render_state->star_glow_texture);
    glUniform1f(glGetUniformLocation(state->render_state->star_glow_shader, "visibility"), visibility);
    glUniform2fv(glGetUniformLocation(state->render_state->star_glow_shader, "star_glow_size"), 1, glm::value_ptr(star_glow_size));
    glUniform3fv(glGetUniformLocation(state->render_state->star_glow_shader, "star_glow_position"), 1, glm::value_ptr(star_glow_position));
    glUniform3fv(glGetUniformLocation(state->render_state->star_glow_shader, "camera_right"), 1, glm::value_ptr(camera_right));
    glUniform3fv(glGetUniformLocation(state->render_state->star_glow_shader, "camera_up"), 1, glm::value_ptr(camera_up));
    glUniform1f(glGetUniformLocation(state->render_state->star_glow_shader, "unNoiseZ"), unNoiseZ);

    glDepthMask(GL_FALSE);
    s = (float) state->root->radius;
    star_glow_size = {s, s};
    glUniform1f(glGetUniformLocation(state->render_state->star_glow_shader, "visibility"), -1.f);
    glUniform2fv(glGetUniformLocation(state->render_state->star_glow_shader, "star_glow_size"), 1, glm::value_ptr(star_glow_size));

    // Query for passed samples
    glBeginQuery(GL_SAMPLES_PASSED, occlusionQuery[0]);
    glDisable(GL_DEPTH_TEST);
    state->render_state->square.draw();
    glEnable(GL_DEPTH_TEST);
    glEndQuery(GL_SAMPLES_PASSED);

    // Query for total samples
    glBeginQuery(GL_SAMPLES_PASSED, occlusionQuery[1]);
    state->render_state->square.draw();
    glEndQuery(GL_SAMPLES_PASSED);

    s = glow_size(state->root->radius, 5778., d);
    star_glow_size = {s, s};
    glUniform1f(glGetUniformLocation(state->render_state->star_glow_shader, "visibility"), visibility);
    glUniform2fv(glGetUniformLocation(state->render_state->star_glow_shader, "star_glow_size"), 1, glm::value_ptr(star_glow_size));
    state->render_state->square.draw();
    glDepthMask(GL_TRUE);

    glBindTexture(GL_TEXTURE_2D, 0);

    render_lens_flare(state, scene_origin, visibility);
}

static void render_position_markers(GlobalState* state, const vec3& scene_origin) {
    // draw circles around celestial bodies when from far away
    glPointSize(20);
    use_program(state, state->render_state->position_marker_shader);
    GLint colorUniform = glGetUniformLocation(state->render_state->position_marker_shader, "u_color");
    glUniform4f(colorUniform, 1.0f, 0.0f, 0.0f, 0.5f);
    OrbitSystem(state->root, scene_origin, state->time).draw();
}

static void render_orbits(GlobalState* state, const vec3& scene_origin) {
    use_program(state, state->render_state->base_shader);
    GLint colorUniform = glGetUniformLocation(state->render_state->position_marker_shader, "u_color");

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
        state->render_state->orbit_meshes.at(body).draw();
        state->render_state->apses_meshes.at(body).draw();
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
    auto position = body_global_position_at_time(body->orbit->primary, state->time) - scene_origin;
    state->render_state->model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(position[0], position[1], position[2]));
    update_matrices(state);
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
    if (state->real_timewarp < state->target_timewarp) {
        state->render_state->hud.print("Time x%g (actual: x%g)\n", state->target_timewarp, state->real_timewarp);
    } else {
        state->render_state->hud.print("Time x%g\n", state->target_timewarp);
    }

    // local time
    if (std::string(state->root->name) == "Sun") {
        time_t simulation_time = J2000 + (time_t) state->time;
        struct tm* t = localtime(&simulation_time);
        char buffer[512];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %z", t);
        state->render_state->hud.print("Date: %s\n", buffer);
    }

    // focus
    state->render_state->hud.print("Focus: %s\n", state->focus->name);

    // distance
    char* s = human_quantity(state->view_altitude + state->focus->radius, "m");
    state->render_state->hud.print("Distance: %s\n", s);
    free(s);

    // altitude
    s = human_quantity(state->view_altitude, "m");
    state->render_state->hud.print("Altitude: %s\n", s);
    free(s);

    // FPS
    double now = real_clock();
    double fps = (double) state->n_frames_since_last / (now - state->last_fps_measure);
    state->render_state->hud.print("%.0f FPS\n", fps);

    // update FPS measure every second
    if (now - state->last_fps_measure > 1.) {
        state->n_frames_since_last = 0;
        state->last_fps_measure = now;
    }

    // version
    state->render_state->hud.print("Version " VERSION "\n");
}

static void render_hud(GlobalState* state) {
    if (!state->show_hud) {
        return;
    }
    if (state->render_state->picking_active) {
        return;
    }

    state->render_state->hud.clear();
    fill_hud(state);

    use_program(state, state->render_state->base_shader);

    // use orthographic projection
    auto model_view = glm::mat4(1.0f);
    GLint uniMV = glGetUniformLocation(state->render_state->base_shader, "model_view_matrix");
    glUniformMatrix4fv(uniMV, 1, GL_FALSE, glm::value_ptr(model_view));

    auto proj = glm::ortho(0.f, (float) state->viewport_width, (float) state->viewport_height, 0.f, -1.f, 1.f);
    GLint uniMVP = glGetUniformLocation(state->render_state->base_shader, "model_view_projection_matrix");
    glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(proj * model_view));

    state->render_state->hud.draw();
    if (state->show_help) {
        state->render_state->help.draw();
    }
}

void render(GlobalState* state) {
    if (state->render_state->picking_active) {
        state->render_state->picking_objects.clear();
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

void set_picking_object(GlobalState* state, CelestialBody* object) {
    if (!state->render_state->picking_active) {
        return;
    }

    state->render_state->picking_objects.push_back(object);
    set_picking_name(state->render_state->picking_objects.size());
}

void clear_picking_object(GlobalState* state) {
    if (!state->render_state->picking_active) {
        return;
    }
    set_picking_name(0);
}

CelestialBody* pick(GlobalState* state) {
    // render with picking activated
    state->render_state->picking_active = true;
    glDisable(GL_MULTISAMPLE);
    render(state);
    glEnable(GL_MULTISAMPLE);
    state->render_state->picking_active = false;

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
    int best_cursor_d2 = search_radius * search_radius;  // squared distance to cursor
    for (int y = 0; y < h; y += 1) {
        for (int x = 0; x < w; x += 1) {
            size_t candidate_name = 0;
            candidate_name |= components[(y*w + x)*4 + 0] << 16;
            candidate_name |= components[(y*w + x)*4 + 1] <<  8;
            candidate_name |= components[(y*w + x)*4 + 2] <<  0;
            if (candidate_name == 0) {
                continue;
            }
            int dx = (x - w / 2);
            int dy = (y - h / 2);
            int cursor_d2 = dx * dx + dy * dy;
            if (cursor_d2 > best_cursor_d2) {
                continue;
            }
            name = candidate_name;
            best_cursor_d2 = cursor_d2;
        }
    }
    delete[] components;

    if (name == 0) {
        return NULL;
    }

    size_t n_objects = state->render_state->picking_objects.size();
    if (name > n_objects) {
        fprintf(stderr, "WARNING: picked %zu but only %zu known objects\n", name, n_objects);
        return NULL;
    } else {
        return state->render_state->picking_objects[name - 1];
    }
}
