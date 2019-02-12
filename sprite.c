//
// sprite.c
// sprite rendering and manipulation
//
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <GL/glew.h>

#include "linmath.h"
#include "assert.h"
#include "color.h"
#include "text.h"
#include "sprite.h"
#include "texture.h"
#include "util.h"
#include "program.h"
#include "gl.h"
#include "ctx.h"

static void skipspace(FILE *f)
{
	int c;

	do {
		c = fgetc(f);
	} while (isblank(c));

	ungetc(c, f);
}

bool load_sprite(struct sprite *s, const char *name, int fw, int fh)
{
	char              path[64];

	// Read image file
	snprintf(path, sizeof(path), "%s/%s.tga", RESOURCE_PATH, name);
	if ((s->texture = texture_load(path, GL_RGBA)) == NULL) {
		infof("sprite", "error loading texture '%s'", path);
		return false;
	}
	strcpy(s->name, name);

	s->fw      = fw;
	s->fh      = fh;
	s->nframes = s->texture->w * s->texture->h / (fw * fh);

	return true;
}

void draw_sprite
(
	struct spritebatch *sb,
	struct sprite      *s,       // Sprite containing the sprite to render
	int                 cellx,   // X index of the cell to render in the sprite
	int                 celly,   // Y index of the cell to render in the sprite
	float               sx,      // Screen 'x' coordinate to draw cell
	float               sy,      // Screen 'y' coordinate to draw cell
	float               w,       // Width of image to draw
	float               h,       // Height of image to draw
	float               rx,      // Repeat X
	float               ry,      // Repeat Y
	vec4_t              mul,     // Multiply color
	unsigned int        opts     // Render options
)
{
	assert(s);

	if (! sb)
		return;

	if (cellx * celly > s->nframes - 1)
		fatalf("sprite", "cell index %zu,%zu doesn't exist for sprite '%s'", cellx, celly, s->name);

	int tx = cellx * s->fw;
	int ty = celly * s->fh;

	float x1 = sx,
	      y1 = sy,
	      x2 = sx + w,
	      y2 = sy + h;

	if (opts & FLIPX) {
		x1 = x2;
		x2 = sx;
	}
	if (opts & FLIPY) {
		y1 = y2;
		y2 = sy;
	}

	spritebatch_add(sb,
		rect(tx, ty, tx + s->fw, ty + s->fh),
		rect(x1, y1, x2, y2),
		rx, ry,
		mul
	);
}

struct sprite *sprite(struct texture *t, rect_t r)
{
	struct sprite *s = malloc(sizeof(*s));

	s->rect          = r;
	s->texture       = t;

	return s;
}

////////////////////////////////////////////////////////////////////////////////

bool spritebatch_init(
	struct spritebatch *sb,
	struct texture     *tex
) {
	glGenVertexArrays(1, &sb->vao);
	glGenBuffers(1, &sb->vbo);

	sb->tex   = tex;
	sb->len   = 0;
	sb->cap   = SPRITEBATCH_INITIAL_SIZE;
	sb->data  = calloc(sb->cap * 6, sizeof(struct vertex)); // 6 vertices per quad

	return true;
}

void spritebatch_grow(struct spritebatch *sb)
{
	sb->cap   *= 2;
	sb->data   = realloc(sb->data, sb->cap * 6 * sizeof(struct vertex));
}

void spritebatch_shrink(struct spritebatch *sb)
{
	sb->cap    = sb->len;
	sb->data   = realloc(sb->data, spritebatch_size(sb));
}

inline size_t spritebatch_size(struct spritebatch *sb)
{
	return sb->len * 6 * sizeof(struct vertex);
}

void spritebatch_release(struct spritebatch *sb)
{
	glDeleteBuffers(1, &sb->vbo);
	glDeleteVertexArrays(1, &sb->vao);

	free(sb->data);
}

bool spritebatch_add(
	struct spritebatch     *sb,
	rect_t                  src,        // Source rect (texture)
	rect_t                  dst,        // Destination rect (screen)
	float                   xrep,       // X repeat
	float                   yrep,       // Y repeat
	vec4_t                  c           // Multiply color
) {
	assert(sb);
	assert(sb->tex);

	if (sb->len == sb->cap) {
		spritebatch_grow(sb);
		assert(sb->len < sb->cap);
	}

	int tw = sb->tex->w,
	    th = sb->tex->h;

	float rx1 = (float)src.x1 / (float)tw;              // Relative texture coordinates
	float ry1 = (float)src.y1 / (float)th;
	float rx2 = (float)src.x2 / (float)tw;
	float ry2 = (float)src.y2 / (float)th;

	GLfloat verts[] = {
		dst.x1,  dst.y1,  rx1 * xrep,  ry2 * yrep, c.x, c.y, c.z, c.w,
		dst.x2,  dst.y1,  rx2 * xrep,  ry2 * yrep, c.x, c.y, c.z, c.w,
		dst.x2,  dst.y2,  rx2 * xrep,  ry1 * yrep, c.x, c.y, c.z, c.w,
		dst.x1,  dst.y1,  rx1 * xrep,  ry2 * yrep, c.x, c.y, c.z, c.w,
		dst.x1,  dst.y2,  rx1 * xrep,  ry1 * yrep, c.x, c.y, c.z, c.w,
		dst.x2,  dst.y2,  rx2 * xrep,  ry1 * yrep, c.x, c.y, c.z, c.w
	};
	memcpy(sb->data + sb->len * 6, verts, elems(verts) * sizeof(float));

	sb->len ++;

	return true;
}

void spritebatch_reset(struct spritebatch *sb)
{
	sb->len = 0;
}

static inline void spritebatch_bind(struct spritebatch *sb)
{
	if (sb) {
		glBindVertexArray(sb->vao);
		glBindBuffer(GL_ARRAY_BUFFER, sb->vbo);

		texture_bind(sb->tex);
		return;
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	texture_bind(NULL);
}

static inline size_t spritebatch_vertices(struct spritebatch *sb)
{
	return sb->len * 6;
}

static inline void spritebatch_upload(struct spritebatch *sb)
{
	/* For each vertex, we have an x,y position, an s,t texture coordinate,
	 * and an r,g,b,a multiply color. In total, that's 8 floats. These
	 * are represented in the shader as two vec4's. */
	size_t size = spritebatch_vertices(sb) * sizeof(struct vertex);
	glBufferData(GL_ARRAY_BUFFER, size, sb->data, GL_DYNAMIC_DRAW);
}

void spritebatch_draw(struct spritebatch *sb, struct context *ctx)
{
	int nverts = (int)spritebatch_vertices(sb);

	ctx_program(ctx, "texture");

	// TODO: (perf) Why do we upload the buffer data every time we draw?
	// Upload data to video memory
	spritebatch_bind(sb);
	spritebatch_upload(sb);

	vec4_t bcolor = ctx->blend.color;

	// TODO: Look into using `glVertexAttribFormat` to separate the format
	// and source of the vertex array.
	// See: https://www.khronos.org/opengl/wiki/Vertex_Specification

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

	spritebatch_bind(NULL);
	ctx_program(ctx, NULL);
}

////////////////////////////////////////////////////////////////////////////////

void spritebatch_inittext(struct spritebatch *sb, struct font *f)
{
	spritebatch_init(sb, f->tex);
}

void spritebatch_vaddtext(struct spritebatch *sb, struct font *f, float *cursor, float sx, float sy, bool centered, const char *fmt, va_list ap)
{
	int offset = 32;
	char str[256] = {0};

	vsprintf(str, fmt, ap);

	if (cursor) {
		if (*cursor == 0) {
			*cursor = sx;
		} else {
			sx += *cursor;
		}
	}

	if (centered) {
		sx -= strlen(str) * f->gw / 2;
		sy -= f->gh / 2;
	}

	for (const char *c = str; *c; c++) {
		float x = (*c - offset) * f->gw;

		spritebatch_add(sb, rect(x, 0, x + f->gw, f->gh), rect(sx, sy + f->gh, sx + f->gw, sy), 1, 1, vec4(1, 1, 1, 1));
		sx += f->gw;

		if (cursor) *cursor += f->gw;
	}
}

void spritebatch_addtext(struct spritebatch *sb, struct font *f, float *cursor, float sx, float sy, bool centered, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	spritebatch_vaddtext(sb, f, cursor, sx, sy, centered, fmt, ap);
	va_end(ap);
}

void spritebatch_drawtext(struct spritebatch *sb, rgba_t color, struct context *ctx)
{
	ctx_blend(ctx, rgba2vec4(color), GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_ALPHA);
	spritebatch_draw(sb, ctx);
	ctx_blend_alpha(ctx);
}
