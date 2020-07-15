#ifndef RENDER_HPP
#define RENDER_HPP

#include "body.hpp"
#include "rocket.hpp"

#include <map>
#include <string>

struct RenderState;

RenderState* make_render_state(const std::map<std::string, CelestialBody*>& bodies);
void delete_render_state(RenderState* render_state);

struct GlobalState {
    bool show_help = false;
    bool show_wireframe = false;
    bool show_helpers = true;
    bool show_hud = true;
    bool enable_vsync = true;

    std::map<std::string, CelestialBody*> bodies;
    CelestialBody* root;
    CelestialBody* focus;
    CelestialBody* target = NULL;
    Rocket rocket;

    double fps = 60.;
    double last_fps_measure;
    size_t n_frames_since_last = 0;

    double time = 0.;
    double target_timewarp = 1.;
    double real_timewarp = 1.;
    double last_timewarp_measure;
    int n_steps_since_last = 0;

    bool drag_active = false;
    double cursor_x;
    double cursor_y;
    double view_altitude = 1e7;
    double view_theta = 0.;
    double view_phi = -90.;
    int windowed_x = 0;
    int windowed_y = 0;
    int windowed_width = 1024;
    int windowed_height = 768;
    int window_width = 1024;
    int window_height = 768;

    RenderState* render_state = NULL;

    ~GlobalState() { delete_render_state(this->render_state); }
};

void reset_matrices(GlobalState* state, bool zoom=true);
void render(GlobalState* state);

void set_picking_name(size_t name);
void set_picking_object(GlobalState* state, CelestialBody* object);
void clear_picking_object(GlobalState* state);
CelestialBody* pick(GlobalState* state);

#endif
