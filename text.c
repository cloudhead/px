//
// text.c
// text rendering
//
// TODO: Implement text color.
// TODO: Implement text shadow.
//
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <GL/glew.h>

#include "linmath.h"
#include "color.h"
#include "tga.h"
#include "text.h"
#include "program.h"
#include "texture.h"
#include "sprite.h"
#include "gl.h"
#include "ctx.h"

bool load_font(struct font *f, const char *path, float gw, float gh)
{
	if ((f->tex = texture_load(path, GL_RGBA)) == NULL) {
		return false;
	}
	f->gw  = gw;
	f->gh  = gh;

	glGenVertexArrays(1, &f->vao);
	glGenBuffers(1, &f->vbo);

	return true;
}

void font_free(struct font *f)
{
	texture_free(f->tex);

	glDeleteVertexArrays(1, &f->vao);
	glDeleteBuffers(1, &f->vbo);

	free(f);
}
