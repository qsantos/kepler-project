extern "C" {
    #include "util.h"
    #include "texture.h"
}
#include "load.hpp"
#include "render.hpp"
#include "picking.hpp"

#include <GLFW/glfw3.h>
#include <cstring>

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
            state->timewarp /= 10.;
            fprintf(stderr, "Time warp: %f\n", state->timewarp);
        } else if (key == GLFW_KEY_PERIOD) {
            state->timewarp *= 10.;
            fprintf(stderr, "Time warp: %f\n", state->timewarp);
        } else if (key == GLFW_KEY_SLASH) {
            state->timewarp = 1.;
            fprintf(stderr, "Time warp: %f\n", state->timewarp);
        } else if (key == GLFW_KEY_I) {
            state->timewarp *= -1.;
            fprintf(stderr, "Time warp: %f\n", state->timewarp);
        } else if (key == GLFW_KEY_O) {
            state->show_helpers = !state->show_helpers;
        } else if (key == GLFW_KEY_H) {
            if (mods & GLFW_MOD_SHIFT) {
                state->show_hud = !state->show_hud;
            } else {
                state->show_help = !state->show_help;
            }
        } else if (key == GLFW_KEY_EQUAL) {
            if (std::string(state->root->name) == "Sun") {
                state->time = (double) (time(NULL) - J2000);
                fprintf(stderr, "Back to present\n");
            } else {
                state->time = 0.f;
                fprintf(stderr, "Back to epoch\n");
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
        state->view_theta += (x - state->cursor_x) / 4.;
        state->view_phi += (y - state->cursor_y) / 4.;

        // clamp to [-180, 0]
        state->view_phi = std::max(-180., std::min(state->view_phi, 0.));
    }

    state->cursor_x = x;
    state->cursor_y = y;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void) xoffset;
    GlobalState* state = static_cast<GlobalState*>(glfwGetWindowUserPointer(window));

    state->view_zoom *= pow(1.2, yoffset);
}

int main() {
    setlocale(LC_ALL, "");

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

    GlobalState state;
    glfwSetWindowUserPointer(window, &state);

    // enable OpenGL debugging
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    // GLFW callbacks
    glfwSetErrorCallback(error_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    state.cubemap_shader = make_program(3, "base", "cubemap", "logz");
    glUseProgram(state.cubemap_shader);
    glUniform1i(glGetUniformLocation(state.cubemap_shader, "cubemap_texture"), 0);  // TODO
    glUniform4f(glGetUniformLocation(state.cubemap_shader, "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);

    state.lighting_shader = make_program(4, "base", "lighting", "picking", "logz");
    glUseProgram(state.lighting_shader);
    glUniform4f(glGetUniformLocation(state.lighting_shader, "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(state.lighting_shader, "picking_active"), 0);

    state.position_marker_shader = make_program(4, "base", "position_marker", "picking", "logz");
    glUseProgram(state.position_marker_shader);
    glUniform4f(glGetUniformLocation(state.position_marker_shader, "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(state.position_marker_shader, "picking_active"), 0);

    state.base_shader = make_program(3, "base", "picking", "logz");
    glUseProgram(state.base_shader);
    glUniform4f(glGetUniformLocation(state.base_shader, "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(state.base_shader, "picking_active"), 0);

    state.star_glow_shader = make_program(3, "base", "star_glow", "logz");
    glUseProgram(state.star_glow_shader);
    glUniform4f(glGetUniformLocation(state.star_glow_shader, "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(state.star_glow_shader, "picking_active"), 0);

    state.lens_flare_shader = make_program(3, "base", "lens_flare", "logz");
    glUseProgram(state.lens_flare_shader);
    glUniform4f(glGetUniformLocation(state.lens_flare_shader, "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(state.lens_flare_shader, "picking_active"), 0);

    glGenVertexArrays(1, &state.vao);
    glBindVertexArray(state.vao);

    // initialize viewport
    glfwGetFramebufferSize(window, &state.viewport_width, &state.viewport_height);
    reset_matrices(&state);

    // fill default texture with white for convenience
    glBindTexture(GL_TEXTURE_2D, 0);
    float white_pixel[] = {1.f, 1.f, 1.f, 1.f};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, white_pixel);

    if (load_bodies(&state.bodies, "data/solar_system.json") < 0) {
        fprintf(stderr, "Failed to load '%s'\n", "data/solar_system.json");
        exit(EXIT_FAILURE);
    }
    for (auto key_value_pair : state.bodies) {
        auto name = key_value_pair.first;
        auto path = "data/textures/solar/" + name + ".jpg";
        state.body_textures[name] = load_texture(path.c_str());
    }
    state.star_glow_texture = load_texture("data/textures/star_glow.png");
    state.lens_flare_texture = load_texture("data/textures/lens_flares.png");
    state.focus = state.bodies.at("Earth");
    state.root = state.bodies.at("Sun");

    for (auto key_value_pair : state.bodies) {
        auto name = key_value_pair.first;
        auto body = key_value_pair.second;
        if (body->orbit == NULL) {
            continue;
        }
        state.orbit_meshes.emplace(name, body->orbit);
        state.apses_meshes.emplace(name, body->orbit);
    }

    // disable vsync
    // glfwSwapInterval(0);

    state.time = 0;
    if (std::string(state.root->name) == "Sun") {
        state.time = (double) (time(NULL) - J2000);
    }

    state.last_simulation_step = real_clock();
    state.last_fps_measure = real_clock();

    // main loop
    while (!glfwWindowShouldClose(window)) {
        // update time
        double now = real_clock();
        state.time += (now - state.last_simulation_step) * state.timewarp;
        state.last_simulation_step = now;

        render(&state);

        glfwSwapBuffers(window);
        glfwPollEvents();
        state.n_frames_since_last += 1;
    }

    glfwTerminate();
    return 0;
}
