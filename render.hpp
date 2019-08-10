#ifndef RENDER_HPP
#define RENDER_HPP

#include "text_panel.hpp"
#include "cubemap.hpp"
#include "shaders.hpp"
#include "body.hpp"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <map>

using std::map;
using std::string;

struct RenderState {
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
    Cubemap skybox = Cubemap(10, "data/textures/skybox/GalaxyTex_{}.jpg");
    GLuint vao;

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
    TextPanel hud = TextPanel(5.f, 5.f);
    TextPanel help = TextPanel(5.f, 119.f);
    UVSphereMesh uv_sphere = UVSphereMesh(1, 64, 64);

    glm::mat4 model_matrix;
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
};

void reset_matrices(RenderState* state, bool zoom=true);
void render(RenderState* state);

#endif
