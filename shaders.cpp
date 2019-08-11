#include "shaders.hpp"

extern "C" {
    #include "util.h"
}

#include <sstream>
#include <cstdarg>

using std::string;

// how is this not part of the STL?
static string replace(string s, string pattern, string replacement) {
    size_t pos = s.find(pattern);
    while (pos != string::npos) {
        s.replace(pos, pattern.size(), replacement);
        pos = s.find(pattern, pos + replacement.size());
    }
    return s;
}

void attach_shader(GLuint program, GLenum shader_type, const char* source, const char* filename) {
    GLuint shader = glCreateShader(shader_type);

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        const char* error = replace(infoLog, "0:", string(filename) + ":").c_str();
        fprintf(stderr, "ERROR: shader compilation failed\n%s", error);
        exit(EXIT_FAILURE);
    }

    glAttachShader(program, shader);
    glDeleteShader(shader);
}

void attach_shader_from_file(GLuint program, GLenum shader_type, const char* filename) {
    GLchar* source = load_file(filename);
    attach_shader(program, shader_type, source, filename);
    free(source);
}

GLuint make_program(int n_shaders, ...) {
    va_list shader_list;
    va_start(shader_list, n_shaders);
    const char** shaders = (const char**) MALLOC(n_shaders * sizeof(const char*));
    for (int i = 0; i < n_shaders; i++) {
        shaders[i] = va_arg(shader_list, const char*);
    }
    va_end(shader_list);

    GLuint program = glCreateProgram();

    // compile individual shaders
    for (int i = 0; i < n_shaders; i++) {
        const char* shader = shaders[i];
        attach_shader_from_file(program, GL_VERTEX_SHADER, ("data/shaders/" + string(shader) + ".vert").c_str());
        attach_shader_from_file(program, GL_FRAGMENT_SHADER, ("data/shaders/" + string(shader) + ".frag").c_str());
    }

    // source for shader with main() function that calls each shader in the given order
    std::ostringstream main;
    main << "#version 110\n";
    // declarations
    for (int i = 0; i < n_shaders; i++) {
        const char* shader = shaders[i];
        main << "void " << shader << "();\n";
    }
    // calls
    main << "void main() {\n";
    for (int i = 0; i < n_shaders; i++) {
        const char* shader = shaders[i];
        main << shader << "();\n";
    }
    main << "}\n";

    // attach main
    attach_shader(program, GL_VERTEX_SHADER, main.str().c_str(), "<main>");
    attach_shader(program, GL_FRAGMENT_SHADER, main.str().c_str(), "<main>");

    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::LINKING_FAILED\n%s", infoLog);
        exit(EXIT_FAILURE);
    }
    return program;
}
