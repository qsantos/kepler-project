#include "text_panel.hpp"

extern "C" {
#include "texture.h"
}
#include <cstdio>
#include <cstdarg>

static const int character_width = 10;
static const int character_height = 19;

TextPanel::TextPanel(float x_, float y_) :
    x{x_},
    y{y_},
    font{load_texture("data/textures/font.png")}
{
    // set up buffer objects
    glGenBuffers(1, &this->vbo);
}

TextPanel::~TextPanel(void) {
    glDeleteBuffers(1, &this->vbo);
}

void append_vertex(TextPanel* self, int font_row, int font_col, int drow, int dcol) {
    int row = self->current_row;
    int col = self->current_col;

    self->data.push_back(self->x + float(col + dcol) * character_width);
    self->data.push_back(self->y + float(row + drow) * character_height);
    self->data.push_back(float(font_col + dcol) / 16.f);
    self->data.push_back((6.f - float(font_row + drow)) / 6.f);
}

void TextPanel::print(const char* format, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    int initial_col = this->current_col;

    for (char *p = buffer, c = *p; c; c = *(++p)) {
        if (c == ' ') {
            this->current_col += 1;
        } else if (c == '\t') {
            this->current_col += 4;
        } else if (c == '\n') {
            this->current_row += 1;
            this->current_col = initial_col;
        } else {
            // display non-printable characters as '?'
            if (!(32 <= c && c < 127)) {
                c = '?';
            }
            // locate the character in the font bitmap
            int font_row = (c - 32) / 16;
            int font_col = (c - 32) % 16;

            // append vertices
            append_vertex(this, font_row, font_col, 0, 0);
            append_vertex(this, font_row, font_col, 1, 0);
            append_vertex(this, font_row, font_col, 1, 1);
            append_vertex(this, font_row, font_col, 1, 1);
            append_vertex(this, font_row, font_col, 0, 1);
            append_vertex(this, font_row, font_col, 0, 0);

            // skip forward to next character
            this->current_col += 1;
        }
    }
}

void TextPanel::clear(void) {
    this->data.clear();
    this->current_row = 0;
    this->current_col = 0;

}
void TextPanel::bind(void) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);

    // vertices
    GLint var = glGetAttribLocation(program, "v_position");
    glEnableVertexAttribArray(var);
    glBufferData(GL_ARRAY_BUFFER, this->data.size() * sizeof(float), this->data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(var, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);

    // textures coordinates
    var = glGetAttribLocation(program, "v_texcoord");
    glEnableVertexAttribArray(var);
    glBufferData(GL_ARRAY_BUFFER, this->data.size() * sizeof(float), this->data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(var, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void TextPanel::draw(void) {
    this->bind();
    glBindTexture(GL_TEXTURE_2D, this->font);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei) this->data.size() / 4);
    glBindTexture(GL_TEXTURE_2D, 0);
}
