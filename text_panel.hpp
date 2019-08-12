#ifndef TEXT_PANEL_HPP
#define TEXT_PANEL_HPP

#ifdef MSYS2
#include <windef.h>
#endif
#include <GL/glew.h>

#include <vector>

struct TextPanel {
    TextPanel(float x, float y);
    ~TextPanel(void);
    void print(const char* format, ...);
    void clear(void);
    void bind(void);
    void draw(void);

    float x;
    float y;
    int current_row;
    int current_col;
    GLuint font;
    GLuint vbo;
    std::vector<float> data;
};

#endif
