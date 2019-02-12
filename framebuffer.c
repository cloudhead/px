//
// framebuffer.c
// Framebuffer support
//
#include <GL/glew.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "linmath.h"
#include "color.h"
#include "texture.h"
#include "assert.h"
#include "gl.h"
#include "ctx.h"
#include "polygon.h"
#include "text.h"
#include "framebuffer.h"
#include "util.h"
#include "program.h"

struct framebuffer *framebuffer_screen(int w, int h, void *pixels)
{
	struct framebuffer *fb = malloc(sizeof(*fb));

	glGenFramebuffers(1, &fb->handle);

	fb->tex = texture(pixels, w, h, GL_RGBA);
	fb->quad = quad();
	fb->screen = true;

	framebuffer_attach(fb, fb->tex);

	return fb;
}

struct framebuffer *framebuffer(int w, int h, void *pixels)
{
	return framebuffer_from(texture(pixels, w, h, GL_RGBA));
}

struct framebuffer *framebuffer_from(struct texture *t)
{
	struct framebuffer *fb = malloc(sizeof(*fb));
	glGenFramebuffers(1, &fb->handle);

	int w = t->w,
	    h = t->h;

	GLfloat verts[] = {
		0,  0,   0.0f, 1.0f,   1.f, 1.f, 1.f, 1.f,
		w,  0,   1.0f, 1.0f,   1.f, 1.f, 1.f, 1.f,
		0,  h,   0.0f, 0.0f,   1.f, 1.f, 1.f, 1.f,
		0,  h,   0.0f, 0.0f,   1.f, 1.f, 1.f, 1.f,
		w,  0,   1.0f, 1.0f,   1.f, 1.f, 1.f, 1.f,
		w,  h,   1.0f, 0.0f,   1.f, 1.f, 1.f, 1.f
	};

	fb->tex = t;
	fb->quad = polygon(verts, 6, 8);
	fb->screen = false;

	framebuffer_attach(fb, fb->tex);

	return fb;
}

void framebuffer_free(struct framebuffer *fb)
{
	texture_free(fb->tex);
	glDeleteFramebuffers(1, &fb->handle);
	free(fb);
}

void framebuffer_attach(struct framebuffer *fb, struct texture *tex)
{
	assert(fb->handle);
	assert(tex->w);
	assert(tex->h);
	assert(tex->handle);

	glBindFramebuffer(GL_FRAMEBUFFER, fb->handle);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex->handle, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		fatalf("framebuffer", "glCheckFramebufferStatus: error %u", status);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer_bind(struct framebuffer *fb)
{
	assert(fb);

	glBindFramebuffer(GL_FRAMEBUFFER, fb->handle);

	/* NB: It's not clear why calling glViewport here breaks things.
	 * Something must be awry in the way we use framebuffers.
	 * glViewport(0, 0, fb->tex->w, fb->tex->h); */
}

// TODO: Order of arguments is confusing
void framebuffer_draw(struct framebuffer *fb, struct context *ctx)
{
	struct texture *tex = fb->tex;
	GLuint vao = fb->quad.vao;
	GLuint vbo = fb->quad.vbo;
	int nverts = (int)fb->quad.nverts;

	// TODO: Get rid of distinction
	if (fb->screen) ctx_program(ctx, "framebuffer");
	else            ctx_program(ctx, "texture");

	assert(tex->sampler);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	texture_bind(fb->tex);

	vec4_t bcolor = ctx->blend.color;

	// TODO: This should come from polygon, as it knows its data
	glEnable(GL_BLEND);
	glBlendColor(bcolor.r, bcolor.g, bcolor.b, bcolor.a);
	glBlendFunc(ctx->blend.sfactor, ctx->blend.dfactor);
	glEnableVertexAttribArray(VERTEX_ATTR);
	glEnableVertexAttribArray(MULTIPLY_ATTR);
	glVertexAttribPointer(
		VERTEX_ATTR,                                  // Vertex attribute location
		4,                                            // Number of vertex components
		GL_FLOAT,                                     // Type
		GL_FALSE,                                     // Normalized?
		sizeof(struct vertex),                        // Stride
		(void*)0                                      // Offset
	);
	glVertexAttribPointer(
		MULTIPLY_ATTR,
		4,
		GL_FLOAT,
		GL_FALSE,
		sizeof(struct vertex),
		(void*)offsetof(struct vertex, color)
	);
	glDrawArrays(GL_TRIANGLES, 0, nverts);                // Draw from vertex 0 to N
	glDisableVertexAttribArray(VERTEX_ATTR);
	glDisableVertexAttribArray(MULTIPLY_ATTR);
	glDisable(GL_BLEND);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	texture_bind(NULL);
	ctx_program(ctx, NULL);
}

void framebuffer_draw_rect(struct context *ctx, struct framebuffer *fb, rect_t r, float sx, float sy)
{
	ctx_texture_draw_rect(ctx, fb->tex, (int)r.x1, (int)r.y1, (int)(rect_w(&r)), (int)(rect_h(&r)), sx, sy);
}

void framebuffer_clear(void)
{
	gl_clear(0.f, 0.f, 0.f, 0.f);
}

void framebuffer_clearcolor(float r, float g, float b, float a)
{
	gl_clear(r, g, b, a);
}

rgba_t framebuffer_sample(struct framebuffer *fb, int x, int y)
{
	framebuffer_bind(fb);

	rgba_t color;
	glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);

	return color;
}

void framebuffer_read(struct framebuffer *fb, rect_t r, rgba_t *buf)
{
	framebuffer_bind(fb);
	glReadPixels((int)r.x1, (int)r.y1, (int)r.x2, (int)r.y2, GL_RGBA, GL_UNSIGNED_BYTE, buf);
}
