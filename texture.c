//
// texture.c
// texture utilities
//
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>

#include "linmath.h"
#include "texture.h"
#include "color.h"
#include "tga.h"
#include "program.h"
#include "assert.h"
#include "util.h"

struct texture *texture_load(const char *path, GLint format)
{
	struct tga         t;
	struct texture    *tx;

	if (! tga_load(&t, path)) {
		return NULL;
	}
	tx = texture(t.data, (int)t.width, (int)t.height, format);

	tx->data = t.data;

	/* tga_release(&t); */

	return tx;
}

struct texture *texture_read(rect_t r)
{
	int w = rect_w(&r);
	int h = rect_h(&r);

	struct texture *t = texture(NULL, w, h, GL_RGBA);

	texture_bind(t);

	glCopyTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0,
		0,
		(int)r.x1,
		(int)r.y1,
		w,
		h
	);
	texture_bind(NULL);

	return t;
}

void texture_bind(struct texture *t)
{
	if (t) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, t->handle);
		glBindSampler(0, t->sampler); // Bind sampler to texture unit 0
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

GLuint gen_sampler(GLuint minFilter, GLuint magFilter)
{
	GLuint sampler;
	glGenSamplers(1, &sampler);

	// Filter to use when texture is minified and magnified.
	glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, minFilter);
	glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, magFilter);

	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return sampler;
}

// TODO: Change argument order.
struct texture *texture(const void *pixels, int w, int h, GLint format)
{
	struct texture *t = malloc(sizeof(*t));

	assert(w <= 4096 && h <= 4096);

	t->sampler = gen_sampler(GL_NEAREST, GL_NEAREST);
	t->w       = w;
	t->h       = h;

	glGenTextures(1, &t->handle);
	glBindTexture(GL_TEXTURE_2D, t->handle);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,                    // Mipmap level
		format,               // Internal texel format
		w, h,                 // Width & height
		0,                    // Should always be 0
		GL_RGBA,              // Texel format of array
		GL_UNSIGNED_BYTE,     // Data type of the components
		pixels                // Data
	);
	glBindTexture(GL_TEXTURE_2D, 0);

	return t;
}

void texture_free(struct texture *t)
{
	glDeleteTextures(1, &t->handle);
	glDeleteSamplers(1, &t->sampler);

	free(t);
}
