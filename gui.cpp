extern "C" {
    #include "util.h"
    #include "texture.h"
}
#include "mesh.hpp"
#include "cubemap.hpp"
#include "shaders.hpp"
#include "body.hpp"
#include "load.hpp"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

#include <iostream>

using std::cout;
using std::endl;
using std::map;
using std::string;

struct RenderState {
    double time = 0.;
    double timewarp = 1.;
    map<string, CelestialBody*> bodies;
    map<string, GLuint> body_textures;
    map<string, OrbitMesh> orbit_meshes;
    CelestialBody* focus;
    GLuint base_shader;
    GLuint cubemap_shader;
    GLuint lighting_shader;
    Cubemap skybox = Cubemap(10, "data/textures/skybox/GalaxyTex_{}.jpg");
    GLuint vao;
    bool drag_active = false;
    double cursor_x;
    double cursor_y;
    double view_zoom = 1e-8;
    double view_theta = 0.;
    double view_phi = -90.;
    int windowed_x = 0;
    int windowed_y = 0;
    int windowed_width = 800;
    int windowed_height = 600;
    int viewport_width = 800;
    int viewport_height = 800;
    UVSphereMesh uv_sphere = UVSphereMesh(1, 64, 64);
    glm::mat4 model_view_projection_matrix;
    glm::mat4 model_view_matrix;
};

bool is_ancestor_of(CelestialBody* candidate, CelestialBody* target) {
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

void setup_matrices(RenderState* state, bool zoom=true) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 view = glm::mat4(1.0f);
    if (zoom) {
        view = glm::translate(view, glm::vec3(0.f, 0.f, -.1f / state->view_zoom));
    }
    view = glm::rotate(view, float(glm::radians(state->view_phi)), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, float(glm::radians(state->view_theta)), glm::vec3(0.0f, 0.0f, 1.0f));

    float aspect = float(state->viewport_width) / float(state->viewport_height);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, .1f, 1e7f);

    state->model_view_projection_matrix = proj * view * model;
    state->model_view_matrix = view * model;

    GLint uniMVP = glGetUniformLocation(program, "model_view_projection_matrix");
    glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(state->model_view_projection_matrix));

    GLint uniMV = glGetUniformLocation(program, "model_view_matrix");
    glUniformMatrix4fv(uniMV, 1, GL_FALSE, glm::value_ptr(state->model_view_matrix));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    RenderState* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window));

    state->viewport_width = width;
    state->viewport_height = height;
    glViewport(0, 0, width, height);
    setup_matrices(state);
}

void toggle_fullscreen(GLFWwindow* window) {
    RenderState* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window));

    GLFWmonitor* monitor = glfwGetWindowMonitor(window);
    if (monitor == NULL) {
        // save windowed state
        glfwGetWindowPos(window, &state->windowed_x, &state->windowed_y);
        glfwGetWindowSize(window, &state->windowed_width, &state->windowed_height);

        // enable fullscreen
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        // restore windowed state
        glfwSetWindowMonitor(window, NULL, state->windowed_x, state->windowed_y, state->windowed_width, state->windowed_height, 0);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void) scancode;
    (void) mods;
    RenderState* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        } else if (key == GLFW_KEY_F11) {
            toggle_fullscreen(window);
        } else if (key == GLFW_KEY_L) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else if (key == GLFW_KEY_F) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else if (key == GLFW_KEY_COMMA) {
            state->timewarp /= 10.;
            cout << "Time warp: "  << state->timewarp << endl;
        } else if (key == GLFW_KEY_PERIOD) {
            state->timewarp *= 10.;
            cout << "Time warp: "  << state->timewarp << endl;
        } else if (key == GLFW_KEY_SLASH) {
            state->timewarp = 1.;
            cout << "Time warp: "  << state->timewarp << endl;
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void) mods;
    RenderState* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window));

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            state->drag_active = true;
        } else if (action == GLFW_RELEASE) {
            state->drag_active = false;
        }
    }
}

static void cursor_position_callback(GLFWwindow* window, double x, double y) {
    RenderState* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window));

    if (state->drag_active) {
        state->view_theta += (x - state->cursor_x) / 4.;
        state->view_phi += (y - state->cursor_y) / 4.;

        state->view_phi = glm::clamp(state->view_phi, -180., 0.);

        setup_matrices(state);
    }

    state->cursor_x = x;
    state->cursor_y = y;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void) xoffset;
    RenderState* state = static_cast<RenderState*>(glfwGetWindowUserPointer(window));

    state->view_zoom *= pow(1.2, yoffset);
    setup_matrices(state);
}

void render_body(RenderState* state, CelestialBody* body, vec3 scene_origin) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto transform = state->model_view_matrix;
    auto position = body_global_position_at_time(body, state->time) - scene_origin;
    transform = glm::translate(transform, glm::vec3(position[0], position[1], position[2]));
    transform = glm::scale(transform, glm::vec3(float(body->radius)));

    // axial tilt
    if (body->north_pole != NULL) {
        double z_angle = body->north_pole->ecliptic_longitude - M_PI / 2.;
        transform = glm::rotate(transform, float(z_angle), glm::vec3(0.f, 0.f, 1.f));
        double x_angle = body->north_pole->ecliptic_latitude - M_PI / 2.;
        transform = glm::rotate(transform, float(x_angle), glm::vec3(1.f, 0.f, 0.f));
    }

    // OpenGL use single precision while Python has double precision
    // reducing modulo 2 PI in Python reduces loss of significance
    double turn_fraction = fmod(state->time / body->sidereal_day, 1.);
    transform = glm::rotate(transform, 2.f * M_PIf32 * float(turn_fraction), glm::vec3(0.f, 0.f, 1.f));

    GLint uniMV = glGetUniformLocation(program, "model_view_matrix");
    glUniformMatrix4fv(uniMV, 1, GL_FALSE, glm::value_ptr(transform));
    float aspect = float(state->viewport_width) / float(state->viewport_height);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, .1f, 1e7f);
    GLint uniMVP = glGetUniformLocation(program, "model_view_projection_matrix");
    glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(proj * transform));

    auto texture = state->body_textures.at(body->name);
    glBindTexture(GL_TEXTURE_2D, texture);
    state->uv_sphere.draw();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void render(RenderState* state) {
    glClearColor(0.f, 0.f, 0.f, .0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // skybox
    glUseProgram(state->cubemap_shader);
    setup_matrices(state, false);
    glDisable(GL_DEPTH_TEST);
    state->skybox.draw();
    glEnable(GL_DEPTH_TEST);

    glUseProgram(state->base_shader);
    setup_matrices(state);

    auto scene_origin = body_global_position_at_time(state->focus, state->time);

    CelestialBody* root = state->bodies.at("Sun");  // TODO
    render_body(state, root, scene_origin);

    // enable lighting
    glUseProgram(state->lighting_shader);
    setup_matrices(state);
    auto pos = body_global_position_at_time(root, state->time) - scene_origin;
    auto pos2 = state->model_view_matrix * glm::vec4(pos[0], pos[1], pos[2], 1.0f);
    GLint lighting_source = glGetUniformLocation(state->lighting_shader, "lighting_source");
    glUniform3fv(lighting_source, 1, glm::value_ptr(pos2));

    for (auto key_value_pair : state->bodies) {
        auto body = key_value_pair.second;
        if (body == root) {
            continue;
        }
        render_body(state, body, scene_origin);
    }

    glUseProgram(state->base_shader);
    setup_matrices(state);

    GLint colorUniform = glGetUniformLocation(state->base_shader, "u_color");
    glUniform4f(colorUniform, 1.0f, 1.0f, 0.0f, 0.2f);
    for (auto key_value_pair : state->bodies) {
        auto body = key_value_pair.second;
        if (is_ancestor_of(body, state->focus)) {
            continue;
        }
        if (body->orbit == NULL) {
            continue;
        }

        auto transform = state->model_view_matrix;
        auto position = body_global_position_at_time(body->orbit->primary, state->time) - scene_origin;
        transform = glm::translate(transform, glm::vec3(position[0], position[1], position[2]));

        GLint uniMV = glGetUniformLocation(state->base_shader, "model_view_matrix");
        glUniformMatrix4fv(uniMV, 1, GL_FALSE, glm::value_ptr(transform));
        float aspect = float(state->viewport_width) / float(state->viewport_height);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, .1f, 1e7f);
        GLint uniMVP = glGetUniformLocation(state->base_shader, "model_view_projection_matrix");
        glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(proj * transform));

        state->orbit_meshes.at(body->name).draw();
    }
    glUniform4f(colorUniform, 1.0f, 1.0f, 1.0f, 1.0f);

    glUniform4f(colorUniform, 1.0f, 1.0f, 0.0f, 1.0f);
    for (auto key_value_pair : state->bodies) {
        auto body = key_value_pair.second;
        if (body == root || !is_ancestor_of(body, state->focus)) {
            continue;
        }

        setup_matrices(state);
        auto transform = state->model_view_matrix;
        auto position = body_global_position_at_time(body, state->time) - scene_origin;
        transform = glm::translate(transform, glm::vec3(position[0], position[1], position[2]));

        GLint uniMV = glGetUniformLocation(state->base_shader, "model_view_matrix");
        glUniformMatrix4fv(uniMV, 1, GL_FALSE, glm::value_ptr(transform));
        float aspect = float(state->viewport_width) / float(state->viewport_height);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, .1f, 1e7f);
        GLint uniMVP = glGetUniformLocation(state->base_shader, "model_view_projection_matrix");
        glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(proj * transform));

        FocusedOrbitMesh(body->orbit, state->time).draw();
    }
    glUniform4f(colorUniform, 1.0f, 1.0f, 1.0f, 1.0f);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Orbit", NULL, NULL);
    if (window == NULL) {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    /* GLFW 3.3+
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    */

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        printf("GLEW init failed: %s!n", glewGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    RenderState state;
    glfwSetWindowUserPointer(window, &state);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // call framebuffer_size_callback() to initialize viewport
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        framebuffer_size_callback(window, width, height);
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    state.cubemap_shader = make_program({"base", "cubemap"});
    glUseProgram(state.cubemap_shader);
    glUniform1i(glGetUniformLocation(state.cubemap_shader, "cubemap_texture"), 0);  // TODO
    glUniform4f(glGetUniformLocation(state.cubemap_shader, "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);

    state.lighting_shader = make_program({"base", "lighting"});
    glUseProgram(state.lighting_shader);
    glUniform4f(glGetUniformLocation(state.lighting_shader, "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);

    state.base_shader = make_program({"base"});
    glUseProgram(state.base_shader);
    glUniform4f(glGetUniformLocation(state.base_shader, "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);

    glGenVertexArrays(1, &state.vao);
    glBindVertexArray(state.vao);

    setup_matrices(&state);

    // fill default texture with white for convenience
    glBindTexture(GL_TEXTURE_2D, 0);

    float white_pixel[] = {1.f, 1.f, 1.f, 1.f};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, white_pixel);

    if (load_bodies(&state.bodies, "solar_system.json") < 0) {
        fprintf(stderr, "Failed to load '%s'\n", "solar_system.json");
        exit(EXIT_FAILURE);
    }
    for (auto key_value_pair : state.bodies) {
        auto name = key_value_pair.first;
        auto path = "data/textures/solar/" + name + ".jpg";
        state.body_textures[name] = load_texture(path.c_str());
    }
    state.focus = state.bodies.at("Moon");

    for (auto key_value_pair : state.bodies) {
        auto name = key_value_pair.first;
        auto body = key_value_pair.second;
        if (body->orbit == NULL) {
            continue;
        }
        state.orbit_meshes.emplace(name, body->orbit);
    }

    // disable vsync
    // glfwSwapInterval(0);

    double last_simulation_step = real_clock();
    double last_fps_measure = real_clock();
    size_t n_frames_since_last = 0;

    // main loop
    while (!glfwWindowShouldClose(window)) {
        double now = real_clock();

        // check FPS
        if (now - last_fps_measure > 1.) {
            double fps = (double) n_frames_since_last / (now - last_fps_measure);
            printf("%.1f FPS\n", fps);
            n_frames_since_last = 0;
            last_fps_measure = now;
        }

        state.time += (now - last_simulation_step) * state.timewarp;
        last_simulation_step = now;

        render(&state);

        glfwSwapBuffers(window);
        glfwPollEvents();
        n_frames_since_last += 1;
    }

    glfwTerminate();
    return 0;
}
