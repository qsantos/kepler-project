extern "C" {
    #include "util.h"
    #include "texture.h"
}
#include "mesh.hpp"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

#include <iostream>

using std::cout;
using std::endl;

struct RenderState {
    GLuint current_shader;
    GLuint vao;
    bool drag_active = false;
    double cursor_x;
    double cursor_y;
    double view_zoom = 1e-7;
    double view_theta = 0.;
    double view_phi = -90.;
    int windowed_x = 0;
    int windowed_y = 0;
    int windowed_width = 800;
    int windowed_height = 600;
    int viewport_width = 800;
    int viewport_height = 800;
    UVSphereMesh uv_sphere = UVSphereMesh(600e3f, 64, 64);
};

GLuint compileVertexShader(const char* filename) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    GLchar* vertexShaderSource = load_file(filename);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    free(vertexShaderSource);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
        glfwTerminate();
        exit(1);
    }
    return vertexShader;
}

GLuint compileFragmentShader(const char* filename) {
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    GLchar* fragmentShaderSource = load_file(filename);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    free(fragmentShaderSource);
    glCompileShader(fragmentShader);

    GLint success;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
        glfwTerminate();
        exit(1);
    }
    return fragmentShader;
}

GLuint compileShaderProgram(const char* vertexFilename, const char* fragmentFilename) {
    GLuint shaderProgram = glCreateProgram();

    GLuint vertexShader = compileVertexShader(vertexFilename);
    GLuint fragmentShader = compileFragmentShader(fragmentFilename);
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::LINKING_FAILED\n" << infoLog << endl;
        glfwTerminate();
        exit(1);
    }
    return shaderProgram;
}

void setup_matrices(RenderState* state) {
    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.f, 0.f, -.1f / state->view_zoom));
    view = glm::rotate(view, float(glm::radians(state->view_phi)), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, float(glm::radians(state->view_theta)), glm::vec3(0.0f, 0.0f, 1.0f));

    float aspect = float(state->viewport_width) / float(state->viewport_height);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, .1f, 1e7f);

    auto model_view_projection_matrix = proj * view * model;
    auto model_view_matrix = view * model;

    GLint uniMVP = glGetUniformLocation(state->current_shader, "model_view_projection_matrix");
    glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(model_view_projection_matrix));

    GLint uniMV = glGetUniformLocation(state->current_shader, "model_view_matrix");
    glUniformMatrix4fv(uniMV, 1, GL_FALSE, glm::value_ptr(model_view_matrix));
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

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        } else if (key == GLFW_KEY_F11) {
            toggle_fullscreen(window);
        } else if (key == GLFW_KEY_L) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else if (key == GLFW_KEY_F) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

void render(RenderState* state) {
    glClearColor(0.f, 0.f, 0.f, .0f);
    glClear(GL_COLOR_BUFFER_BIT);

    state->uv_sphere.draw();
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
    //glEnable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND);
    //glEnable(GL_MULTISAMPLE);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    state.current_shader = compileShaderProgram("basic.vert", "basic.frag");
    glUseProgram(state.current_shader);

    glGenVertexArrays(1, &state.vao);
    glBindVertexArray(state.vao);

    GLint colorUniform = glGetUniformLocation(state.current_shader, "f_color");
    glUniform3f(colorUniform, 0.1f, 0.0f, 0.0f);
    setup_matrices(&state);

    glActiveTexture(GL_TEXTURE0);
    GLuint texture = load_texture("data/textures/kerbol/Kerbin.jpg");
    (void) texture;

    glUniform1i(glGetUniformLocation(state.current_shader, "texture"), 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // disable vsync
    // glfwSwapInterval(0);

    double last = real_clock();
    size_t n_frames_since_last = 0;

    // main loop
    while (!glfwWindowShouldClose(window)) {
        double now = real_clock();
        if (now - last > 1.) { 
            double fps = (double) n_frames_since_last / (now - last);
            printf("%.1f FPS\n", fps);
            n_frames_since_last = 0;
            last = now;
        }

        render(&state);

        glfwSwapBuffers(window);
        glfwPollEvents();
        n_frames_since_last += 1;
    }

    glfwTerminate();
    return 0;
}
