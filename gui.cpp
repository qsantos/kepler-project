extern "C" {
    #include "util.h"
    #include "logging.h"
    #include "config.h"
}
#include "load.hpp"
#include "render.hpp"
#include "glm.hpp"

#ifdef MSYS2
#include <windef.h>
#endif
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstring>

static const double SIMULATION_STEP = 1. / 128.;
static const double TIMEWARP_FLOOR = 2.2250738585072014e-308;  // 0x1.0p-1022
static const double TIMEWARP_CEILING = 8.98846567431158e+307;  // 0x1.0p980
static const double THROTTLE_SPEED = .5;

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

    int level;
    const char* severity_str;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         severity_str = "ERROR";   level = 40; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       severity_str = "WARNING"; level = 30; break;
        case GL_DEBUG_SEVERITY_LOW:          severity_str = "INFO";    level = 20; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "DEBUG";   level = 10; break;
        default:                             severity_str = "UNKNOWN"; level = 50; break;
    }

    log_message(level, severity_str, "[OpenGL] (%s, %s, %#x): %s", source_str, type_str, id, message);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    GlobalState* state = static_cast<GlobalState*>(glfwGetWindowUserPointer(window));

    state->window_width = width;
    state->window_height = height;
    INFO("Window resized to %dx%d\n", width, height);
}

void toggle_fullscreen(GLFWwindow* window) {
    GlobalState* state = static_cast<GlobalState*>(glfwGetWindowUserPointer(window));

    GLFWmonitor* monitor = glfwGetWindowMonitor(window);
    if (monitor == NULL) {
        INFO("Switching to fullscreen");
        // save windowed state
        glfwGetWindowPos(window, &state->windowed_x, &state->windowed_y);
        glfwGetWindowSize(window, &state->windowed_width, &state->windowed_height);

        // enable fullscreen
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

        INFO("Switched to fullscreen");
    } else {
        INFO("Switching to windowed");
        // restore windowed state
        glfwSetWindowMonitor(window, NULL, state->windowed_x, state->windowed_y, state->windowed_width, state->windowed_height, 0);
        INFO("Switched to windowed");
    }

    glfwSwapInterval(state->enable_vsync);
}

void error_callback(int error, const char* description) {
    ERROR("[GLFW] (%#x) %s", error, description);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void) scancode;
    (void) mods;
    GlobalState* state = static_cast<GlobalState*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
            INFO("Exit required");
        } else if (key == GLFW_KEY_F11) {
            toggle_fullscreen(window);
        } else if (key == GLFW_KEY_T) {
            state->rocket.sas_enabled = !state->rocket.sas_enabled;
            if (state->rocket.sas_enabled) {
                INFO("SAS enabled");
            } else {
                INFO("SAS disnabled");
            }
        } else if (key == GLFW_KEY_Y) {
            state->show_wireframe = !state->show_wireframe;
            if (state->show_wireframe) {
                INFO("Wireframe mode ennabled");
            } else {
                INFO("Wireframe mode disnabled");
            }
        } else if (key == GLFW_KEY_COMMA) {
            if (state->target_timewarp / 2. >= TIMEWARP_FLOOR) {
                state->target_timewarp /= 2.;
                INFO("Reduced time-wrap to %g (%a)\n", state->target_timewarp, state->target_timewarp);
            }
        } else if (key == GLFW_KEY_PERIOD) {
            if (state->real_timewarp == state->target_timewarp && state->target_timewarp * 2 <= TIMEWARP_CEILING) {
                state->target_timewarp *= 2;
                INFO("Increased time-wrap to %g (%a)\n", state->target_timewarp, state->target_timewarp);
            }
        } else if (key == GLFW_KEY_SLASH) {
            state->target_timewarp = 1.;
            INFO("Reset time-wrap to %g (%a)\n", state->target_timewarp, state->target_timewarp);
        } else if (key == GLFW_KEY_I) {
            state->target_timewarp *= -1.;
            INFO("Inverted time-wrap to %g (%a)\n", state->target_timewarp, state->target_timewarp);
        } else if (key == GLFW_KEY_P) {
            state->paused = !state->paused;
            if (state->paused) {
                INFO("Paused");
            } else {
                INFO("Resumed");
            }
        } else if (key == GLFW_KEY_O) {
            state->show_helpers = !state->show_helpers;
            if (state->show_helpers) {
                INFO("Helpers enabled");
            } else {
                INFO("Helpers disabled");
            }
        } else if (key == GLFW_KEY_V) {
            state->enable_vsync = !state->enable_vsync;
            glfwSwapInterval(state->enable_vsync);
            if (state->enable_vsync) {
                INFO("VSync enabled");
            } else {
                INFO("VSync disabled");
            }
        } else if (key == GLFW_KEY_H) {
            if (mods & GLFW_MOD_SHIFT) {
                state->show_hud = !state->show_hud;
                if (state->show_hud) {
                    INFO("HUD enabled");
                } else {
                    INFO("HUD disabled");
                }
            } else {
                state->show_help = !state->show_help;
                if (state->show_help) {
                    INFO("Help enabled");
                } else {
                    INFO("Help disabled");
                }
            }
        } else if (key == GLFW_KEY_EQUAL) {
            if (std::string(state->root->name) == "Sun") {
                state->time = (double) (time(NULL) - J2000);
                INFO("Reset to current time");
            } else {
                state->time = 0.f;
                INFO("Reset to epoch");
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
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
                state->target = target;
                if (target) {
                    INFO("Switched target to %s", target->name);
                } else {
                    INFO("Target unselected");
                }
            } else if (target != NULL) {
                state->focus = target;
                INFO("Switched focus to %s", target->name);
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
        double dtheta = degrees(atan(dx / state->window_width * 2 * H / R));
        double dphi = degrees(atan(dy / state->window_height * H / R));

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
    DEBUG("GLFW initialization");

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Orbit", NULL, NULL);
    if (window == NULL) {
        CRITICAL("Failed to create GFLW window");
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
        CRITICAL("GLEW init failed: %s", glewGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // GLFW callbacks
    glfwSetErrorCallback(error_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    DEBUG("GLFW initialized");
    return window;
}

void init_ogl(void) {
    DEBUG("OpenGL initialization");

    // enable OpenGL debugging
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // fill default textures with white for convenience
    float white_pixel[] = {1.f, 1.f, 1.f, 1.f};

    // default 2D texture
    glBindTexture(GL_TEXTURE_2D, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, white_pixel);

    // default cubemap texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    for (int i = 0; i < 6; i += 1) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, white_pixel
        );
    }

    DEBUG("OpenGL initialized");
}

void update_rocket_soi(GlobalState* state) {
    auto rocket = &state->rocket;
    auto primary = rocket->orbit->primary;
    auto pos = rocket->state.position;
    auto vel = rocket->state.velocity;

    // switch to primary's parents SoI
    while (glm::length(pos) > primary->sphere_of_influence) {
        // change reference frame
        pos += orbit_position_at_time(primary->orbit, state->time);
        vel += orbit_velocity_at_time(primary->orbit, state->time);
        rocket->state = State{pos, vel};

        INFO("%s exited SoI from %s to %s", rocket->name, primary->name, primary->orbit->primary->name);
        primary = primary->orbit->primary;
    }

    // switch to satellite's SoI
    for (size_t i = 0; i < primary->n_satellites; i += 1) {
        auto satellite = primary->satellites[i];
        auto sat_pos = orbit_position_at_time(satellite->orbit, state->time);

        if (glm::distance(pos, sat_pos) < satellite->sphere_of_influence) {
            auto sat_vel = orbit_velocity_at_time(satellite->orbit, state->time);

            // change reference frame
            pos -= sat_pos;
            vel -= sat_vel;
            rocket->state = State{pos, vel};

            INFO("%s entered SoI of %s from %s", rocket->name, satellite->name, primary->name);
            primary = satellite;
            break;
        }
    }

    // update rocket orbit
    orbit_from_state(rocket->orbit, primary, rocket->state.position, rocket->state.velocity, state->time);
}

void usage(const char* name) {
    INFO("%s [--system (solar|kerbol)]", name);
}

int main(int argc, char** argv) {
    setlocale(LC_ALL, "");
    set_log_file("last.log");
    set_log_level(LOGLEVEL_INFO);
    INFO("Starting GUI");

    // parse args
    const char* system_id = "solar";
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (strcmp(arg, "--system") == 0 || strcmp(arg, "-s") == 0) {
            if (i >= argc - 1) {
                CRITICAL("Command-line argument %s missing value", arg);
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            system_id = argv[i + 1];
            i++;
        } else {
            CRITICAL("Unexpected command-line argument '%s'", arg);
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    Config config = load_config("data/config.json", system_id);

    GlobalState state;

    DEBUG("Loading %s config", config.system.display_name);
    if (load_bodies(&state.bodies, config.system.system_data) < 0) {
        CRITICAL("Failed to load '%s'", "data/kerbol_system.json");
        exit(EXIT_FAILURE);
    }
    DEBUG("Loaded %s config", config.system.display_name);

    GLFWwindow* window = init_glfw();
    init_ogl();

    glfwSetWindowUserPointer(window, &state);

    // initialize viewport
    glfwGetFramebufferSize(window, &state.window_width, &state.window_height);

    state.render_state = make_render_state(state.bodies, config.system.textures_directory);

    state.star_temperature = config.system.star_temperature;
    state.focus = state.bodies.at(config.system.default_focus);
    state.root = state.bodies.at(config.system.root);

    reset_matrices(&state);

    state.time = 0;
    if (std::string(state.root->name) == "Sun") {
        state.time = (double) (time(NULL) - J2000);
    }

    Orbit orbit;
    orbit_from_periapsis(&orbit, state.focus, state.focus->radius + config.system.spaceship_altitude, 0.);
    orbit_orientate(&orbit, 0., 0., 0., 0., 0.);
    state.rocket.name = "Rocket";
    state.rocket.radius = 5.;
    state.rocket.sphere_of_influence = 0.;
    state.rocket.n_satellites = 0;
    state.rocket.orbit = &orbit;
    state.rocket.state = {
        orbit_position_at_true_anomaly(&orbit, 0.),
        orbit_velocity_at_true_anomaly(&orbit, 0.),
    };
    body_append_satellite(state.focus, &state.rocket);

    delete_config(&config);

    state.last_fps_measure = real_clock();
    state.last_timewarp_measure = real_clock();
    state.focus = &state.rocket;

    double last = real_clock();
    double unprocessed_time = 0.;

    glfwSwapInterval(state.enable_vsync);

    // main loop
    while (!glfwWindowShouldClose(window)) {
        if (state.paused) {
            render(&state);
            glfwSwapBuffers(window);
            glfwPollEvents();
            state.n_frames_since_last += 1;
            continue;
        }

        // update time
        double now = real_clock();
        double elapsed = now - last;
        unprocessed_time += elapsed * state.target_timewarp;
        last = now;

        // update rocket state
        double n_steps;
        if (unprocessed_time < 0 || state.rocket.throttle == 0.) {
            n_steps = trunc(unprocessed_time / SIMULATION_STEP);
            unprocessed_time -= n_steps * SIMULATION_STEP;
            state.time += n_steps * SIMULATION_STEP;

            state.rocket.state.position = orbit_position_at_time(state.rocket.orbit, state.time);
            state.rocket.state.velocity = orbit_velocity_at_time(state.rocket.orbit, state.time);
        } else {
            n_steps = 0;
            while (unprocessed_time >= SIMULATION_STEP && (real_clock() - last) < 1. / 64.) {
                rocket_update(&state.rocket, state.time, SIMULATION_STEP, state.rocket.throttle * 100);
                unprocessed_time -= SIMULATION_STEP;
                state.time += SIMULATION_STEP;
                n_steps += 1;
            }
        }
        state.n_steps_since_last += n_steps;

        double k = SIMULATION_STEP * (double) n_steps * HACK_TO_KEEP_GLM_FROM_WRAPING_QUATERNION;
        state.rocket.orientation *= pow(state.rocket.angular_velocity_quat, k);

        update_rocket_soi(&state);

        if (unprocessed_time >= SIMULATION_STEP) {  // we had to interrupt the simulation
            // update time-warp measure every second
            if (now - state.last_timewarp_measure > 1.) {
                state.real_timewarp = (double) state.n_steps_since_last * SIMULATION_STEP / (now - state.last_timewarp_measure);
                state.n_steps_since_last = 0;
                state.last_timewarp_measure = now;
            }
            // avoid accumulating unprocessed time that will have to be
            // processed even after the player has reduced time warp
            unprocessed_time = 0.;
        } else {  // we simulated all the steps
            state.real_timewarp = state.target_timewarp;
        }

        render(&state);
        glfwSwapBuffers(window);
        glfwPollEvents();

        // throttle up
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                state.rocket.throttle = 1;
            } else {
                state.rocket.throttle += elapsed * THROTTLE_SPEED;
                if (state.rocket.throttle > 1.) {
                    state.rocket.throttle = 1;
                }
            }
        }

        // throttle down
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                state.rocket.throttle = 0;
            } else {
                state.rocket.throttle -= elapsed * THROTTLE_SPEED;
                if (state.rocket.throttle < 0.) {
                    state.rocket.throttle = 0;
                }
            }
        }

        // TODO: apply timewrap / pause logic to the part below

        // orientation
        double x = .04 / HACK_TO_KEEP_GLM_FROM_WRAPING_QUATERNION;
        bool user_input = false;

        // X
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            state.rocket.angular_velocity_quat *= glm::dquat(glm::dvec3(+x, 0, 0));
            user_input = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            state.rocket.angular_velocity_quat *= glm::dquat(glm::dvec3(-x, 0, 0));
            user_input = true;
        }

        // Y
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            state.rocket.angular_velocity_quat *= glm::dquat(glm::dvec3(0, +x, 0));
            user_input = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            state.rocket.angular_velocity_quat *= glm::dquat(glm::dvec3(0, -x, 0));
            user_input = true;
        }

        // Z
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            state.rocket.angular_velocity_quat *= glm::dquat(glm::dvec3(0, 0, -x));
            user_input = true;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            state.rocket.angular_velocity_quat *= glm::dquat(glm::dvec3(0, 0, +x));
            user_input = true;
        }

        // SAS
        if (state.rocket.sas_enabled && !user_input) {
            double l = angle(state.rocket.angular_velocity_quat);
            double sas_torque = .04 / HACK_TO_KEEP_GLM_FROM_WRAPING_QUATERNION;
            if (sas_torque > l) {
                state.rocket.angular_velocity_quat = glm::identity<glm::dquat>();
            } else {
                state.rocket.angular_velocity_quat = pow(state.rocket.angular_velocity_quat, 1. - sas_torque / l);
            }
        }

        state.n_frames_since_last += 1;
    }

    glfwTerminate();
    return 0;
}
