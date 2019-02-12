/*
 * ui.c
 * user interface utilities
 */
#include <inttypes.h>

#include "linmath.h"
#include "color.h"
#include "text.h"
#include "gl.h"
#include "ctx.h"
#include "ui.h"
#include "polygon.h"
#include "program.h"

void ui_drawbox(struct context *ctx, rect_t r, int w, rgba_t color)
{
	GLfloat verts[] = {
		/* Bottom */
		r.x1,      r.y1,
		r.x2,      r.y1,
		r.x1,      r.y1 + w,
		r.x1,      r.y1 + w,
		r.x2,      r.y1,
		r.x2,      r.y1 + w,

		/* Top */
		r.x1,      r.y2,
		r.x2,      r.y2,
		r.x1,      r.y2 - w,
		r.x1,      r.y2 - w,
		r.x2,      r.y2,
		r.x2,      r.y2 - w,

		/* Left */
		r.x1,      r.y1,
		r.x1 + w,  r.y1,
		r.x1,      r.y2,
		r.x1,      r.y2,
		r.x1 + w,  r.y1,
		r.x1 + w,  r.y2,

		/* Right */
		r.x2,      r.y1,
		r.x2 - w,  r.y1,
		r.x2,      r.y2,
		r.x2,      r.y2,
		r.x2 - w,  r.y1,
		r.x2 - w,  r.y2,
	};
	struct polygon p = polygon(verts, 6 * 4, 2);
	vec4_t vcolor = rgba2vec4(color);

	ctx_program(ctx, "constant");

	/* TODO: Should be set somewhere else */
	set_uniform_vec4(ctx->program, "color", &vcolor);
	polygon_draw(ctx, &p);
	polygon_release(&p);

	ctx_program(ctx, NULL);
}
