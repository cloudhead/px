//
// text.h
// text rendering
//
#include <stdbool.h>
#include <GL/glew.h>

struct context;

struct font {
	GLuint              vbo;
	GLuint              vao;
	struct texture     *tex;
	float               gw, gh;
};

bool load_font(struct font *, const char *, float, float);
void font_free(struct font *);
