#ifndef RENDER_HPP
#define RENDER_HPP

#include "cubemap.hpp"
#include "body.hpp"
extern "C" {
#include "shaders.h"
}

#include <GL/glew.h>
#include <map>
#include <vector>

using std::map;
using std::string;

struct RenderState;

RenderState* make_render_state(void);
void delete_render_state(RenderState* render_state);

struct GlobalState {
    double time = 0.;
    double timewarp = 1.;
    bool show_help = false;
    bool show_wireframe = false;
    bool show_helpers = true;
    bool show_hud = true;
    map<string, CelestialBody*> bodies;
    map<string, GLuint> body_textures;
    map<string, OrbitMesh> orbit_meshes;
    map<string, OrbitApsesMesh> apses_meshes;
    CelestialBody* root;
    CelestialBody* focus;
    GLuint base_shader;
    GLuint cubemap_shader;
    GLuint lighting_shader;
    GLuint position_marker_shader;
    GLuint star_glow_shader;
    GLuint lens_flare_shader;
    GLuint vao;

    GLint star_glow_texture;
    GLint lens_flare_texture;

    double last_simulation_step;
    double last_fps_measure;
    size_t n_frames_since_last = 0;

    bool drag_active = false;
    bool picking_active = false;
    std::vector<CelestialBody*> picking_objects;
    double cursor_x;
    double cursor_y;
    double view_zoom = 1e-7;
    double view_theta = 0.;
    double view_phi = -90.;
    int windowed_x = 0;
    int windowed_y = 0;
    int windowed_width = 1024;
    int windowed_height = 768;
    int viewport_width = 1024;
    int viewport_height = 768;

    RenderState* render_state = make_render_state();

    ~GlobalState() { delete_render_state(this->render_state); }
};

void reset_matrices(GlobalState* state, bool zoom=true);
void render(GlobalState* state);

#endif
