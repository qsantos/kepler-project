extern "C" {
    #include "util.h"
}
#include "load.hpp"
#include "render.hpp"

#ifdef MSYS2
#include <windef.h>
#endif
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstring>

static const double SIMULATION_STEP = 1. / 128.;

// TODO
static const time_t J2000 = 946728000UL;  // 2000-01-01T12:00:00Z

void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    (void) length;
    (void) userParam;

    const char* source_str;
    switch (source) {
        case GL_DEBUG_SOURCE_API:             source_str = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_str = "window system"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "shader compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     source_str = "third party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     source_str = "application"; break;
        case GL_DEBUG_SOURCE_OTHER:           source_str = "other"; break;
        default:                              source_str = "unknown"; break;
    }

    const char* severity_str;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         severity_str = "error"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       severity_str = "warning"; break;
        case GL_DEBUG_SEVERITY_LOW:          severity_str = "info"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "debug"; break;
        default:                             severity_str = "unknown"; break;
    }
    const char* type_str;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               type_str = "error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "deprecated behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_str = "undefined behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         type_str = "portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         type_str = "performance"; break;
        case GL_DEBUG_TYPE_MARKER:              type_str = "marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          type_str = "push group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           type_str = "pop group"; break;
        case GL_DEBUG_TYPE_OTHER:               type_str = "other"; break;
        default:                                type_str = "unknown"; break;
    }

    fprintf(stderr, "[%s] %s (%s) [%#x]: %s", source_str, severity_str, type_str, id, message);
    if (message[strlen(message) - 1] != '\n') {
        fprintf(stderr, "\n");
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    GlobalState* state = static_cast<GlobalState*>(glfwGetWindowUserPointer(window));

    state->viewport_width = width;
    state->viewport_height = height;
    glViewport(0, 0, width, height);
}

void toggle_fullscreen(GLFWwindow* window) {
    GlobalState* state = static_cast<GlobalState*>(glfwGetWindowUserPointer(window));

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

    glfwSwapInterval(state->enable_vsync);
}

void error_callback(int error, const char* description) {
    fprintf(stderr, "[GLFW] [%#x] %s\n", error, description);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void) scancode;
    (void) mods;
    GlobalState* state = static_cast<GlobalState*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        } else if (key == GLFW_KEY_F11) {
            toggle_fullscreen(window);
        } else if (key == GLFW_KEY_W) {
            state->show_wireframe = !state->show_wireframe;
        } else if (key == GLFW_KEY_COMMA) {
            state->target_timewarp /= 10.;
        } else if (key == GLFW_KEY_PERIOD) {
            state->target_timewarp *= 10.;
        } else if (key == GLFW_KEY_SLASH) {
            state->target_timewarp = 1.;
        } else if (key == GLFW_KEY_I) {
            state->target_timewarp *= -1.;
        } else if (key == GLFW_KEY_O) {
            state->show_helpers = !state->show_helpers;
        } else if (key == GLFW_KEY_V) {
            state->enable_vsync = !state->enable_vsync;
        } else if (key == GLFW_KEY_H) {
            if (mods & GLFW_MOD_SHIFT) {
                state->show_hud = !state->show_hud;
            } else {
                state->show_help = !state->show_help;
            }
        } else if (key == GLFW_KEY_EQUAL) {
            if (std::string(state->root->name) == "Sun") {
                state->time = (double) (time(NULL) - J2000);
            } else {
                state->time = 0.f;
            }
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void) mods;
    GlobalState* state = static_cast<GlobalState*>(glfwGetWindowUserPointer(window));

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            CelestialBody* target = pick(state);
            if (target != NULL) {
                state->focus = target;
                printf("Switched to %s\n", target->name);
            }
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            state->drag_active = true;
        } else if (action == GLFW_RELEASE) {
            state->drag_active = false;
        }
    }
}

static void cursor_position_callback(GLFWwindow* window, double x, double y) {
    GlobalState* state = static_cast<GlobalState*>(glfwGetWindowUserPointer(window));

    if (state->drag_active) {
        double dx = x - state->cursor_x;
        double dy = y - state->cursor_y;

        // approximate map and drop at low altitude
        double H = state->view_altitude;
        double R = state->focus->radius;
        double dtheta = degrees(atan(dx / state->viewport_width * 2 * H / R));
        double dphi = degrees(atan(dy / state->viewport_height * H / R));

        // clamp speed at high altitude
        if (abs(dtheta) > abs(dx / 4.)) {
            dtheta = dx / 4.;
        }
        if (abs(dphi) > abs(dy / 4.)) {
            dphi = dy / 4.;
        }

        state->view_theta += dtheta;
        state->view_phi += dphi;

        // clamp to [-180, 0]
        state->view_phi = std::max(-180., std::min(state->view_phi, 0.));
    }

    state->cursor_x = x;
    state->cursor_y = y;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void) xoffset;
    GlobalState* state = static_cast<GlobalState*>(glfwGetWindowUserPointer(window));

    // each scroll unit effects the altitude by one decibel
    state->view_altitude *= pow(10., -yoffset / 10.);

    // clamp to [1 mm, 1 Tm]
    state->view_altitude = std::max(1e-3, std::min(state->view_altitude, 1e15));
}

GLFWwindow* init_glfw(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Orbit", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GFLW window\n");
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

    // GLFW callbacks
    glfwSetErrorCallback(error_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    return window;
}

void init_ogl(void) {
    // enable OpenGL debugging
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // fill default texture with white for convenience
    glBindTexture(GL_TEXTURE_2D, 0);
    float white_pixel[] = {1.f, 1.f, 1.f, 1.f};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, white_pixel);
}

int main() {
    setlocale(LC_ALL, "");

    GLFWwindow* window = init_glfw();
    init_ogl();

    GlobalState state;
    glfwSetWindowUserPointer(window, &state);

    if (load_bodies(&state.bodies, "data/solar_system.json") < 0) {
        fprintf(stderr, "Failed to load '%s'\n", "data/solar_system.json");
        exit(EXIT_FAILURE);
    }

    // initialize viewport
    glfwGetFramebufferSize(window, &state.viewport_width, &state.viewport_height);

    state.render_state = make_render_state(state.bodies);

    reset_matrices(&state);

    state.focus = state.bodies.at("Earth");
    state.root = state.bodies.at("Sun");

    state.time = 0;
    if (std::string(state.root->name) == "Sun") {
        state.time = (double) (time(NULL) - J2000);
    }

    Orbit orbit;
    state.rocket.name = "Rocket";
    state.rocket.radius = 100.;
    state.rocket.n_satellites = 0;
    state.rocket.orbit = &orbit;
    state.rocket.state = {
        vec3{6371e3 + 300e3, 0, 0},
        vec3{0, 7660, 0},
    },
    state.rocket.orbit_updated = 0.f;
    body_append_satellite(state.focus, &state.rocket);
    orbit_from_state(&orbit, state.focus, state.rocket.state, state.time);

    state.last_fps_measure = real_clock();
    state.focus = &state.rocket;

    double last = real_clock();
    double unprocessed_time = 0.;

    glfwSwapInterval(state.enable_vsync);

    // main loop
    while (!glfwWindowShouldClose(window)) {
        // update time
        double now = real_clock();
        double elapsed = now - last;
        unprocessed_time += elapsed * state.target_timewarp;
        last = now;

        size_t steps = 0;
        while (unprocessed_time >= SIMULATION_STEP && (real_clock() - last) < 1. / 64.) {
            rocket_update(&state.rocket, state.time, SIMULATION_STEP);
            unprocessed_time -= SIMULATION_STEP;
            state.time += SIMULATION_STEP;
            steps += 1;
        }

        if (unprocessed_time >= SIMULATION_STEP) {
            // we had to interrupt the simulation
            state.real_timewarp = (double) steps * SIMULATION_STEP / elapsed;
            // avoid accumulating unprocessed time that will have to be
            // processed even after the player has reduced time warp
            unprocessed_time = 0.;
        } else {
            // we simulated all the steps
            state.real_timewarp = state.target_timewarp;
        }

        render(&state);

        glfwSwapBuffers(window);
        glfwPollEvents();
        state.n_frames_since_last += 1;
    }

    glfwTerminate();
    return 0;
}
