#include "shaders.h"

#include "util.h"

#include <stdio.h>
#include <stdarg.h>

void attach_shader(GLuint program, GLenum shader_type, const char* source, const char* filename) {
    GLuint shader = glCreateShader(shader_type);

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // check status and info log
    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    GLint info_log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
    GLchar* info_log = MALLOC((size_t) info_log_length);
    glGetShaderInfoLog(shader, info_log_length, &info_log_length, info_log);
    if (compile_status) {
        if (info_log_length == 0) {
            // OK
        } else {
            fprintf(stderr, "WARNING: compilation of %s generated warnings\n%s", filename, info_log);
        }
    } else {
        if (info_log_length == 0) {
            fprintf(stderr, "ERROR: compilation of %s failed (no info log)\n", filename);
        } else {
            fprintf(stderr, "ERROR: compilation of %s failed\n%s", filename, info_log);
        }
        free(info_log);
        exit(EXIT_FAILURE);
    }
    free(info_log);

    glAttachShader(program, shader);
    glDeleteShader(shader);
}

void attach_shader_from_file(GLuint program, GLenum shader_type, const char* filename) {
    GLchar* source = load_file(filename);
    attach_shader(program, shader_type, source, filename);
    free(source);
}

static int make_main(char* buffer, size_t n, size_t n_shaders, const char** shaders) {
    int ret;
    unsigned  s = 0;

    ret = snprintf(buffer, n, "#version 110\n");
    if (ret < 0) {
        return ret;
    }
    s += (unsigned) ret;

    // declarations
    for (size_t i = 0; i < n_shaders; i++) {
        ret = snprintf(buffer + s, n > 0 ? n - s : 0, "void %s();\n", shaders[i]);
        if (ret < 0) {
            return ret;
        }
        s += (unsigned) ret;
    }

    ret = snprintf(buffer + s, n > 0 ? n - s : 0, "void main() {\n");
    if (ret < 0) {
        return ret;
    }
    s += (unsigned) ret;

    // calls
    for (size_t i = 0; i < n_shaders; i++) {
        ret = snprintf(buffer + s, n > 0 ? n - s : 0, "%s();\n", shaders[i]);
        if (ret < 0) {
            return ret;
        }
        s += (unsigned) ret;
    }

    ret = snprintf(buffer + s, n > 0 ? n - s : 0, "}\n");
    if (ret < 0) {
        return ret;
    }
    s += (unsigned) ret;
    return (int) s;
}

GLuint make_program(size_t n_shaders, ...) {
    va_list shader_list;
    va_start(shader_list, n_shaders);
    const char** shaders = MALLOC(n_shaders * sizeof(const char*));
    for (size_t i = 0; i < n_shaders; i++) {
        shaders[i] = va_arg(shader_list, const char*);
    }
    va_end(shader_list);

    GLuint program = glCreateProgram();

    // compile individual shaders
    for (size_t i = 0; i < n_shaders; i++) {
        const char* shader = shaders[i];
        char path[512];

        // vertex shader
        snprintf(path, sizeof(path), "data/shaders/%s.vert", shader);
        attach_shader_from_file(program, GL_VERTEX_SHADER, path);

        // fragment shader
        snprintf(path, sizeof(path), "data/shaders/%s.frag", shader);
        attach_shader_from_file(program, GL_FRAGMENT_SHADER, path);
    }

    // generate source code main(), hich calls each shader in the given order
    char dummy_buffer[1];  // makes cppcheck happy
    int ret = make_main(dummy_buffer, 0, n_shaders, shaders) + 1;
    if (ret < 0) {
        free(shaders);
        fprintf(stderr, "Error while sizing the main shader (%i)\n", ret);
        exit(EXIT_FAILURE);
    }

    char* buffer = MALLOC((unsigned) ret);
    ret = make_main(buffer, (unsigned) ret, n_shaders, shaders);
    if (ret < 0) {
        free(buffer);
        free(shaders);
        fprintf(stderr, "Error while generating the main shader (%i)\n", ret);
        exit(EXIT_FAILURE);
    }

    // attach main
    attach_shader(program, GL_VERTEX_SHADER, buffer, "<main>");
    attach_shader(program, GL_FRAGMENT_SHADER, buffer, "<main>");

    free(buffer);
    free(shaders);

    glLinkProgram(program);

    // check status and info log
    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    GLint info_log_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
    GLchar* info_log = MALLOC((size_t) info_log_length);
    glGetProgramInfoLog(program, info_log_length, &info_log_length, info_log);
    if (link_status) {
        if (info_log_length == 0) {
            // OK
        } else {
            fprintf(stderr, "WARNING: linking of program generated warnings\n%s", info_log);
        }
    } else {
        if (info_log_length == 0) {
            fprintf(stderr, "ERROR: linking of program failed (no info log)\n");
        } else {
            fprintf(stderr, "ERROR: linking of program failed\n%s", info_log);
        }
        free(info_log);
        exit(EXIT_FAILURE);
    }
    free(info_log);


    return program;
}
