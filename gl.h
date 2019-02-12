#include <stdio.h>

void       gl_init(int, int, bool);
void       gl_clear(float, float, float, float);
void       gl_viewport(int, int);
void       gl_draw_triangles(int, int, int);
void       gl_draw_triangles_blend(int, int, int, vec4_t, GLenum, GLenum);

void _gl_errors(const char *, int);

struct vertex {
	vec2_t pos;
	vec2_t texcoord;
	vec4_t color;
};

#define gl_errors()       _gl_errors(__FILE__, __LINE__)
