//
// polygon.h
// polygon utilities
//
#include <GL/glew.h>
#include <stdio.h>

#include "util.h"

#define nvertices(v) (elems(v)/3)

struct polygon {
	GLuint vao, vbo;
	size_t nverts;
	size_t arity;
};

struct polygon polygon(GLfloat *, size_t, size_t);
void           polygon_release(struct polygon *);
void           polygon_draw(struct context *, struct polygon *);
struct polygon triangle(void);
struct polygon quad(void);
struct polygon line(line2_t, float);
struct polygon line_tapered(line2_t, float, float);
struct polygon rectangle(rect_t);
