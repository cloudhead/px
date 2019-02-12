//
// polygon.h
// polygon utilities
//
#include <GL/glew.h>
#include <stdlib.h>
#include <stdbool.h>

#include "linmath.h"
#include "texture.h"
#include "gl.h"
#include "ctx.h"
#include "polygon.h"
#include "assert.h"
#include "program.h"

struct polygon polygon(GLfloat *verts, size_t nverts, size_t arity)
{
	struct polygon poly = {0, 0, nverts, arity};

	glGenVertexArrays(1, &poly.vao);
	glGenBuffers(1, &poly.vbo);

	glBindVertexArray(poly.vao);
	glBindBuffer(GL_ARRAY_BUFFER, poly.vbo);

	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(GLfloat) * nverts * arity,
		verts,
		GL_STATIC_DRAW
	);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return poly;
}

struct polygon triangle(void)
{
	static GLfloat verts[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 0.0f,  1.0f, 0.0f,
	};
	return polygon(verts, 3, 3);
}

struct polygon quad(void)
{
	static GLfloat verts[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,     1.f, 1.f, 1.f, 1.f,
		 1.0f, -1.0f, 1.0f, 0.0f,     1.f, 1.f, 1.f, 1.f,
		-1.0f,  1.0f, 0.0f, 1.0f,     1.f, 1.f, 1.f, 1.f,
		-1.0f,  1.0f, 0.0f, 1.0f,     1.f, 1.f, 1.f, 1.f,
		 1.0f, -1.0f, 1.0f, 0.0f,     1.f, 1.f, 1.f, 1.f,
		 1.0f,  1.0f, 1.0f, 1.0f,     1.f, 1.f, 1.f, 1.f
	};
	return polygon(verts, 6, 8);
}

struct polygon line(line2_t l, float width)
{
	vec2_t v = vec2norm(vec2sub(l.p2, l.p1));

	float wx = width/2.f * v.y;
	float wy = width/2.f * v.x;

	GLfloat verts[] = {
		l.p1.x - wx,    l.p1.y + wy,
		l.p1.x + wx,    l.p1.y - wy,
		l.p2.x - wx,    l.p2.y + wy,
		l.p2.x - wx,    l.p2.y + wy,
		l.p1.x + wx,    l.p1.y - wy,
		l.p2.x + wx,    l.p2.y - wy,
	};
	return polygon(verts, 6, 2);
}

struct polygon line_tapered(line2_t l, float width, float taper)
{
	vec2_t v = vec2norm(vec2sub(l.p2, l.p1));

	float wx = width/2.f * v.y;
	float wy = width/2.f * v.x;

	GLfloat verts[] = {
		l.p1.x - wx,            l.p1.y + wy,
		l.p1.x + wx,            l.p1.y - wy,
		l.p2.x - wx * taper,    l.p2.y + wy * taper,
		l.p2.x - wx * taper,    l.p2.y + wy * taper,
		l.p1.x + wx,            l.p1.y - wy,
		l.p2.x + wx * taper,    l.p2.y - wy * taper,
	};
	return polygon(verts, 6, 2);
}

struct polygon rectangle(rect_t r)
{
	GLfloat verts[] = {
		r.x1,    r.y1,
		r.x2,    r.y1,
		r.x1,    r.y2,
		r.x1,    r.y2,
		r.x2,    r.y1,
		r.x2,    r.y2
	};
	return polygon(verts, 6, 2);
}

void polygon_release(struct polygon *p)
{
	glDeleteBuffers(1, &p->vbo);
	glDeleteVertexArrays(1, &p->vao);
}

void polygon_draw(struct context *ctx, struct polygon *poly)
{
	assert(poly->vao > 0);
	assert(poly->vbo > 0);
	assert(poly->nverts > 0);

	glBindVertexArray(poly->vao);
	glBindBuffer(GL_ARRAY_BUFFER, poly->vbo);

	gl_draw_triangles_blend(
		VERTEX_ATTR,
		(int)poly->nverts,
		(int)poly->arity,
		ctx->blend.color,
		ctx->blend.sfactor,
		ctx->blend.dfactor
	);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
