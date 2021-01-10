#include "render.hpp"

extern "C" {
#include "version.h"
#include "util.h"
#include "logging.h"
#include "texture.h"
#include "shaders.h"
}

#include "mesh.hpp"
#include "model.hpp"
#include "text_panel.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

using std::map;

static const float NAVBALL_RADIUS = 100.f;
static const float NAVBALL_MARKER_SIZE = 50.f;
static const float NAVBALL_FRAME_RADIUS = NAVBALL_RADIUS * 1.25;
static const float NEELDLE_LENGTH = NAVBALL_FRAME_RADIUS - NAVBALL_RADIUS;
static const float NEELDLE_MIN_ANGLE = (float) radians(-135.);
static const float NEELDLE_MAX_ANGLE = (float) radians(-45.);

static const int THUMBNAIL_SIZE = 250;
static const double THUMBNAIL_RATIO_THRESHOLD = 50.;
static const double THUMBNAIL_ALTITUDE_FACTOR = 3.;

struct RenderState {
    // matrices
    glm::mat4 model_matrix;
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;

    // shaders
    GLuint base_shader;
    GLuint hud_shader;
    GLuint skybox_shader;
    GLuint cubemap_shader;
    GLuint lighting_shader;
    GLuint position_marker_shader;
    GLuint star_glow_shader;
    GLuint lens_flare_shader;
    GLuint billboard_shader;

    // VAOs
    GLuint vao;

    // meshes
    CubeMesh cube = CubeMesh(10.f);
    UVSphereMesh uv_sphere = UVSphereMesh(1, 4);
    RectMesh square = RectMesh(1, 1);
    RectMesh navball_marker_mesh = RectMesh(NAVBALL_MARKER_SIZE, -NAVBALL_MARKER_SIZE);
    map<CelestialBody*, OrbitMesh> orbit_meshes;
    map<CelestialBody*, OrbitApsesMesh> apses_meshes;

    // textures
    GLuint star_glow_texture;
    GLuint lens_flare_texture;
    GLuint skybox_texture;
    GLuint navball_texture;
    GLuint navball_frame_texture;
    GLuint level_indicator_texture;
    GLuint prograde_marker_texture;
    GLuint retrograde_marker_texture;
    GLuint normal_marker_texture;
    GLuint anti_normal_marker_texture;
    GLuint radial_in_marker_texture;
    GLuint radial_out_marker_texture;
    GLuint throttle_needle_texture;

    map<CelestialBody*, GLuint> body_textures;
    map<CelestialBody*, GLuint> body_cubemaps;

    // models
    TextPanel general_info = TextPanel(5.f, 5.f);
    TextPanel help = TextPanel(5.f, 195.f);
    TextPanel orbital_info = TextPanel(5.f, 5.f);

    Model rocket_model;

    bool picking_active = false;
    size_t current_picking_name = 0;
    std::vector<CelestialBody*> picking_objects;
};

RenderState* make_render_state(const map<std::string, CelestialBody*>& bodies, const std::string& textures_directory) {
    auto render_state = new RenderState;

    // shaders
    DEBUG("Shaders compilation");
    render_state->skybox_shader = make_program(2, "skybox", "logz");
    render_state->cubemap_shader = make_program(4, "cubemap", "lighting", "picking", "logz");
    render_state->lighting_shader = make_program(4, "base", "lighting", "picking", "logz");
    render_state->position_marker_shader = make_program(4, "base", "position_marker", "picking", "logz");
    render_state->base_shader = make_program(3, "base", "picking", "logz");
    render_state->hud_shader = make_program(2, "base", "picking");
    render_state->star_glow_shader = make_program(3, "base", "star_glow", "logz");
    render_state->lens_flare_shader = make_program(3, "base", "lens_flare", "logz");
    render_state->billboard_shader = make_program(2, "base", "billboard");
    DEBUG("Shaders compiled");

    // fix orientation of cubemap (e.g. Y up → Z up)
    glUseProgram(render_state->cubemap_shader);
    glm::mat4 cubemap_matrix = glm::mat4(1.f);
    cubemap_matrix = glm::rotate(cubemap_matrix, -M_PIf32/2.f, glm::vec3(1.f, 0.f, 0.f));
    cubemap_matrix = glm::rotate(cubemap_matrix, M_PIf32/2.f, glm::vec3(0.f, 0.f, 1.f));
    GLint var = glGetUniformLocation(render_state->cubemap_shader, "cubemap_matrix");
    glUniformMatrix4fv(var, 1, GL_FALSE, glm::value_ptr(cubemap_matrix));

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
    DEBUG("Textures loading");
    render_state->star_glow_texture          = load_texture("data/textures/star_glow.png");
    render_state->lens_flare_texture         = load_texture("data/textures/lens_flares.png");
    render_state->skybox_texture             = load_cubemap("data/textures/skybox/GalaxyTex_{}.jpg");
    render_state->navball_texture            = load_texture("data/textures/navball.png");
    render_state->navball_frame_texture      = load_texture("data/textures/navball-frame.png");
    render_state->level_indicator_texture    = load_texture("data/textures/markers/Level_indicator.png");
    render_state->prograde_marker_texture    = load_texture("data/textures/markers/Prograde.png");
    render_state->retrograde_marker_texture  = load_texture("data/textures/markers/Retrograde.png");
    render_state->normal_marker_texture      = load_texture("data/textures/markers/Normal.png");
    render_state->anti_normal_marker_texture = load_texture("data/textures/markers/Anti-normal.png");
    render_state->radial_in_marker_texture   = load_texture("data/textures/markers/Radial-in.png");
    render_state->radial_out_marker_texture  = load_texture("data/textures/markers/Radial-out.png");
    render_state->throttle_needle_texture    = load_texture("data/textures/needle.png");

    for (auto key_value_pair : bodies) {
        auto body = key_value_pair.second;

        auto cubemap_path = textures_directory + "/" + std::string(body->name) + "/{}.jpg";
        auto cubemap = load_cubemap(cubemap_path.c_str());
        if (cubemap != 0) {
            render_state->body_cubemaps[body] = cubemap;
            continue;
        }

        auto texture_path = textures_directory + "/" + std::string(body->name) + ".jpg";
        auto texture = load_texture(texture_path.c_str());
        if (texture != 0) {
            render_state->body_textures[body] = texture;
            continue;
        }

        WARNING("Missing texture for %s", body->name);
    }
    DEBUG("Textures loaded");

    char* help = load_file("data/help.txt");
    if (help == NULL) {
        WARNING("Could not load help file at data/help.txt");
        render_state->help.print("COULD NOT LOAD HELP FILE\n");
    } else {
        render_state->help.print("%s", help);
        free(help);
    }

    // models
    DEBUG("Models loading");
    render_state->rocket_model.load("data/models/h2f2obj/f.obj");
    DEBUG("Models loaded");

    return render_state;
}

void delete_render_state(RenderState* render_state) {
    delete render_state;
}

const time_t J2000 = 946728000UL;  // 2000-01-01T12:00:00Z

void set_color(float red, float green, float blue, float alpha=1.f) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint var = glGetUniformLocation(program, "u_color");
    if (var >= 0) {
        glUniform4f(var, red, green, blue, alpha);
    }
}

static void use_program(GlobalState* state, GLuint program, bool zoom=true) {
    glUseProgram(program);
    reset_matrices(state, zoom);
    set_color(1, 1, 1);

    GLint var;

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
        set_picking_name(state->render_state->current_picking_name);
    }
}

static void update_matrices(GlobalState* state) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto model_view = state->render_state->view_matrix * state->render_state->model_matrix;
    GLint var = glGetUniformLocation(program, "model_view_matrix");
    if (var >= 0) {
        glUniformMatrix4fv(var, 1, GL_FALSE, glm::value_ptr(model_view));
    }

    auto proj = state->render_state->projection_matrix;
    var = glGetUniformLocation(program, "projection_matrix");
    if (var >= 0) {
        glUniformMatrix4fv(var, 1, GL_FALSE, glm::value_ptr(proj));
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

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float aspect = float(viewport[2]) / float(viewport[3]);  // width / height
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

static void set_body_matrices(GlobalState* state, CelestialBody* body, const glm::dvec3& scene_origin) {
    auto model = glm::mat4(1.f);
    auto position = body_global_position_at_time(body, state->time) - scene_origin;
    model = glm::translate(model, glm::vec3(position));
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

static void render_body(GlobalState* state, CelestialBody* body, const glm::dvec3& scene_origin, bool lighting=true) {
    // draw sphere textured with cubemap (preferrably), or equirectangular texture
    auto& cubemaps = state->render_state->body_cubemaps;
    auto search = cubemaps.find(body);
    if (search != cubemaps.end()) {
        // cubemap
        auto cubemap = search->second;

        // prepare shader
        use_program(state, state->render_state->cubemap_shader);
        set_body_matrices(state, body, scene_origin);

        // bind cubemap texture
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

        // draw
        state->render_state->uv_sphere.draw();

        // unbind
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    } else {
        // equirectangular texture
        if (lighting) {
            use_program(state, state->render_state->lighting_shader);
        } else {
            use_program(state, state->render_state->base_shader);
        }
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

        // unbind
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

static void render_star(GlobalState* state, const glm::dvec3& scene_origin) {
    use_program(state, state->render_state->base_shader);

    set_picking_object(state, state->root);
    render_body(state, state->root, scene_origin, false);

    clear_picking_object(state);
}

static void render_bodies(GlobalState* state, const glm::dvec3& scene_origin) {
    for (auto key_value_pair : state->bodies) {
        auto body = key_value_pair.second;
        if (body == state->root) {
            continue;
        }
        set_picking_object(state, body);
        render_body(state, body, scene_origin);
        clear_picking_object(state);
    }

    auto model = glm::mat4(1.f);
    auto position = body_global_position_at_time(state->rocket.orbit->primary, state->time)
        - scene_origin
        + state->rocket.state.position;
    model = glm::translate(model, glm::vec3(position));
    model *= glm::mat4(glm::toMat4(state->rocket.orientation));
    state->render_state->model_matrix = model;
    update_matrices(state);

    set_picking_object(state, &state->rocket);
    glBindTexture(GL_TEXTURE_2D, 0);
    state->render_state->rocket_model.draw();
    clear_picking_object(state);
}

static double glow_size(double radius, double temperature, double distance) {
    // from https://www.seedofandromeda.com/blogs/51-procedural-star-rendering
    static const double sun_radius = 696e6;
    static const double sun_surface_temperature = 5778.0;

    double luminosity = (radius / sun_radius) * pow(temperature / sun_surface_temperature, 4.);
    return 1e17 * pow(luminosity, 0.25) / pow(distance, 0.5);
}

static GLuint init_lens_flare(void) {
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

        // x, y, z, u, v, offset
        data[k++] = -s; data[k++] = -s; data[k++] = 0.f; data[k++] = l + 0.0f; data[k++] = 0.f; data[k++] = o;
        data[k++] = +s; data[k++] = -s; data[k++] = 0.f; data[k++] = l + 0.5f; data[k++] = 0.f; data[k++] = o;
        data[k++] = -s; data[k++] = +s; data[k++] = 0.f; data[k++] = l + 0.0f; data[k++] = 1.f; data[k++] = o;

        data[k++] = -s; data[k++] = +s; data[k++] = 0.f; data[k++] = l + 0.0f; data[k++] = 1.f; data[k++] = o;
        data[k++] = +s; data[k++] = -s; data[k++] = 0.f; data[k++] = l + 0.5f; data[k++] = 0.f; data[k++] = o;
        data[k++] = +s; data[k++] = +s; data[k++] = 0.f; data[k++] = l + 0.5f; data[k++] = 1.f; data[k++] = o;
    }

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, n, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    free(data);

    return vbo;
}

static void render_lens_flare(GlobalState* state, const glm::dvec3& scene_origin, float visibility) {
    static GLuint lens_flare_vbo = 0;
    if (lens_flare_vbo == 0) {
        lens_flare_vbo = init_lens_flare();
    }

    auto position = body_global_position_at_time(state->root, state->time) - scene_origin;
    glm::vec3 light_source = position;

    float size = .1f;
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float aspect = float(viewport[2]) / float(viewport[3]);  // width / height
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

static void render_star_glow(GlobalState* state, const glm::dvec3& scene_origin, GLuint occlusion_query_buffer[2]) {
    if (state->render_state->picking_active) {
        return;
    }

    // NOTE: the visibility of the star glow (and associated lens flare) is
    // decided using occlusion queries; this means querying the GPU for the
    // number of rendered samples, which stalls until the rendering is done; to
    // mitigate this, we use the query of the previous frame; with this
    // approach, there will still be stalling between two frames, but not in
    // the middle of one (avoids compounding effects from several queries)
    // Update queries from previous frame, or create the query objects if needed

    float visibility;
    if (occlusion_query_buffer[0] == 0) {
        glGenQueries(2, occlusion_query_buffer);
        visibility = 1.f;
    } else {
        // occlusion querying causes performance warnings
        glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_PERFORMANCE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_FALSE);
        // query the total number of samples rendered without depth test
        int total_samples = 0;
        glGetQueryObjectiv(occlusion_query_buffer[0], GL_QUERY_RESULT, &total_samples);
        // query the number of samples that pass the depth test
        int passed_samples= 0;
        glGetQueryObjectiv(occlusion_query_buffer[1], GL_QUERY_RESULT, &passed_samples);
        // restore performance warnings
        glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_PERFORMANCE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
        // deduce visibility of star glow
        if (total_samples == 0) {
            // force glow when star is so far no pixel might be rendered
            visibility = 1.f;
        } else if (passed_samples == 0) {
            visibility = 0.0f;
        } else {
            float param = (float) passed_samples/ (float) total_samples;
            visibility = 1.f - expf(-4.f * param);
        }
    }

    auto position = body_global_position_at_time(state->root, state->time) - scene_origin;
    glm::vec3 star_glow_position = position;
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

    // Query for total samples
    glBeginQuery(GL_SAMPLES_PASSED, occlusion_query_buffer[0]);
    glDisable(GL_DEPTH_TEST);
    state->render_state->square.draw();
    glEnable(GL_DEPTH_TEST);
    glEndQuery(GL_SAMPLES_PASSED);

    // Query for passed samples
    glBeginQuery(GL_SAMPLES_PASSED, occlusion_query_buffer[1]);
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

static void render_position_markers(GlobalState* state, const glm::dvec3& scene_origin) {
    // draw circles around celestial bodies when from far away
    glPointSize(20);
    use_program(state, state->render_state->position_marker_shader);
    set_color(1, 0, 0, .5);
    OrbitSystem(state->root, scene_origin, state->time).draw();
}

static void render_orbits(GlobalState* state, const glm::dvec3& scene_origin) {
    use_program(state, state->render_state->base_shader);

    glPointSize(5);

    // unfocused orbits
    for (auto key_value_pair : state->bodies) {
        auto body = key_value_pair.second;
        if (is_ancestor_of(body, state->focus)) {
            continue;
        }
        if (body->orbit == NULL) {
            continue;
        }

        // transform matrices
        auto position = body_global_position_at_time(body->orbit->primary, state->time) - scene_origin;
        state->render_state->model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(position));
        update_matrices(state);

        // select color
        if (body == state->target) {
            set_color(1, 0, 0, .3f);
        } else {
            set_color(1, 1, 0, .1f);
        }

        // draw
        set_picking_object(state, body);
        state->render_state->orbit_meshes.at(body).draw();
        state->render_state->apses_meshes.at(body).draw();
        clear_picking_object(state);
    }

    // focused orbits
    for (auto key_value_pair : state->bodies) {
        auto body = key_value_pair.second;
        if (body == state->root || !is_ancestor_of(body, state->focus)) {
            continue;
        }

        // transform matrices
        auto position = body_global_position_at_time(body, state->time) - scene_origin;
        state->render_state->model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(position[0], position[1], position[2]));
        update_matrices(state);

        // select color
        if (body == state->target) {
            set_color(1, 0, 0);
        } else {
            set_color(1, 1, 0);
        }

        // draw
        set_picking_object(state, body);
        OrbitMesh(body->orbit, state->time, true).draw();
        OrbitApsesMesh(body->orbit, state->time, true).draw();
        clear_picking_object(state);
    }

    // rocket
    CelestialBody* body = &state->rocket;

    set_color(0, 1, 1);
    set_picking_object(state, body);
    if (body == state->focus) {
        auto position = body_global_position_at_time(body, state->time) - scene_origin;
        state->render_state->model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(position[0], position[1], position[2]));
        update_matrices(state);

        OrbitMesh(body->orbit, state->time, true).draw();
        OrbitApsesMesh(body->orbit, state->time, true).draw();
    } else {
        auto position = body_global_position_at_time(body->orbit->primary, state->time) - scene_origin;
        state->render_state->model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(position[0], position[1], position[2]));
        update_matrices(state);

        OrbitMesh(body->orbit, state->time).draw();
        OrbitApsesMesh(body->orbit, state->time).draw();
    }
    clear_picking_object(state);
}

static void render_helpers(GlobalState* state, const glm::dvec3& scene_origin) {
    if (!state->show_helpers) {
        return;
    }

    render_position_markers(state, scene_origin);
    render_orbits(state, scene_origin);
}

static void print_general_info(GlobalState* state, TextPanel* out) {
    // time warp
    if (state->real_timewarp < state->target_timewarp) {
        out->print("Time x%g (CPU-bound)\n", state->real_timewarp);
    } else {
        out->print("Time x%g\n", state->real_timewarp);
    }

    // local time
    if (std::string(state->root->name) == "Sun") {
        time_t simulation_time = J2000 + (time_t) state->time;
        struct tm* t = localtime(&simulation_time);
        char buffer[512];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %z", t);
        out->print("Date: %s\n", buffer);
    }

    // focus
    out->print("Focus: %s\n", state->focus->name);

    // target
    out->print("Target: %s\n", state->target != NULL ? state->target->name : "None");

    // distance
    char* s = human_quantity(state->view_altitude + state->focus->radius, "m");
    out->print("Distance: %s\n", s);
    free(s);

    // altitude
    s = human_quantity(state->view_altitude, "m");
    out->print("Altitude: %s\n", s);
    free(s);

    // FPS
    double now = real_clock();
    out->print("%.0f FPS (VSync %s)\n", state->fps, state->enable_vsync ? "on" : "off");

    // update FPS measure every second
    if (now - state->last_fps_measure > 1.) {
        state->fps = (double) state->n_frames_since_last / (now - state->last_fps_measure);
        state->n_frames_since_last = 0;
        state->last_fps_measure = now;
    }

    // version
    out->print("SAS: %s\n", state->rocket.sas_enabled ? "ON" : "OFF");

    // version
    out->print("Version " VERSION "\n");
}

static void print_orbital_info(GlobalState* state, TextPanel* out) {
    auto orbit = state->rocket.orbit;

    // orbit
    out->print("Orbit\n");
    out->print("\n");
    out->print("Primary %s\n", orbit->primary->name);
    out->print("Periapsis         %14.1f m\n", orbit->periapsis);
    out->print("Apoapsis          %14.1f m\n", orbit->apoapsis);
    out->print("Semi-major axis   %14.1f m\n", orbit->semi_major_axis);
    out->print("Semi-minor axis   %14.1f m\n", orbit->semi_minor_axis);
    out->print("Semi-latus rectum %14.1f m\n", orbit->semi_latus_rectum);
    out->print("Eccentricity      %16.3f\n", orbit->eccentricity);
    out->print("Longitude of AN         %6.1f deg\n", degrees(orbit->longitude_of_ascending_node));
    out->print("Inclination             %6.1f deg\n", degrees(orbit->inclination));
    out->print("Argument of periapsis   %6.1f deg\n", degrees(orbit->argument_of_periapsis));
    out->print("Period            %14.1f s\n", orbit->period);

    out->print("\n");
    out->print("\n");

    // current state
    double mean_anomaly = orbit_mean_anomaly_at_time(orbit, state->time);
    if (mean_anomaly < 0.) { mean_anomaly += 2 * M_PI; }
    double eccentric_anomaly = orbit_eccentric_anomaly_at_mean_anomaly(orbit, mean_anomaly);
    double true_anomaly = orbit_true_anomaly_at_eccentric_anomaly(orbit, eccentric_anomaly);
    auto pos = orbit_position_at_time(orbit, state->time);
    auto vel = orbit_velocity_at_time(orbit, state->time);
    out->print("Current State\n");
    out->print("\n");
    out->print("Altitude          %14.1f m\n", glm::length(pos) - orbit->primary->radius);
    out->print("Distance          %14.1f m\n", glm::length(pos));
    out->print("Mean anomaly            %6.1f deg\n", degrees(mean_anomaly));
    out->print("Eccentric anomaly       %6.1f deg\n", degrees(eccentric_anomaly));
    out->print("True anomaly            %6.1f deg\n", degrees(true_anomaly));
    out->print("Pitch:                  %6.1f deg\n", degrees(glm::pitch(state->rocket.orientation)));
    out->print("Yaw:                    %6.1f deg\n", degrees(glm::yaw(state->rocket.orientation)));
    out->print("Roll:                   %6.1f deg\n", degrees(glm::roll(state->rocket.orientation)));
    out->print("Orbital speed     %12.1f m/s\n", glm::length(vel));

    out->print("\n");
    out->print("\n");

    out->print("Timers\n");
    out->print("\n");
    double time_to_periapsis = orbit_time_at_true_anomaly(orbit, 0.) - state->time;
    double time_to_apospsis = orbit_time_at_true_anomaly(orbit, M_PI) - state->time;
    double time_to_ascending_node = orbit_time_at_true_anomaly(orbit, 2 * M_PI - orbit->argument_of_periapsis) - state->time;
    double time_to_descending_node = orbit_time_at_true_anomaly(orbit, M_PI - orbit->argument_of_periapsis) - state->time;
    double time_to_escape = orbit_time_at_escape(orbit) - state->time;
    if (time_to_periapsis < 0.) { time_to_periapsis += orbit->period; }
    if (time_to_ascending_node < 0.) { time_to_ascending_node += orbit->period; }
    if (time_to_descending_node < 0.) { time_to_descending_node += orbit->period; }
    out->print("Time to periapsis %14.1f s\n", time_to_periapsis);
    out->print("Time to apoapsis  %14.1f s\n", time_to_apospsis);
    out->print("Time to AN        %14.1f s\n", time_to_ascending_node);
    out->print("Time to DN        %14.1f s\n", time_to_descending_node);
    if (isnan(time_to_escape)) {
        out->print("Time to escape                   -\n", time_to_escape);
    } else {
        out->print("Time to escape    %14.1f s\n", time_to_escape);
    }
}

static void render_navball_sphere(GlobalState* state) {
    // view (bottom center)
    float w = (float) state->window_width;
    float h = (float) state->window_height;
    auto model = glm::translate(glm::mat4(1.0f), glm::vec3(w / 2.f, h - NAVBALL_RADIUS, -1e3f));

    // set size
    model = glm::scale(model, glm::vec3(NAVBALL_RADIUS));

    // rocket orientation
    model /= glm::mat4(glm::toMat4(state->rocket.orientation));

    // surface orientation
    model = glm::rotate(model, -M_PIf32 / 2.f, glm::vec3(0.f, 0.f, 1.f));
    auto dir = CelestialCoordinates::from_cartesian(state->rocket.state.position);
    model = glm::rotate(model, (float) dir.ecliptic_longitude, glm::vec3(0.f, 0.f, 1.f));
    model = glm::rotate(model, M_PIf32 - (float) dir.ecliptic_latitude, glm::vec3(1.f, 0.f, 0.f));

    // primary's tilt
    CelestialBody* body = state->rocket.orbit->primary;
    if (body->positive_pole != NULL) {
        // See diagram at the top of http://www.krysstal.com/sphertrig.html
        // point A = vertical
        // point B = rocket
        // point C = pole
        // we want to find angle B to orientate the navball towards the pole
        // cos b = cos a cos c + sin a sin c cos B
        // so:
        // B = acos((cos b - cos a cos c) / (sin a sin c))

        // A: vertical
        glm::dvec3 vert = {0, 0, body->radius};

        // B: rocket
        glm::dvec3 pos = state->rocket.state.position;

        // C: positive/north pole
        glm::dvec3 pole = {0, 0, body->radius};
        double x_angle = body->positive_pole->ecliptic_latitude - M_PI / 2.;
        pole = glm::dmat3(glm::rotate(glm::dmat4(1), x_angle, glm::dvec3(1., 0., 0.))) * pole;
        double z_angle = body->positive_pole->ecliptic_longitude - M_PI / 2.;
        pole = glm::dmat3(glm::rotate(glm::dmat4(1), z_angle, glm::dvec3(0., 0., 1.))) * pole;

        // deduce angles a, b, c
        double a = glm::angle(glm::normalize(pos),  glm::normalize(pole));
        double b = glm::angle(glm::normalize(pole), glm::normalize(vert));
        double c = glm::angle(glm::normalize(pos),  glm::normalize(vert));

        // deduce angle B
        double B = acos((cos(b) - cos(a)*cos(c)) / (sin(a) * sin(c)));

        // orient angle B
        if (glm::orientedAngle(glm::normalize(pos), glm::normalize(pole), glm::normalize(vert)) < 0) {
            B = -B;
        }

        model = glm::rotate(model, (float) B, glm::vec3(0.f, 0.f, 1.f));
    }

    // setup matrices
    state->render_state->model_matrix = model;
    update_matrices(state);

    // draw navball
    glBindTexture(GL_TEXTURE_2D, state->render_state->navball_texture);
    state->render_state->uv_sphere.draw();
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void render_navball_markers(GlobalState* state) {
    use_program(state, state->render_state->billboard_shader);

    // use orthographic projection
    state->render_state->view_matrix = glm::mat4(1.0f);
    state->render_state->projection_matrix = glm::ortho(0.f, (float) state->window_width, (float) state->window_height, 0.f, -2e3f, 2e3f);

    // bottom center
    float w = (float) state->window_width;
    float h = (float) state->window_height;
    auto model = glm::translate(glm::mat4(1.0f), glm::vec3(w / 2.f, h - NAVBALL_RADIUS, -1e3f));

    // convert state rocket to single precision
    glm::quat orientation = glm::inverse(state->rocket.orientation);
    glm::vec3 position = state->rocket.state.position;
    glm::vec3 velocity = state->rocket.state.velocity;

    // compute positions of markers
    float r = NAVBALL_RADIUS * 1.01f;
    glm::vec3 prograde = orientation * glm::normalize(velocity) * r;
    glm::vec3 normal = orientation * glm::normalize(glm::cross(position, velocity)) * r;
    glm::vec3 radial = glm::normalize(glm::cross(prograde, normal)) * r;

    glDisable(GL_DEPTH_TEST);

    // prograde / retrograde
    if (prograde[2] > 0) {
        state->render_state->model_matrix = glm::translate(model, -prograde);
        glBindTexture(GL_TEXTURE_2D, state->render_state->prograde_marker_texture);
    } else {
        state->render_state->model_matrix = glm::translate(model, prograde);
        glBindTexture(GL_TEXTURE_2D, state->render_state->retrograde_marker_texture);
    }
    update_matrices(state);
    state->render_state->navball_marker_mesh.draw();

    // normal / anti-normal
    if (normal[2] > 0) {
        state->render_state->model_matrix = glm::translate(model, -normal);
        glBindTexture(GL_TEXTURE_2D, state->render_state->normal_marker_texture);
    } else {
        state->render_state->model_matrix = glm::translate(model, normal);
        glBindTexture(GL_TEXTURE_2D, state->render_state->anti_normal_marker_texture);
    }
    update_matrices(state);
    state->render_state->navball_marker_mesh.draw();

    // radial-in / radial-out
    if (radial[2] <= 0) {
        state->render_state->model_matrix = glm::translate(model, radial);
        glBindTexture(GL_TEXTURE_2D, state->render_state->radial_in_marker_texture);
    } else {
        state->render_state->model_matrix = glm::translate(model, -radial);
        glBindTexture(GL_TEXTURE_2D, state->render_state->radial_out_marker_texture);
    }
    update_matrices(state);
    state->render_state->navball_marker_mesh.draw();

    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);
}

static void render_navball_frame(GlobalState* state) {
    use_program(state, state->render_state->hud_shader);

    // use orthographic projection
    state->render_state->view_matrix = glm::mat4(1.0f);
    state->render_state->projection_matrix = glm::ortho(0.f, (float) state->window_width, (float) state->window_height, 0.f, -2e3f, 2e3f);
    update_matrices(state);

    // general information
    float w = (float) state->window_width;
    float h = (float) state->window_height;

    // view (bottom center)
    auto model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(w / 2.f, h - NAVBALL_RADIUS, -1e3f));
    model = glm::scale(model, glm::vec3(2 * NAVBALL_FRAME_RADIUS));
    model = glm::scale(model, glm::vec3(1, -1, 1));

    // setup matrices
    state->render_state->model_matrix = model;
    update_matrices(state);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // render frame
    glBindTexture(GL_TEXTURE_2D, state->render_state->navball_frame_texture);
    state->render_state->square.draw();
    glBindTexture(GL_TEXTURE_2D, 0);

    float angle = (float) lerp(NEELDLE_MIN_ANGLE, NEELDLE_MAX_ANGLE, state->rocket.throttle);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(w / 2.f, h - NAVBALL_RADIUS, -1e3f));
    model = glm::rotate(model, angle, glm::vec3(0.f, 0.f, 1.f));
    model = glm::translate(model, glm::vec3(0.f, - NAVBALL_RADIUS - NEELDLE_LENGTH / 2.f, 0.f));
    model = glm::scale(model, glm::vec3(NEELDLE_LENGTH));

    // TODO: seriously
    model = glm::scale(model, glm::vec3(1.f, -1.f, 1.f));

    // setup matrices
    state->render_state->model_matrix = model;
    update_matrices(state);

    // render throttle needle
    glBindTexture(GL_TEXTURE_2D, state->render_state->throttle_needle_texture);
    state->render_state->square.draw();
    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

static void render_navball(GlobalState* state) {
    // enable write to stencil buffer
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);

    render_navball_sphere(state);

    // enable read of stencil buffer
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0x00);

    render_navball_markers(state);

    // disable stencil buffer
    glDisable(GL_STENCIL_TEST);

    render_navball_frame(state);
}

static void render_hud(GlobalState* state) {
    if (!state->show_hud) {
        return;
    }
    if (state->render_state->picking_active) {
        return;
    }

    use_program(state, state->render_state->hud_shader);

    // use orthographic projection
    state->render_state->view_matrix = glm::mat4(1.0f);
    state->render_state->projection_matrix = glm::ortho(0.f, (float) state->window_width, (float) state->window_height, 0.f, -2e3f, 2e3f);
    update_matrices(state);

    state->render_state->general_info.clear();
    print_general_info(state, &state->render_state->general_info);
    state->render_state->general_info.draw();

    state->render_state->orbital_info.clear();
    state->render_state->orbital_info.x = (float) state->window_width - 19.f * 20.f;
    print_orbital_info(state, &state->render_state->orbital_info);
    state->render_state->orbital_info.draw();

    if (state->show_help) {
        state->render_state->help.draw();
    }
    render_navball(state);
}

void render(GlobalState* state) {
    TRACE("Render started");
    if (state->render_state->picking_active) {
        state->render_state->picking_objects.clear();
    }

    glViewport(0, 0, state->window_width, state->window_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::dvec3 scene_origin;
    if (state->focus == &state->rocket) {
        scene_origin = state->rocket.state.position + body_global_position_at_time(state->rocket.orbit->primary, state->time);
    } else {
        scene_origin = body_global_position_at_time(state->focus, state->time);
    }

    if (state->show_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // main rendering
    TRACE("Main render started");
    render_skybox(state);
    render_bodies(state, scene_origin);
    static GLuint main_occlusion_query_buffer[2];
    render_star_glow(state, scene_origin, main_occlusion_query_buffer);
    render_helpers(state, scene_origin);
    render_star(state, scene_origin);
    TRACE("Main render dispatched");

    // thumbnail rendering
    if (state->view_altitude / state->focus->radius > THUMBNAIL_RATIO_THRESHOLD) {
        TRACE("Thumbnail render started");
        glViewport(10, 10, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
        glClear(GL_DEPTH_BUFFER_BIT);
        double view_altitude = state->view_altitude;
        state->view_altitude = state->focus->radius * THUMBNAIL_ALTITUDE_FACTOR;

        render_skybox(state);
        render_bodies(state, scene_origin);
        static GLuint thumbnail_occlusion_query_buffer[2];
        render_star_glow(state, scene_origin, thumbnail_occlusion_query_buffer);
        render_helpers(state, scene_origin);
        render_star(state, scene_origin);

        state->view_altitude = view_altitude;;
        TRACE("Thumbnail render finished");
    }

    glViewport(0, 0, state->window_width, state->window_height);
    glClear(GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    render_hud(state);
    TRACE("Render dispatched");
}

void set_picking_name(size_t name) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    GLint var = glGetUniformLocation(program, "picking_name");
    if (var >= 0) {
        glUniform3f(
            var,
            float((name >> 16) & 0xff) / 255.f,
            float((name >>  8) & 0xff) / 255.f,
            float((name >>  0) & 0xff) / 255.f
        );
    }
}

void set_picking_object(GlobalState* state, CelestialBody* object) {
    if (!state->render_state->picking_active) {
        return;
    }

    state->render_state->picking_objects.push_back(object);
    size_t name = state->render_state->picking_objects.size();
    state->render_state->current_picking_name = name;
    set_picking_name(name);
}

void clear_picking_object(GlobalState* state) {
    if (!state->render_state->picking_active) {
        return;
    }
    state->render_state->current_picking_name = 0;
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
    int cy = state->window_height - (int) state->cursor_y;

    int min_x = std::max(cx - search_radius, 0);
    int max_x = std::min(cx + search_radius, state->window_width - 1);
    int min_y = std::max(cy - search_radius, 0);
    int max_y = std::min(cy + search_radius, state->window_height - 1);

    int w = max_x - min_x + 1;
    int h = max_y - min_y + 1;

    unsigned char* components = new unsigned char[h * w * 4];
    glReadPixels(min_x, min_y, w, h, GL_RGBA,  GL_UNSIGNED_BYTE, components);

    size_t name = 0;
    int best_cursor_d2 = search_radius * search_radius;  // squared distance to cursor
    for (int y = 0; y < h; y += 1) {
        for (int x = 0; x < w; x += 1) {
            size_t candidate_name = 0;
            candidate_name |= ((size_t) components[(y*w + x)*4 + 0]) << 16;
            candidate_name |= ((size_t) components[(y*w + x)*4 + 1]) <<  8;
            candidate_name |= ((size_t) components[(y*w + x)*4 + 2]) <<  0;
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
        ERROR("Picked object %zu but only %zu known objects", name, n_objects);
        return NULL;
    } else {
        return state->render_state->picking_objects[name - 1];
    }
}
