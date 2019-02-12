/*
 * px.c
 * pixel editor
 *
 * TODO: (bug) Palette is cut off on small windows.
 * TODO: (bug) Checker should be same size no matter view size.
 * TODO: (bug) Sometimes, eg. with a #ff00ff background, you can't see the selection.
 * TODO: (bug) `window` command does weird shit.
 * TODO: (bug) If 'cursor/down' is the first event, the replay doesn't happen properly.
 *
 * TODO: (test) We need to hash all non-cursor-move events and check them, to see where the tests fail.
 * TODO: (test) When the mouse isn't down, test/record shouldn't save all intermediate cursor positions.
 *
 * TODO: Grid-based pixel-mode navigation.
 * TODO: Allow `px` to be invoked with multiple files to edit.
 * TODO: Make it easy to see a 100% version of the view.
 * TODO  It's not possible to move a selection with the keyboard such that it's outside a view.
 * TODO: Snapshot should only save affected pixels.
 * TODO: Ping pong animation playback
 * TODO: Start px from any directory - package files somehow
 * TODO: Ctrl-P / Ctrl-N in command mode does backwards/forwards history search.
 * TODO: Saving a snapshot after undoing should delete all future snapshots.
 * TODO: Paint brush should be round.
 * TODO: Paint brush shouldn't show outside of view.
 * TODO: Convert all keyboard functions into commands, make bindings invoke commands.
 * TODO: All commands should check number of args.
 * TODO: Show what is in the paste buffer in pixel mode.
 */
#define _POSIX_C_SOURCE 200809L

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stropts.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <alloca.h>
#include <assert.h>

#if defined(DEBUG)
#include <assert.h>
#elif !defined(assert)
#define assert(A)
#endif

#include "color.h"
#include "text.h"
#include "sprite.h"
#include "gl.h"
#include "ctx.h"
#include "util.h"
#include "tga.h"
#include "polygon.h"
#include "program.h"
#include "texture.h"
#include "ui.h"
#include "animation.h"
#include "framebuffer.h"
#include "hash.h"

typedef float    f32;
typedef double   f64;

typedef uint64_t u64;
typedef int64_t  i64;

typedef uint32_t u32;
typedef int32_t  i32;

typedef uint16_t u16;
typedef int16_t  i16;

typedef uint8_t  u8;
typedef int8_t   i8;

#include "px.h"

#define PAL_SWATCH_SIZE                 24
#define point(x, y)                     ((struct point){(x), (y)})
#define WHITE                           rgba(255, 255, 255, 255)
#define BLACK                           rgba(0, 0, 0, 255)
#define GREY                            rgba(128, 128, 128, 255)
#define LIGHTGREY                       rgba(160, 160, 160, 255)
#define DARKGREY                        rgba(64, 64, 64, 255)
#define TRANSPARENT                     rgba(0, 0, 0, 0)
#define RED                             rgba(160, 0, 0, 255)
#define GRID_COLOR                      rgba(0, 0, 255, 128)
#define vw(v)                           (v->fw * v->nframes)
#define vh(v)                           (v->fh)

static bool source(struct session *, const char *);
static void session_view_blank(struct session *, char *, enum filestatus, int, int);
static void view_readpixels(struct view *s, rgba_t *);
static void palette_addcolor(struct palette *, rgba_t color);
static void palette_setcolor(struct palette *, rgba_t color, int);
static void draw_boundary(rgba_t color, int x, int y, int w, int h);

static void kb_create_frame(struct session *, const union arg *);
static void kb_create_view(struct session *, const union arg *);
static void kb_save(struct session *, const union arg *);
static void kb_move(struct session *, const union arg *);
static void kb_nudge(struct session *, const union arg *);
static void kb_next_view(struct session *, const union arg *);
static void kb_prev_view(struct session *, const union arg *);
static void kb_center_view(struct session *, const union arg *);
static void kb_px_copy(struct session *, const union arg *);
static void kb_px_cut(struct session *, const union arg *);
static void kb_px_paste(struct session *, const union arg *);
static void kb_px_cursor(struct session *, const union arg *);
static void kb_px_cursor_select(struct session *, const union arg *);
static void kb_px_move_frame(struct session *, const union arg *);
static void kb_px_select_frame(struct session *, const union arg *);
static void kb_pan(struct session *, const union arg *);
static void kb_swap_colors(struct session *, const union arg *);
static void kb_onion(struct session *, const union arg *);
static void kb_zoom(struct session *, const union arg *);
static void kb_flipx(struct session *, const union arg *);
static void kb_flipy(struct session *, const union arg *);
static void kb_undo(struct session *, const union arg *);
static void kb_redo(struct session *, const union arg *);
static void kb_pause(struct session *, const union arg *);
static void kb_brush_size(struct session *, const union arg *);
static void kb_adjust_fps(struct session *, const union arg *);
static void kb_brush(struct session *, const union arg *);
static void kb_eraser(struct session *, const union arg *);
static void kb_cmdmode(struct session *, const union arg *);
static void kb_pixelmode(struct session *, const union arg *);
static void kb_presentmode(struct session *, const union arg *);
static void kb_help(struct session *, const union arg *);

static struct session *session;

static bool cmd_quit(struct session *, int, char **);
static bool cmd_quit_forced(struct session *, int, char **);
static bool cmd_quit_all_forced(struct session *, int, char **);
static bool cmd_edit(struct session *, int, char **);
static bool cmd_write(struct session *, int, char **);
static bool cmd_write_quit(struct session *, int, char **);
static bool cmd_read(struct session *, int, char **);
static bool cmd_resize(struct session *, int, char **);
static bool cmd_slice(struct session *, int, char **);
static bool cmd_slice_all(struct session *, int, char **);
static bool cmd_checker(struct session *, int, char **);
static bool cmd_source(struct session *, int, char **);
static bool cmd_fullscreen(struct session *, int, char **);
static bool cmd_window(struct session *, int, char **);
static bool cmd_echo(struct session *, int, char **);
static bool cmd_palette_add(struct session *, int, char **);
static bool cmd_palette_clear(struct session *, int, char **);
static bool cmd_palette_sample(struct session *, int, char **);
static bool cmd_grid(struct session *, int, char **);
static bool cmd_pause(struct session *, int, char **);
static bool cmd_zoom(struct session *, int, char **);
static bool cmd_center(struct session *, int, char **);
static bool cmd_fill(struct session *, int, char **);
static bool cmd_crop(struct session *, int, char **);
static bool cmd_record(struct session *, int, char **);
static bool cmd_play(struct session *, int, char **);
static bool cmd_cursormove(struct session *, int, char **);
static bool cmd_cursordown(struct session *, int, char **);
static bool cmd_cursorup(struct session *, int, char **);
static bool cmd_sleep(struct session *, int, char **);
static bool cmd_test_digest(struct session *, int, char **);
static bool cmd_test_record(struct session *, int, char **);
static bool cmd_test_play(struct session *, int, char **);
static bool cmd_test_save(struct session *, int, char **);
static bool cmd_test_discard(struct session *, int, char **);
static bool cmd_test_check(struct session *, int, char **);

static struct command commands[] = {
	{"q",                  "quit",                            cmd_quit,                0},
	{"q!",                 "quit (forced)",                   cmd_quit_forced,         0},
	{"qa!",                "quit all (forced)",               cmd_quit_all_forced,     0},
	{"e",                  "edit",                            cmd_edit,                0},
	{"w",                  "write",                           cmd_write,               0},
	{"wq",                 "write & quit",                    cmd_write_quit,          0},
	{"resize",             "resize canvas",                   cmd_resize,              1},
	{"slice",              "slice canvas",                    cmd_slice,               1},
	{"slice!",             "slice canvas (all)",              cmd_slice_all,           1},
	{"checker",            "toggle checker",                  cmd_checker,             0},
	{"source",             "source script",                   cmd_source,              1},
	{"fullscreen",         "toggle fullscreen",               cmd_fullscreen,          0},
	{"window",             "window size",                     cmd_window,              1},
	{"echo",               "echo a message",                  cmd_echo,                0},
	{"p/add",              "add a palette color",             cmd_palette_add,         1},
	{"p/clear",            "clear the palette",               cmd_palette_clear,       0},
	{"p/sample",           "sample a palette",                cmd_palette_sample,      0},
	{"grid",               "toggle grid",                     cmd_grid,                0},
	{"pause",              "pause animation",                 cmd_pause,               0},
	{"zoom",               "zoom view",                       cmd_zoom,                1},
	{"center",             "center view",                     cmd_center,              0},
	{"fill",               "fill area",                       cmd_fill,                1},
	{"crop",               "crop view",                       cmd_crop,                0},
	{"record",             "record macro",                    cmd_record,              0},
	{"play",               "play macro",                      cmd_play,                1},
	{"cursor/move",        "cursor move",                     cmd_cursormove,          2},
	{"cursor/down",        "cursor down",                     cmd_cursordown,          0},
	{"cursor/up",          "cursor up",                       cmd_cursorup,            0},
	{"sleep",              "sleep",                           cmd_sleep,               1},
	{"test/digest",        "test/digest",                     cmd_test_digest,         0},
	{"test/record",        "test/record",                     cmd_test_record,         0},
	{"test/play",          "test/play",                       cmd_test_play,           1},
	{"test/save",          "test/save",                       cmd_test_save,           0},
	{"test/discard",       "test/discard",                    cmd_test_discard,        0},
	{"test/check",         "test/check",                      cmd_test_check,          1},
};

#include "config.h"

static void ui_drawtext(struct context *ctx, float *offset, float sx, float sy, rgba_t color, const char *fmt, ...)
{
	struct spritebatch sb;

	va_list ap;
	va_start(ap, fmt);

	spritebatch_inittext(&sb, ctx->font);
	spritebatch_vaddtext(&sb, ctx->font, offset, sx, sy, false, fmt, ap);
	spritebatch_drawtext(&sb, color, ctx);
	spritebatch_release(&sb);

	va_end(ap);
}

/* TODO: Take rect_t as argument */
static void fill_rect(struct context *ctx, int x1, int y1, int x2, int y2, rgba_t color)
{
	vec4_t v = rgba2vec4(color);
	float verts[] = {
		x1,  y1,
		x2,  y1,
		x1,  y2,
		x1,  y2,
		x2,  y1,
		x2,  y2,
	};
	struct polygon p = polygon(verts, 6, 2);

	ctx_program(ctx, "constant");
	set_uniform_vec4(ctx->program, "color", &v);
	polygon_draw(ctx, &p);
	polygon_release(&p);
}

static void draw_boundary(rgba_t color, int x1, int y1, int x2, int y2)
{
	ui_drawbox(session->ctx, rect(x1, y1, x2, y2), 1, color);
}

static void draw_vline(rgba_t color, int x, int y1, int y2)
{
	draw_boundary(
		color,
		x,     y1,
		x + 1, y2
	);
}

static void draw_hline(rgba_t color, int x1, int x2, int y)
{
	draw_boundary(
		color,
		x1,    y,
		x2   , y + 1
	);
}

static void draw_grid(rgba_t color, int gw, int gh, int totalw, int totalh)
{
	for (int i = gw; i < totalw; i += gw)
		draw_vline(color, i, 0, totalh);

	for (int i = gh; i < totalh; i += gh)
		draw_hline(color, 0, totalw, i);
}

static void message(enum msgtype t, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(session->message, sizeof(session->message) - 1, fmt, ap);
	va_end(ap);

	switch (t) {
	case MSG_INFO:   session->messagecolor = MSG_COLOR_WHITE;
			 infof("info", "%s", session->message); break;
	case MSG_ERR:    session->messagecolor = MSG_COLOR_RED;
			 infof("err", "\033[1;31m%s\033[0m", session->message); break;
	case MSG_ECHO:   session->messagecolor = MSG_COLOR_LIGHT;
			 infof("echo", "\033[0;33m%s\033[0m", session->message); break;
	case MSG_OK:     session->messagecolor = MSG_COLOR_GREEN;
			 infof("ok", "\033[1;32m%s\033[0m", session->message); break;
	case MSG_REPLAY: session->messagecolor = MSG_COLOR_GREY;  break;
	}
}

static void message_clear(struct session *s)
{
	s->message[0] = '\0';
}

static struct polygon brush_quad(float s)
{
	GLfloat verts[] = {
		0,  0,
		s,  0,
		0,  s,
		0,  s,
		s,  0,
		s,  s,
	};
	return polygon(verts, 6, 2);
}

/** VIEWS *********************************************************************/

static void view_filename(struct view *, const char *);
static void view_snapshot_save(struct context *, struct view *, bool);
static void view_draw_onionskin(struct context *, struct view *, int);
static void view_draw_checker(struct context *, struct view *, int);
static void view_dirty(struct view *);

static struct view *view
	( struct context *ctx
	, char *filename
	, enum filestatus fs
	, int fw
	, int fh
	, uint8_t *pixels
	, int start
	, int end
	)
{
	struct view *v = calloc(1, sizeof(*v));

	v->fw           = fw;
	v->fh           = fh;
	v->x            = 0;
	v->y            = 0;
	v->flipx        = false;
	v->flipy        = false;
	v->hover        = false;
	v->nframes      = (end - start) + 1;
	v->snapshot     = NULL;
	v->fb           = framebuffer(fw * v->nframes, fh, pixels);
	v->prev         = NULL;
	v->next         = NULL;
	v->filestatus   = fs;
	v->filename[0]  = '\0';

	if (filename)
		view_filename(v, filename);

	view_snapshot_save(ctx, v, fs == FILE_SAVED);

	return v;
}

static void view_filename(struct view *v, const char *filename)
{
	if (strlen(filename) + 1 > sizeof(v->filename)) {
		message(MSG_ERR, "Error: invalid filename size");
		return;
	}

	if (v->filename != filename)
		strcpy(v->filename, filename);
}

static void view_fileinfo(struct view *v, size_t n, char *fileinfo)
{
	if (v->filestatus == FILE_NEW) {
		snprintf(fileinfo, n, "%s [new]", v->filename);
	} else if (v->filestatus == FILE_MODIFIED) {
		snprintf(fileinfo, n, "%s [modified]", v->filename);
	} else if (v->filestatus == FILE_SAVED) {
		snprintf(fileinfo, n, "%s", v->filename);
	} else {
		snprintf(fileinfo, n, "");
	}
}

static rect_t view_rect(struct view *v)
{
	return rect(0, 0, vw(v), vh(v));
}

static rect_t view_frame_rect(struct view *v, int frame)
{
	return rect(frame * v->fw, 0, (frame + 1) * v->fw, vh(v));
}

static void view_free(struct view *v)
{
	framebuffer_free(v->fb);

	struct snapshot *s = v->snapshot, *next;

	while (s && s->prev)
		s = s->prev;

	while (s) {
		next = s->next;

		texture_free(s->pixels);
		free(s);

		s = next;
	}
	free(v);
}

static void view_offset(struct view *v, struct session *s, int *x, int *y)
{
	*x = v->x + s->x;
	*y = v->y + s->y;
}

static bool view_within(struct view *v, int x, int y, int zoom)
{
	int vx, vy;
	view_offset(v, session, &vx, &vy);

	return
		vx <= x && x < (vx + vw(v) * zoom) &&
		vy <= y && y < (vy + vh(v) * zoom);
}

static void view_hover(struct session *s, int x, int y)
{
	struct view *v = s->views;
	while (v) {
		v->hover = view_within(v, x, y, s->zoom);
		v = v->next;
	}
}

static inline int view_frame_at(struct view *v, int x, int y)
{
	return (x - v->x - session->x) / v->fw / session->zoom;
}

static void view_draw_frame(struct context *ctx, struct view *v, int frame)
{
	ctx_texture_draw_rect(ctx, v->fb->tex, frame * v->fw, 0, v->fw, v->fh, 0, 0);
}

static void view_draw_animation(struct context *ctx, struct view *v)
{
	double elapsed = ctx_time(ctx) - session->started;
	double frac    = session->fps * elapsed;
	int frame      = (int)(floor(frac)) % v->nframes;

	view_draw_frame(ctx, v, frame);
}

static void view_draw_onionskin(struct context *ctx, struct view *v, int frame)
{
	if (frame == 0 || frame > v->nframes - 1)
		return;

	float alpha = 0.5f / (float)frame;

	for (int i = 0; i < frame; i ++) {
		ctx_blend(ctx,
			vec4(0, 0, 0, alpha * (i + 1)),
			GL_CONSTANT_ALPHA, GL_DST_ALPHA
		);
		ctx_texture_draw_rect(
			ctx,
			v->fb->tex,
			v->fw * i,      0,
			v->fw,          v->fh,
			v->fw * frame,  0
		);
		ctx_blend_alpha(ctx); /* Default blending */
	}
}

static void view_readpixels(struct view *v, rgba_t *buf)
{
	framebuffer_read(v->fb, view_rect(v), buf);
}

static void view_resize_framebuffer(struct view *v, int w, int h, struct context *ctx)
{
	int cw = v->fb->tex->w,
	    ch = v->fb->tex->h;

	if (w != cw || h != ch) {
		struct framebuffer *fb = framebuffer(w, h, NULL);

		ctx_identity(ctx);

		framebuffer_bind(fb);
		framebuffer_clear();
		framebuffer_draw(v->fb, ctx);
		framebuffer_free(v->fb);

		v->fb = fb;
	}
}

static bool view_crop_framebuffer(struct view *v, rect_t crop, struct context *ctx)
{
	int w = rect_w(&crop);
	int h = rect_h(&crop);

	if (v->nframes > 1 && w % v->fw != 0)
		return false;

	struct framebuffer *fb = framebuffer(w, h, NULL);

	ctx_identity(ctx);

	framebuffer_bind(fb);
	framebuffer_clear();
	framebuffer_draw_rect(ctx, v->fb, rect_norm(rect_flipy(crop)), 0, 0);
	framebuffer_free(v->fb);
	// TODO: Why do we have to flip the rect above?

	/* If we have a single frame, we have to change the frame width,
	 * otherwise, we just change the number of frames, given that
	 * our crop width is a multiple of our frame width. */
	if (v->nframes == 1) {
		v->fw = w;
	} else {
		v->nframes = w / v->fw;
	}
	v->fh = h;
	v->fb = fb;

	view_snapshot_save(ctx, v, false);
	view_dirty(v);

	return true;
}

static void views_refresh(struct view *vs, int zoom)
{
	for (struct view *v = vs; v; v = v->next) {
		if (v->prev) {
			v->y = v->prev->y - v->fh * zoom - 25;
		}
	}
}

static void view_resize(struct view *v, int fw, int fh, struct context *ctx)
{
	view_resize_framebuffer(v, fw * v->nframes, fh, ctx);

	v->fw = fw;
	v->fh = fh;

	view_snapshot_save(ctx, v, false);
	view_dirty(v);
}

static void view_slice(struct view *v, int fw, int fh, struct context *ctx)
{
	v->nframes = v->fb->tex->w / fw;
	v->fw      = fw;
	v->fh      = fh;

	view_snapshot_save(ctx, v, false);
	view_dirty(v);
}

static void view_dirty(struct view *v)
{
	if (v->filestatus == FILE_SAVED)
		v->filestatus = FILE_MODIFIED;
}

static void view_saved(struct view *v)
{
	if (v->filestatus != FILE_NONE)
		v->filestatus = FILE_SAVED;
}

static void view_addframe(struct view *v, struct context *ctx)
{
	assert(v);

	int w = vw(v) + v->fw;
	int h = vh(v);

	view_resize_framebuffer(v, w, h, ctx);

	framebuffer_bind(v->fb);
	ctx_identity(ctx);

	ctx_texture_draw_rect(ctx, v->fb->tex,
		vw(v) - v->fw, 0,
		v->fw,         v->fh,
		vw(v),         0
	);

	v->nframes ++;

	view_snapshot_save(ctx, v, false);
	view_dirty(v);
}

static void view_snapshot_save(struct context *ctx, struct view *v, bool saved)
{
	int w = vw(v),
	    h = vh(v);

	struct snapshot *s = malloc(sizeof(*s));

	s->x          = 0;
	s->y          = 0;
	s->w          = w;
	s->h          = h;
	s->saved      = saved;
	s->next       = NULL;
	s->prev       = v->snapshot;
	s->nframes    = v->nframes;

	v->snapshot   = s;

	if (s->prev) {
		s->next       = s->prev->next;
		s->prev->next = s;
	}

	framebuffer_bind(v->fb);
	s->pixels = texture_read(view_rect(v));
	framebuffer_bind(ctx->screen);
}

static void view_snapshot_restore(struct context *ctx, struct view *v, struct snapshot *s)
{
	if (vw(v) != s->w || vh(v) != s->h) {
		view_resize_framebuffer(v, s->w, s->h, ctx);
	}

	framebuffer_bind(v->fb);
	framebuffer_clear();

	ctx_identity(ctx);

	ctx_texture_draw(ctx, s->pixels, 0, 0);
	framebuffer_bind(ctx->screen);

	if (s->saved) view_saved(v);
	else          view_dirty(v);

	v->fw       = s->w / s->nframes;
	v->fh       = s->h;
	v->nframes  = s->nframes;
	v->snapshot = s;
}

static bool view_save_as(struct view *v, const char *filename)
{
	if (!filename || !strlen(filename)) {
		message(MSG_ERR, "Error: no file name");
		return false;
	}
	int w = vw(v);
	int h = vh(v);

	rgba_t *tmp = malloc(sizeof(rgba_t) * w * h);
	view_readpixels(session->view, tmp);
	framebuffer_bind(session->ctx->screen);

	if (tga_save(tmp, w, h, 32, filename) != 0) {
		infof("px", "error: unable to save copy to '%s'", filename);
		return false;
	}
	free(tmp);

	view_filename(v, filename);
	v->filestatus = FILE_SAVED;
	v->snapshot->saved = true;
	message(MSG_INFO, "\"%s\" %d pixels written", filename, w * h);

	return true;
}

static void view_redo(struct context *ctx, struct view *v)
{
	if (! v->snapshot->next) { /* Max redos reached */
		return;
	}
	view_snapshot_restore(ctx, v, v->snapshot->next);
}

static void view_undo(struct context *ctx, struct view *v)
{
	if (! v->snapshot->prev) { /* Max undos reached */
		return;
	}
	view_snapshot_restore(ctx, v, v->snapshot->prev);
}

static void view_draw_boundaries(struct view *v)
{
	int x = v->x,
	    y = v->y;

	for (int i = 1; i < v->nframes; i++) {
		int bx = x + i * v->fw * session->zoom,
		    by = y;

		draw_vline(DARKGREY, bx, by, by + v->fh * session->zoom);

	}

	rgba_t color =
		v == session->view ?
			(session->mode == MODE_PIXEL ? RED : WHITE) : (v->hover ? LIGHTGREY : GREY);

	draw_boundary(
		color,
		x                         - 1,
		y                         - 1,
		x + vw(v) * session->zoom + 1,
		y + vh(v) * session->zoom + 1
	);

	if (session->checker.active && v->nframes > 1 && !session->paused) {
		draw_boundary(
			GREY,
			x - v->fw * session->zoom - 1,
			y                         - 1,
			x                         + 0,
			y + v->fh * session->zoom + 1
		);
	}
}

static void view_draw_grid(struct view *v, int gw, int gh, rgba_t color, int zoom)
{
	if (gw <= 0 || gh <= 0)
		return;

	int totalw = vw(v) * zoom;
	int totalh = vh(v) * zoom;

	gw *= zoom;
	gh *= zoom;

	draw_grid(color, gw, gh, totalw, totalh);
}

static void view_draw
	( struct context *ctx
	, struct view *v
	, float zoom
	, int mx
	, int my
	)
{
	view_draw_checker(ctx, v, v->nframes);

	ctx_save(ctx);
	ctx_scale(ctx, v->flipx ? -1 : 1, v->flipy ? -1 : 1);
	ctx_translate(ctx, v->flipx ? vw(v) * zoom : 0, v->flipy ? vh(v) * zoom : 0);

	framebuffer_draw(v->fb, ctx);

	ctx_restore(ctx);

	if (v->nframes > 1) {
		if (session->onion) {
			view_draw_onionskin(ctx, v, view_frame_at(v, (int)mx, (int)my));
		}
		if (! session->paused) {
			ctx_save(ctx);
			ctx_translate(ctx, -(v->fw * zoom), 0);
			view_draw_checker(ctx, v, 1);
			view_draw_animation(ctx, v);
			ctx_restore(ctx);
		}
	}
}

static void view_draw_checker(struct context *ctx, struct view *v, int nframes)
{
	struct spritebatch sb;

	if (! session->checker.active)
		return;

	float ratio   = (float)v->fw * nframes / (float)v->fh;
	float repeatx = session->zoom * ratio;
	float repeaty = session->zoom;

	spritebatch_init(&sb, session->checker.tex);
	spritebatch_add(&sb,
		rect(0, 0, session->checker.tex->w, session->checker.tex->h),
		rect(0, 0, v->fw * nframes, v->fh),
		repeatx,
		repeaty,
		vec4identity
	);
	spritebatch_draw(&sb, ctx);
	spritebatch_release(&sb);
}

/** SHORTCUTS *****************************************************************/

static void session_view_vcenter(struct session *, struct view *);
static void session_view_center(struct session *, struct view *);
static void session_zoom(struct session *, int);
static void session_tool_switch(struct session *, enum tooltype);
static void session_brush_paint(struct session *);
static void session_brush_erase(struct session *);

static void kb_save(struct session *s, const union arg *arg)
{
	view_save_as(s->view, s->view->filename);
}

static void kb_move(struct session *s, const union arg *arg)
{
	s->view->x += s->view->fw * -arg->p.x * s->zoom;
}

static void kb_nudge(struct session *s, const union arg *arg)
{
	s->x += -arg->p.x * 32;
	s->y += -arg->p.y * 32;
}

static void kb_next_view(struct session *s, const union arg *arg)
{
	if (s->view->next) {
		s->view = s->view->next;
		session_view_vcenter(s, s->view);
	}
}

static void kb_prev_view(struct session *s, const union arg *arg)
{
	if (s->view->prev) {
		s->view = s->view->prev;
		session_view_vcenter(s, s->view);
	}
}

static void kb_center_view(struct session *s, const union arg *arg)
{
	session_view_center(s, s->view);
}

static void session_copy_rect(struct session *s, rect_t *r)
{
	if (s->paste)
		texture_free(s->paste);

	s->paste = texture_read(rect_norm(*r));
}

static void kb_px_copy(struct session *s, const union arg *arg)
{
	framebuffer_bind(s->view->fb);
	session_copy_rect(s, &s->selection);
	framebuffer_bind(s->ctx->screen);
	message(MSG_INFO, "%d pixels copied", rect_w(&s->selection) * (int)rect_h(&s->selection));
}

static void kb_px_cut(struct session *s, const union arg *arg)
{
	framebuffer_bind(s->view->fb);
	ctx_identity(s->ctx);

	rect_t sel = rect_norm(s->selection);

	session_copy_rect(s, &sel);

	ctx_blend(s->ctx,
		vec4(0, 0, 0, 0),
		GL_ONE, GL_SRC_ALPHA
	);
	fill_rect
		( s->ctx
		, (int)sel.x1
		, (int)sel.y1
		, (int)sel.x2
		, (int)sel.y2
		, rgba(0, 0, 0, 0)
		);
	ctx_blend_alpha(s->ctx);
	framebuffer_bind(s->ctx->screen);
	view_snapshot_save(s->ctx, s->view, false);
	view_dirty(s->view);
}

static void kb_px_paste(struct session *s, const union arg *arg)
{
	if (! s->paste)
		return;

	framebuffer_bind(s->view->fb);
	ctx_identity(s->ctx);

	struct spritebatch sb;
	spritebatch_init(&sb, s->paste);
	spritebatch_add(&sb,
		rect(0, 0, s->paste->w, s->paste->h),
		rect_norm(s->selection),
		1, 1, vec4identity);
	spritebatch_draw(&sb, s->ctx);
	spritebatch_release(&sb);

	framebuffer_bind(s->ctx->screen);
	view_snapshot_save(s->ctx, s->view, false);
	view_dirty(s->view);

	message(MSG_INFO, "%d pixels pasted", s->paste->w * s->paste->h);
}

static void kb_px_cursor(struct session *s, const union arg *arg)
{
	rect_t r      = rect_translate(s->selection, arg->vec);
	vec2_t min    = rect_min(r);
	vec2_t max    = rect_max(r);

	if (
		min.x < 0           || min.y < 0 ||
		max.x > vw(s->view) || max.y > vh(s->view)
	)
		return;

	s->selection = r;
}

static void kb_px_cursor_select(struct session *s, const union arg *arg)
{
	float nx = s->selection.x2 + arg->vec.x;
	float ny = s->selection.y2 + arg->vec.y;

	if (nx <= vw(s->view) && nx >= 0)
		s->selection.x2 = nx;

	if (ny <= vh(s->view) && ny >= 0)
		s->selection.y2 = ny;

	/* Avoid zero-width/height rectangle */

	if (s->selection.x2 == s->selection.x1)
		s->selection.x2 += arg->vec.x;

	if (s->selection.y2 == s->selection.y1)
		s->selection.y2 += arg->vec.y;
}

static void kb_px_move_frame(struct session *s, const union arg *arg)
{
	rect_t sel = rect_translate(s->selection, vec2(s->view->fw * arg->i, 0));
	rect_t vr  = view_rect(s->view);

	if (rect_intersects(&sel, &vr))
		s->selection = sel;
}

static void kb_px_select_frame(struct session *s, const union arg *arg)
{
	rect_t sel = s->selection;
	rect_t vr  = view_rect(s->view);

	sel.x2 += s->view->fw * arg->i;

	sel = rect_norm(sel);

	if (sel.x2 > vr.x2)
		sel.x2 = vr.x2;
	if (sel.x1 < 0)
		sel.x1 = 0;

	/* Don't allow zero-width selections. */
	if (sel.x1 == sel.x2)
		return;

	s->selection = sel;
}

static void kb_onion(struct session *s, const union arg *arg)
{
	s->onion = !s->onion;
}

static void kb_pan(struct session *s, const union arg *arg)
{
	if (arg->b) {
		session_tool_switch(s, TOOL_PAN);
	} else {
		session_tool_switch(s, s->tool.prev);
	}
}

static void kb_cmdmode(struct session *s, const union arg *arg)
{
	s->mode = MODE_COMMAND;
}

static void kb_pixelmode(struct session *s, const union arg *arg)
{
	s->mode = s->mode == MODE_PIXEL ? MODE_NORMAL : MODE_PIXEL;

	if (s->mode == MODE_PIXEL) {
		s->selection = view_frame_rect(s->view, 0);
	} else {
		s->selection = rect(0, 0, 0, 0);
	}
}

static void kb_presentmode(struct session *s, const union arg *arg)
{
	s->mode = s->mode == MODE_PRESENT ? MODE_NORMAL : MODE_PRESENT;
}

static void kb_help(struct session *s, const union arg *arg)
{
	s->help = arg->b;
}

static void kb_create_frame(struct session *s, const union arg *arg)
{
	view_addframe(s->view, s->ctx);
}

static void kb_create_view(struct session *s, const union arg *arg)
{
}

static void kb_undo(struct session *s, const union arg *_a)
{
	view_undo(s->ctx, s->view);
}

static void kb_redo(struct session *s, const union arg *_a)
{
	view_redo(s->ctx, s->view);
}

static void kb_toggle_checker(struct session *s, const union arg *a)
{
	s->checker.active = !s->checker.active;
}

static void kb_swap_colors(struct session *s, const union arg *a)
{
	rgba_t tmp  = s->fg;
	s->fg = s->bg;
	s->bg = tmp;
}

static void kb_brush(struct session *s, const union arg *_a)
{
	session_tool_switch(s, TOOL_BRUSH);
	session_brush_paint(s);
}

static void kb_eraser(struct session *s, const union arg *a)
{
	session_tool_switch(s, TOOL_BRUSH);
	session_brush_erase(s);
}

static void kb_brush_size(struct session *s, const union arg *arg)
{
	s->tool.brush.size += arg->i;

	/* Snap to sizes divisible by two */
	if (arg->i > 0)
		s->tool.brush.size += s->tool.brush.size % 2;
	else
		s->tool.brush.size -= s->tool.brush.size % 2;

	if (s->tool.brush.size < 1)
		s->tool.brush.size = 1;

	polygon_release(&s->tool.brush.quad);
	s->tool.brush.quad = brush_quad(s->tool.brush.size);
}

static void kb_zoom(struct session *s, const union arg *arg)
{
	static int zooms[] = {1, 2, 3, 4, 6, 8, 10, 12, 16, 20, 24, 32, 64, 128};

	unsigned i;
	int z = arg->i;

	if (z > 0) {
		i = 0;

		while (i < elems(zooms)) {
			if (s->zoom < zooms[i])
				break;
			i ++;
		}
		if (s->zoom < 128) {
			session_zoom(s, zooms[i]);
		}

	} else if (z < 0) {
		i = elems(zooms) - 1;

		while (i > 0) {
			if (s->zoom > zooms[i])
				break;
			i --;
		}
		if (s->zoom > 1) {
			session_zoom(s, zooms[i]);
		}
	}
}

static void kb_flipx(struct session *s, const union arg *arg)
{
	s->view->flipx = arg->b;

	if (! arg->b) /* Release both flips when we release the key. */
		s->view->flipy = false;
}

static void kb_flipy(struct session *s, const union arg *arg)
{
	s->view->flipy = arg->b;

	if (! arg->b) /* Release both flips when we release the key. */
		s->view->flipx = false;
}

static void kb_adjust_fps(struct session *s, const union arg *arg)
{
	s->fps += arg->i;

	if (s->fps < 1)
		s->fps = 1;
}

static void kb_pause(struct session *s, const union arg *arg)
{
	s->paused = !s->paused;
}

/** CMDLINE *******************************************************************/

static bool command(struct session *, char *);

static void input_putc(struct input *in, unsigned int scancode)
{
	char c = (char)scancode;

	if (in->cursor + 1 >= sizeof(in->field))
		return;

	in->field[in->cursor ++] = c;
	in->field[in->cursor]    = '\0';
}

static void input_puts(struct input *in, char *str)
{
	size_t len = min(strlen(str), sizeof(in->field) - in->cursor);
	memcpy(&in->field[in->cursor], str, len);
	in->cursor += len;
}

static void input_delc(struct input *in)
{
	in->field[-- in->cursor] = '\0';
}

static struct input *input(void)
{
	return calloc(1, sizeof(struct input));
}

static void input_clear(struct input *in)
{
	memset(in->field, 0, in->cursor + 1);
	in->cursor = 0;
}

static struct cmdline cmdline(void)
{
	return (struct cmdline){
		.in        = input()
	};
}

static void cmdline_hide(struct session *s, struct cmdline *cmd)
{
	s->mode = MODE_NORMAL;
	s->selection = rect(0, 0, 0, 0);
	input_clear(cmd->in);
}

static void cmdline_handle_backspace(struct session *s, struct cmdline *cmd)
{
	input_delc(cmd->in);

	if (cmd->in->cursor == 0) {
		cmdline_hide(s, cmd);
	}
}

static void cmdline_handle_enter(struct session *s, struct cmdline *cmd)
{
	command(s, cmd->in->field + 1); /* Skip ':' */
	cmdline_hide(s, cmd);
}

static void cmdline_handle_input(struct session *s, struct cmdline *cmd, unsigned int scancode)
{
	input_putc(cmd->in, scancode);
	message_clear(s);
}

/** SESSION *******************************************************************/

static void view_draw_brush(struct view *, struct brush *, int, int);
static void palette_draw(struct palette *, struct context *);
static void draw_current_colors(struct context *, rgba_t, rgba_t);
static bool session_view_quit(struct session *, struct view *, bool);
static struct point session_view_coords(struct session *, struct view *, int, int);
static struct point snap(struct session *, struct point, int, int);

static struct checker checker(bool active)
{
	return (struct checker){
		.tex    = texture_load("resources/alpha.tga", GL_RGBA),
		.active = active,
	};
}

static void session_init(struct session *s, struct context *ctx)
{
	s->w          = ctx->width;
	s->h          = ctx->height;
	s->x          = 0;
	s->y          = 0;
	s->mx         = (int)ctx->cursorx;
	s->my         = (int)ctx->cursory;
	s->mousedown  = false;
	s->mselection = rect(0, 0, 0, 0);
	s->zoom       = 1;
	s->paused     = true;
	s->help       = false;
	s->onion      = false;
	s->fg         = WHITE;
	s->bg         = BLACK;
	s->started    = ctx_time(ctx);
	s->fps        = 6;
	s->views      = NULL;
	s->view       = NULL;
	s->ctx        = ctx;
	s->cmdline    = cmdline();
	s->checker    = checker(false);
	s->gridw      = 0;
	s->gridh      = 0;
	s->mode       = MODE_NORMAL;
	s->paste      = NULL;
	s->selection  = rect(0, 0, 0, 0);
	s->macro      = NULL;
	s->macros     = NULL;
	s->play       = NULL;
	s->recording  = false;
	s->recopts    = 0;

	*s->message   = '\0';

	s->tool.prev            = TOOL_BRUSH;
	s->tool.curr            = TOOL_BRUSH;
	s->tool.brush.size      = +1;
	s->tool.brush.prev.x    = -1;
	s->tool.brush.prev.y    = -1;
	s->tool.brush.curr.x    = -1;
	s->tool.brush.curr.y    = -1;
	s->tool.brush.drawing   = DRAW_NONE;
	s->tool.brush.quad      = brush_quad(1);
	s->tool.brush.sblend    = GL_SRC_ALPHA;
	s->tool.brush.dblend    = GL_ONE_MINUS_SRC_ALPHA;
	s->tool.brush.erase     = false;
	s->tool.brush.multi     = false;
}

static struct macro *session_macro_alloc(struct session *s)
{
	if (! s->macros) {
		s->macros = s->macro = calloc(1, sizeof(*s->macro));
	} else {
		s->macro->next = calloc(1, sizeof(*s->macro));
		s->macro = s->macro->next;
	}
	return s->macro;
}

static void session_macro_record(struct session *s, const char *fmt, ...)
{
	if (! s->recording)
		return;

	char         *cmd;
	struct macro *m;

	{
		va_list ap;
		va_start(ap, fmt);
		vasprintf(&cmd, fmt, ap);
		va_end(ap);
	}

	if (! (s->recopts & REC_TEST)) { /* Handle time delay between macro steps */
		struct timeval now;
		gettimeofday(&now, NULL);

		int      frame_msec = 1000 / s->ctx->vidmode->refreshRate;
		long     delta_usec = now.tv_usec - s->macro_tv.tv_usec;
		time_t   delta_sec  = now.tv_sec - s->macro_tv.tv_sec;
		long     delta_msec = delta_sec * 1000 + delta_usec / 1000 - frame_msec;

		if (delta_msec > 0 && s->macros) {
			m = session_macro_alloc(s);
			asprintf(&m->cmd, "sleep %ld", delta_msec);
		}
		s->macro_tv.tv_sec  = now.tv_sec;
		s->macro_tv.tv_usec = now.tv_usec;
	}

	m      = session_macro_alloc(s);
	m->cmd = cmd;
}

static void session_macro_play(struct session *s)
{
	if (! s->play)
		return;

	/* Keep track of the value of `cmd` across function calls, so that
	 * we can always play the _previous_ command, while displaying the
	 * _current_. This ensures that we always have a graphics tick
	 * in between, so that commands are shown before they are run. */
	static char cmd[128] = {0};

	if (strlen(cmd))
		command(s, cmd);

	if (fgetstr(cmd, sizeof(cmd), s->play)) {
		if (strlen(cmd) > 0 && cmd[0] != ';') {
			message(MSG_REPLAY, cmd);
		}
	} else {
		fclose(s->play);
		s->play = NULL;
		cmd[0] = '\0';
	}
}

static unsigned long session_digest(struct session *s)
{
	int w = s->ctx->winw;
	int h = s->ctx->winh;

	/* Height of the top of the status bar */
	int statush = 15 + (int)s->ctx->font->gh * 2;

	size_t len = sizeof(rgba_t) * w * (h - statush);

	/* We use `malloc` because the data might not fit on the stack. */
	rgba_t *buf = malloc(len);
	framebuffer_read(s->ctx->screen, rect(0, statush, w, h), buf);

	unsigned long digest = hash((const char *)buf, len);
	free(buf);

	return digest;
}

static int session_finish_recording(struct session *s, const char *recpath)
{
	struct macro      *m = s->macros;
	struct macro      *prev;

	FILE              *fp;
	int                steps = 0;

	if (! recpath)
		recpath = "/dev/null";

	if (! (fp = fopen(recpath, "w+")))
		fatal("record", "Couldn't create recording file");

	while (m) {
		fprintf(fp, "%s\n", m->cmd);

		free(m->cmd);
		prev = m;
		m = m->next;
		free(prev);

		steps ++;
	}
	fclose(fp);

	s->recording = false;

	return steps;
}

static void session_start_recording(struct session *s, unsigned opts)
{
	/* Start the macro timer */
	gettimeofday(&s->macro_tv, NULL);
	s->recording = true;
	s->recopts   = opts;
}

static void session_tool_switch(struct session *s, enum tooltype t)
{
	if (s->tool.curr == t)
		return;

	s->tool.prev = s->tool.curr;
	s->tool.curr = t;
}

static void session_zoom(struct session *s, int zoom)
{
	double px           = s->mx - s->x;
	double py           = s->my - s->y;
	double zprev        = s->zoom;

	s->zoom = zoom;

	if (view_within(s->view, s->mx, s->my, (int)zprev)) {
		double zdiff = (double)s->zoom / zprev;

		int nx = (int)floor(px * zdiff);
		int ny = (int)floor(py * zdiff);

		s->x = s->mx - nx;
		s->y = s->my - ny;

		double vx = s->view->x;
		double vy = s->view->y;

		views_refresh(s->views, s->zoom);

		int dx = s->view->x - (int)floor(vx * zdiff);
		int dy = s->view->y - (int)floor(vy * zdiff);

		s->x -= dx;
		s->y -= dy;
	} else {
		views_refresh(s->views, s->zoom);
		session_view_center(s, s->view);
	}
}

static void session_pick_color(struct session *s, rgba_t color)
{
	if (! color.a) {
		/* If picking a fully transparent color, switch to eraser. */
		s->tool.prev = TOOL_BRUSH;
		session_brush_erase(s);
		return;
	}

	if (
		color.r != s->fg.r ||
		color.g != s->fg.g ||
		color.b != s->fg.b ||
		color.a != s->fg.a
	) {
		s->bg = s->fg;
		s->fg = color;
	}

	/* Pick color and set previous tool to brush so that when
	 * we are done we automatically switch to the paint brush. */
	s->tool.prev = TOOL_BRUSH;
	session_brush_paint(s);
}

static rgba_t session_color_at(struct session *s, int x, int y)
{
	struct view *v = s->views;
	while (v) {
		if (v->hover) {
			struct point p = session_view_coords(s, v, x, y);
			return framebuffer_sample(v->fb, p.x, p.y);
		}
		v = v->next;
	}
	return framebuffer_sample(s->ctx->screen, x, y);
}

static void session_pick_color_at(struct session *s, int x, int y)
{
	session_pick_color(s, session_color_at(s, x, y));
}

static void session_brush_paint(struct session *s)
{
	s->tool.brush.erase     = false;
	s->tool.brush.sblend    = GL_SRC_ALPHA;
	s->tool.brush.dblend    = GL_ONE_MINUS_SRC_ALPHA;
}

static void session_brush_erase(struct session *s)
{
	s->tool.brush.erase     = true;
	s->tool.brush.sblend    = GL_ONE;
	s->tool.brush.dblend    = GL_ZERO;
}

static void session_view_vcenter(struct session *s, struct view *v)
{
	s->y = s->ctx->height/2 - vh(s->view)/2 * s->zoom - v->y;
}

static void session_view_hcenter(struct session *s, struct view *v)
{
	s->x = s->ctx->width/2 - (vw(s->view) * s->zoom)/2 - v->x;
}

static void session_view_center(struct session *s, struct view *v)
{
	session_view_vcenter(s, v);
	session_view_hcenter(s, v);
}

static void session_view_resize(struct session *s, struct view *v, int fw, int fh)
{
	view_resize(v, fw, fh, s->ctx);
	views_refresh(s->views, s->zoom);
}

static struct point session_view_coords(struct session *s, struct view *v, int x, int y)
{
	int vx = (x - v->x - s->x) / s->zoom;
	int vy = (y - v->y - s->y) / s->zoom;

	if (s->mode != MODE_PIXEL && s->tool.curr == TOOL_BRUSH) {
		/* Coords should be bottom left corner of brush */
		vx -= s->tool.brush.size / 2;
		vy -= s->tool.brush.size / 2;
	}

	if (v->flipx)
		vx = vw(v) - vx;
	if (v->flipy)
		vy = vh(v) - vy;

	return (struct point){vx, vy};
}

static void session_add_view(struct session *s, struct view *v)
{
	if (s->views) {
		struct view *u = s->view;
		assert(u);

		v->x = u->x;

		if (u->next) {
			v->next = u->next;
			v->next->prev = v;
		}
		v->prev = u;
		u->next = v;

		/* Recompute the spacing between the views */
		views_refresh(s->views, s->zoom);
	} else {
		s->views = s->view = v;
		session_view_center(s, v);
	}
}

static void session_view_blank(struct session *s, char *filename, enum filestatus fs, int w, int h)
{
	struct view *v = view(s->ctx, filename, fs, w, h, NULL, 0, 0);
	session_add_view(s, v);

	framebuffer_bind(v->fb);
	framebuffer_clear();
}

static void session_edit_view(struct session *s, struct view *v)
{
	s->view = v;
	session_view_vcenter(s, v);
}

static bool session_view_load(struct session *s, char *path)
{
	struct tga  t;

	if (! tga_load(&t, path)) {
		if (errno == ENOENT) {
			return false;
		} else {
			errno = -1;
			message(MSG_ERR, "Error: couldn't load image \"%s\"", path);
			return false;
		}
	}

	struct view *vprev = s->view;
	struct view *v = view(
		s->ctx, path, FILE_SAVED, t.width, t.height, (uint8_t *)t.data, 0, 0
	);
	/* If the previous view was a dummy view, close it now that we have
	 * something interesting loaded. */
	if (vprev && vprev->filestatus == FILE_NONE)
		session_view_quit(s, vprev, false);

	session_add_view(s, v);
	session_edit_view(s, v);

	message(MSG_INFO, "\"%s\" %d pixels read", path, t.width * t.height);

	tga_release(&t);

	return true;
}

static bool session_view_quit(struct session *s, struct view *v, bool exit)
{
	for (struct view *vp = s->views; vp; vp = vp->next) {
		if (vp == v) {
			if (v->prev) {
				s->view = v->prev;
				v->prev->next = v->next;
				if (v->next)
					v->next->prev = v->prev;
			} else {
				s->views = s->view = v->next;
				if (v->next)
					v->next->prev = NULL;
			}
			view_free(v);

			if (s->views) {
				views_refresh(s->views, s->zoom);
				session_view_vcenter(s, s->view);
			} else if (exit) {
				ctx_closewindow(s->ctx);
			}
			return true;
		}
	}
	return false;
}

static void session_palette_center(struct session *s, struct palette *p)
{
	int n = min(p->ncolors, 16);

	p->x = 0;
	p->y = s->ctx->height/2 - n * p->cellsize/2;
}

static void session_draw_statusbar(struct session *s)
{
	if (s->mode == MODE_PRESENT)
		return;

	char buffer[256];

	int mx = s->mx,
	    my = s->my;

	rgba_t cc = session_color_at(s, mx, my);

	const char *paused = s->paused               ? "p"            : "";
	const char *onion  = s->onion                ? "o"            : "";

	char coords[32];

	if (s->mode == MODE_PIXEL) {
		snprintf(coords, sizeof(coords), "%4d,%d;%d,%-4d",
			(int)s->selection.x1,
			(int)s->selection.y1,
			(int)s->selection.x2,
			(int)s->selection.y2);
	} else {
		struct point p = session_view_coords(s, s->view, mx, my);
		snprintf(coords, sizeof(coords), "%4d,%-4d", p.x, p.y);
	}
	framebuffer_bind(s->ctx->screen);

	/* Session information */
	ui_drawtext(s->ctx, NULL, s->w - 36 * s->ctx->font->gw - 10, 10 + s->ctx->font->gh + 5, RGBA_GREY,
		"%1s %1s  #%.2x%.2x%.2x %16s %5d%%",
		paused, onion, cc.r, cc.g, cc.b, coords, s->zoom * 100);

	view_fileinfo(s->view, sizeof(buffer), buffer);

	float offx = 0;
	ctx_save(s->ctx);
	ctx_translate(s->ctx, 10, 15 + s->ctx->font->gh);
	ui_drawtext(s->ctx, &offx, 0, 0, RGBA_GREY, buffer);

	if (s->recording || s->play) {
		assert(! (s->recording && s->play));

		float  margin = s->view->filestatus == FILE_NONE ? 0 : s->ctx->font->gw * 2;
		rgba_t color  = s->recording ? RGBA_RED : RGBA_GREY;
		char  *text   = s->recording ? "* recording" : "> playing";

		ui_drawtext(s->ctx, &offx, margin, 0, color, text);
	}
	ctx_restore(s->ctx);
	draw_current_colors(s->ctx, s->fg, s->bg);
}

static void session_draw_cmdline(struct session *s)
{
	/* Command line */
	if (s->mode == MODE_COMMAND) {
		ui_drawtext(s->ctx, NULL, 10, 10, RGBA_WHITE, "%s", s->cmdline.in->field);
	} else {
		ui_drawtext(s->ctx, NULL, 10, 10, s->messagecolor, "%s", s->message);
	}
}

static void session_draw_views(struct session *s)
{
	struct context *ctx = s->ctx;

	int mx = s->mx,
	    my = s->my;

	int zoom = s->zoom;

	ctx_save(ctx);
	ctx_translate(ctx, s->x, s->y);

	for (struct view *v = s->views; v; v = v->next) {
		ctx_save(ctx);
		ctx_translate(ctx, v->x, v->y);

		ctx_save(ctx);
		ctx_scale(ctx, zoom, zoom);
		view_draw(ctx, v, zoom, mx, my);
		ctx_restore(ctx);

		/* View information */
		ui_drawtext(ctx, NULL, 0, -ctx->font->gh - 5, RGBA_GREY,
			"%dx%dx%d", v->fw, v->fh, v->nframes);

		if (v->nframes > 1 && !s->paused) {
			ui_drawtext(ctx, NULL, 90, -ctx->font->gh - 5, RGBA_GREY, "@ %dHz", s->fps);
		}

		ctx_restore(ctx);
	}


	if (s->mode == MODE_PIXEL || !rect_isempty(s->selection)) {
		ctx_save(ctx);
		ctx_translate(ctx, s->view->x, s->view->y);

		int x1 = (int)s->selection.x1 * zoom;
		int y1 = (int)s->selection.y1 * zoom;
		int x2 = (int)s->selection.x2 * zoom;
		int y2 = (int)s->selection.y2 * zoom;

		ui_drawtext(ctx, NULL, x2 + 5, y2 + 5, rgba(128, 0, 0, 255),
			"%dx%d", rect_w(&s->selection), rect_h(&s->selection));

		ctx_blend(ctx, vec4(0, 0, 0, 0), GL_ONE, GL_ONE);
		fill_rect(
			ctx,
			x1, y1,
			x2, y2,
			rgba(160, 0, 0, 128)
		);
		draw_boundary(
			rgba(255, 0, 0, 128),
			min(x1, x2) - 1, min(y1, y2) - 1,
			max(x1, x2) + 1, max(y1, y2) + 1
		);
		ctx_blend_alpha(ctx);

		if (zoom >= 6) {
			ctx_save(ctx);
			ctx_translate(ctx, rect_min(s->selection).x * zoom,  rect_min(s->selection).y * zoom);
			draw_grid(rgba(190, 0, 0, 32), zoom, zoom,
				rect_w(&s->selection) * zoom,
				rect_h(&s->selection) * zoom
			);
			ctx_restore(ctx);
		}
		ctx_restore(ctx);
	} else if (s->tool.curr == TOOL_BRUSH && s->mode != MODE_PRESENT) { /* Brush */
		ctx_save(ctx);
		ctx_translate(ctx, -s->x, -s->y);
		view_draw_brush(s->view, &s->tool.brush, (int)floor(mx), (int)floor(my));
		ctx_restore(ctx);
	} else if (s->tool.curr == TOOL_SAMPLER) {
		/* If not hovering over the palette, and zoom level is sufficient. */
		if (zoom >= 8 && s->palette->hover == -1) {
			struct point n = snap(s, point(mx, my), s->view->x, s->view->y);

			ctx_save(ctx);
			ctx_translate(ctx, -s->x, -s->y);
			draw_boundary(GREY, n.x - 1, n.y - 1, n.x + zoom + 1, n.y + zoom + 1);
			ctx_restore(ctx);
		}
	}

	/* Grid */
	ctx_save(ctx);
	ctx_translate(ctx, s->view->x, s->view->y);
	view_draw_grid(s->view, s->gridw, s->gridh, GRID_COLOR, zoom);
	ctx_restore(ctx);

	/* View boundaries */
	for (struct view *v = s->views; v; v = v->next)
		view_draw_boundaries(v);

	ctx_restore(ctx);
}

static void session_draw_palette(struct session *s)
{
	/* Render palette */
	ctx_save(s->ctx);
	ctx_translate(s->ctx, s->palette->x, s->palette->y);
	palette_draw(s->palette, s->ctx);
	ctx_restore(s->ctx);
}


static struct point snap(struct session *s, struct point p, int x, int y)
{
	return (struct point){
		.x = p.x - ((p.x - x - s->x) % s->zoom),
		.y = p.y - ((p.y - y - s->y) % s->zoom)
	};
}

static void view_draw_brush_cursor(struct view *v, struct brush *b, struct point n, rgba_t c)
{
	if (b->erase) {
		int    s     = b->size;
		int    z     = session->zoom;

		draw_boundary(WHITE,
			n.x          - s/2 * z,
			n.y          - s/2 * z,
			n.x  + s * z - s/2 * z,
			n.y  + s * z - s/2 * z
		);
	} else {
		vec4_t color   = rgba2vec4(c);
		int    s       = (b->size / 2) * session->zoom;

		ctx_scale(session->ctx, session->zoom, session->zoom);
		ctx_translation(session->ctx, n.x - s, n.y - s);
		ctx_program(session->ctx, "constant");

		ctx_blend(session->ctx, vec4(0, 0, 0, 0), b->sblend, b->dblend);
		set_uniform_vec4(session->ctx->program, "color", &color);
		polygon_draw(session->ctx, &b->quad);
		ctx_blend_alpha(session->ctx);

		ctx_identity(session->ctx);
	}
}

static void view_draw_brush_cursor_disabled(struct view *v, struct brush *b, struct point n, rgba_t fg)
{
	int    s       = b->size;
	int    z       = session->zoom;

	rgba_t color = b->erase ? GREY : fg;

	draw_boundary(color,
		n.x          - s * z/2,
		n.y          - s * z/2,
		n.x  + s * z - s * z/2,
		n.y  + s * z - s * z/2
	);
}

static void draw_tool_icon(struct context *ctx, struct tools *ts, enum tooltype t, int sx, int sy)
{
	rect_t r = ts->icons[t].rect;
	int ax   = ts->icons[t].ax,
	    ay   = ts->icons[t].ay;

	GLenum sfactor = ts->icons[t].sfactor,
	       dfactor = ts->icons[t].dfactor;

	rgba_t sample     = framebuffer_sample(ctx->screen, sx, sy);
	float  brightness = (float)(sample.r + sample.g + sample.b) / 3.f;
	vec4_t blendcolor = brightness >= 128 ? vec4(0, 0, 0, 255) : vec4(255, 255, 255, 255);

	ctx_blend(ctx, blendcolor, sfactor, dfactor);
	ctx_texture_draw_rect(ctx, ts->texture, (int)r.x, (int)r.y, (int)r.w, (int)r.h, sx + ax, sy + ay);
	ctx_blend_alpha(ctx);
}

static void session_draw_cursor(struct session *s, int x, int y, enum tooltype t)
{
	if (s->mode == MODE_PRESENT)
		return;

	if (s->mode == MODE_PIXEL) {
		draw_tool_icon(s->ctx, &s->tools, TOOL_BRUSH, x, y);
		return;
	}

	switch (t) {
	case TOOL_BRUSH: {
		struct brush *b = &s->tool.brush;

		if (! b->erase)
			draw_tool_icon(s->ctx, &s->tools, TOOL_BRUSH, x, y);
		break;
	}
	case TOOL_SAMPLER:
		/* Draw the icon with a one pixel offset to avoid sampling
		 * the icon itself. */
		draw_tool_icon(s->ctx, &s->tools, TOOL_SAMPLER, x + 1, y + 1);
		break;
	case TOOL_PAN:
		// TODO
		break;
	}
}

static void view_draw_brush(struct view *v, struct brush *b, int x, int y)
{
	int z = session->zoom;

	struct point n  = {x, y};

	/* Buttery-smooth cursor movement outside of the view, pixel-aligned
	 * movement inside of the view. */
	if (view_within(v, x, y, z)) {
		n = snap(session, point(x, y), v->x, v->y);

		if (b->multi) {
			for (int i = 0; i < v->nframes - view_frame_at(v, n.x, n.y); i++) {
				struct point p = {
					n.x + i * v->fw * z,
					n.y
				};
				view_draw_brush_cursor(v, b, p, session->fg);
			}
		} else {
			view_draw_brush_cursor(v, b, n, session->fg);
		}
	} else {
		view_draw_brush_cursor_disabled(v, b, n, session->fg);
	}
}

static void brush_paint(struct context *ctx, struct brush *b, rgba_t fg, int x0, int y0, int x1, int y1)
{
	vec4_t   color = rgba2vec4(fg);

	ctx_program(ctx, "constant");

	if (b->erase) {
		color = rgba2vec4(TRANSPARENT);
	}
	set_uniform_vec4(ctx->program, "color", &color);

	ctx_save(ctx);
	ctx_blend(ctx, vec4(0, 0, 0, 0), b->sblend, b->dblend);

	if (b->drawing > DRAW_STARTED) {
		int dx  = abs(x1 - x0);
		int dy  = abs(y1 - y0);
		int sx  = x0 < x1 ? 1 : -1;
		int sy  = y0 < y1 ? 1 : -1;
		int err = (dx > dy ? dx : -dy) / 2, err2;

		for (;;) {
			ctx_translation(ctx, x0, y0);
			polygon_draw(ctx, &b->quad);

			if (x0 == x1 && y0 == y1) break;

			err2 = err;

			if (err2 > -dx) { err -= dy; x0 += sx; }
			if (err2 <  dy) { err += dx; y0 += sy; }
		}
	} else {
		ctx_translation(ctx, x0, y0);
		polygon_draw(ctx, &b->quad);
	}
	ctx_restore(ctx);
}

static void brush_tick(struct context *ctx, struct view *s, struct brush *b, rgba_t color, int mx, int my)
{
	struct point p = session_view_coords(ctx->extra, s, mx, my);

	if (b->drawing == DRAW_STARTED) {
		b->prev.x = p.x;
		b->prev.y = p.y;
	} else {
		b->prev = b->curr;
	}
	b->curr.x = p.x;
	b->curr.y = p.y;

	framebuffer_bind(s->fb);
	ctx_identity(ctx);

	int x1 = b->prev.x;
	int y1 = b->prev.y;
	int x2 = b->curr.x;
	int y2 = b->curr.y;

	if (b->multi) {
		for (int i = 0; i < s->nframes - view_frame_at(s, mx, my); i++) {
			brush_paint(ctx, b, color, x1 + i * s->fw, y1, x2 + i * s->fw, y2);
		}
	} else {
		brush_paint(ctx, b, color, x1, y1, x2, y2);
	}
	framebuffer_bind(ctx->screen);
}

static void brush_start_drawing(struct context *ctx, struct view *s, struct brush *b, rgba_t color, int x, int y)
{
	if (! view_within(s, x, y, session->zoom))
		return;

	b->drawing = DRAW_STARTED;
	brush_tick(ctx, s, b, color, x, y);
	view_dirty(s);
}

static void brush_stop_drawing(struct context *ctx, struct view *s, struct brush *b)
{
	b->drawing = DRAW_ENDED;
	view_snapshot_save(ctx, s, false);
}

/** PALETTE *******************************************************************/

static struct palette *palette(int cellsize)
{
	struct palette *pal = malloc(sizeof(*pal));

	pal->fb = framebuffer(
		cellsize * 2,
		cellsize * (32 / 2),
		NULL
	);
	pal->ncolors  = 0;
	pal->cellsize = cellsize;
	pal->x        = 0;
	pal->y        = 0;
	pal->hover    = -1;
	memset(pal->colors, 0, sizeof(pal->colors));

	return pal;
}

static void palette_free(struct palette *p)
{
	framebuffer_free(p->fb);
	free(p);
}

static void palette_hover(struct palette *p, int x, int y)
{
	x -= p->x;
	y -= p->y;

	int width  = p->ncolors > 16 ? p->cellsize * 2 : p->cellsize;
	int height = min(p->ncolors, 16) * p->cellsize;

	if (x >= width || y >= height || x < 0 || y < 0) {
		p->hover = -1;
		return;
	}

	y /= p->cellsize;
	x /= p->cellsize;

	int index = y + x * 16;

	p->hover = index < p->ncolors ? index : -1;
}

static void palette_refresh(struct palette *p, struct context *ctx)
{
	framebuffer_bind(p->fb);
	framebuffer_clear();

	ctx_identity(ctx);

	for (int i = 0; i < p->ncolors; i++) {
		int x = i >= 16  ? p->cellsize : 0,
		    y = (i % 16) * p->cellsize;

		fill_rect(
			ctx,
			x, y,
			x + p->cellsize,
			y + p->cellsize,
			p->colors[i]
		);
	}
	framebuffer_bind(ctx->screen);
}

static void palette_draw(struct palette *p, struct context *ctx)
{
	framebuffer_draw(p->fb, ctx);

	if (p->hover >= 0) {
		int x = p->hover >= 16 ? p->cellsize : 0,
		    y = (p->hover % 16) * p->cellsize;

		draw_boundary(WHITE,
			x,
			y,
			x + p->cellsize,
			y + p->cellsize
		);
	}
}

static void palette_clear(struct palette *p)
{
	p->ncolors = 0;
}

static void palette_addcolor(struct palette *p, rgba_t color)
{
	for (int i = 0; i < p->ncolors; i ++) {
		if (! rgbacmp(p->colors[i], color)) {
			return;
		}
	}
	palette_setcolor(p, color, p->ncolors ++);
}

static void palette_setcolor(struct palette *p, rgba_t color, int idx)
{
	assert(idx <= 255);
	p->colors[idx] = color;
}

static void palette_set_colors(struct palette *p, const char *colors[], int ncolors)
{
	p->ncolors = ncolors;

	for (int i = 0; i < ncolors; i++) {
		rgba_t color = hex2rgba(colors[i]);
		palette_setcolor(p, color, i);
	}
}

static struct palette *palette_read(struct context *ctx, struct view *v)
{
	int total = vw(v) * vh(v);
	rgba_t *pixels = alloca(sizeof(rgba_t) * total);
	struct palette *p = palette(PAL_SWATCH_SIZE);

	view_readpixels(v, pixels);

	for (int i = 0; i < total; i++) {
		if (pixels[i].a > 0)
			palette_addcolor(p, pixels[i]);
	}

	return p;
}

/** CALLBACKS *****************************************************************/

static void key_callback(struct context *ctx, int key, int scancode, int action, int mods)
{
	struct session *s = ctx->extra;

	/* While the mouse is down, don't accept keyboard input. */
	if (s->mousedown)
		return;

	if (action == INPUT_REPEAT)
		action = INPUT_PRESS;

	if (key == KEY_ESCAPE && action == INPUT_PRESS && s->mode != MODE_NORMAL) {
		if (s->mode == MODE_COMMAND) {
			cmdline_hide(s, &s->cmdline);
		} else if (s->mode == MODE_PIXEL) {
			s->selection = rect(0, 0, 0, 0);
			s->mode = MODE_NORMAL;
		} else {
			s->mode = MODE_NORMAL;
		}
		return;
	}

	if (s->mode == MODE_COMMAND) {
		/* TODO: Use keybindings for this. */
		if (key == KEY_BACKSPACE && action == INPUT_PRESS) {
			cmdline_handle_backspace(s, &s->cmdline);
		} else if (key == KEY_ENTER && action == INPUT_PRESS) {
			cmdline_handle_enter(s, &s->cmdline);
		}
		return;
	}

	for (u32 i = 0; i < elems(bindings); i++) {
		if (bindings[i].mode && bindings[i].mode != s->mode)
			continue;
		if (bindings[i].key == key
			&& bindings[i].mods == mods
			&& bindings[i].action == action
			&& bindings[i].callback) {
			bindings[i].callback(s, &(bindings[i].arg));
			return;
		}
	}
	if (key == KEY_LCTRL) {
		if (action == INPUT_PRESS) {
			session_tool_switch(s, TOOL_SAMPLER);
		} else {
			session_tool_switch(s, s->tool.prev);
		}
	}
	if (key == KEY_LSHIFT && s->tool.curr == TOOL_BRUSH) {
		if (action == INPUT_PRESS) {
			s->tool.brush.multi = true;
		} else if (action == INPUT_RELEASE) {
			s->tool.brush.multi = false;
		}
	}
}

static void char_callback(struct context *ctx, unsigned int scancode)
{
	struct session *s = ctx->extra;

	if (s->mode != MODE_COMMAND)
		return;
	if (! isprint(scancode))
		return;

	cmdline_handle_input(s, &s->cmdline, scancode);
}

static void window_size_callback(struct context *ctx, int w, int h)
{
	struct session *s = ctx->extra;

	s->w = w;
	s->h = h;

	s->mx = (int)ctx->cursorx;
	s->my = (int)ctx->cursory;

	session_view_center(s, s->view);
	session_palette_center(s, s->palette);
	palette_refresh(s->palette, s->ctx);
}

static void window_pos_callback(struct context *ctx, int x, int y)
{
	struct session *s = ctx->extra;

	s->mx = (int)ctx->cursorx;
	s->my = (int)ctx->cursory;
}

static void mouse_button_callback(struct context *ctx, int button, int action, int mods)
{
	double x = ctx->cursorx,
	       y = ctx->cursory;

	struct session *s   = ctx->extra;
	struct palette *pal = s->palette;

	if (action == INPUT_PRESS) {
		s->mousedown = true;
		session_macro_record(s, "cursor/down");
	} else if (action == INPUT_RELEASE) {
		s->mousedown = false;
		session_macro_record(s, "cursor/up");
	}

	/* Click on palette */
	if (pal->hover >= 0 && action == INPUT_PRESS) {
		rgba_t color = pal->colors[pal->hover];
		char hex[8]; sprintf(hex, "#%.2x%.2x%.2x", color.r, color.g, color.b);

		if (s->mode == MODE_COMMAND) {
			input_puts(s->cmdline.in, hex);
		} else {
			session_pick_color(s, color);
		}
		return;
	}

	/* Click on active view */
	if (action == INPUT_PRESS && view_within(s->view, (int)x, (int)y, s->zoom)) {
		if (s->mode == MODE_NORMAL) {
			switch (s->tool.curr) {
			case TOOL_BRUSH:
				brush_start_drawing(ctx, s->view, &s->tool.brush, s->fg, (int)floor(x), (int)floor(y));
				break;
			case TOOL_SAMPLER:
				session_pick_color_at(s, (int)round(x) - 1, (int)round(y) - 1);
				break;
			case TOOL_PAN:
				break;
			}
		} else if (s->mode == MODE_COMMAND) {
			cmdline_hide(s, &s->cmdline);
		} else if (s->mode == MODE_PIXEL) {
			struct point p = session_view_coords(s, s->view, (int)x, (int)y);
			s->selection   = rect(p.x, p.y, p.x + 1, p.y + 1);
			s->mselection  = rect((float)x, (float)y, (float)x, (float)y);
		}
	} else if (action == INPUT_PRESS) { /* Click on inactive view to switch to it */
		if (s->tool.curr != TOOL_SAMPLER) {
			struct view *v = s->views;
			while (v) {
				if (v->hover) {
					session_edit_view(s, v);
					break;
				}
				v = v->next;
			}
		}
	} else if (action == INPUT_RELEASE) {
		if (s->tool.curr == TOOL_BRUSH) {
			brush_stop_drawing(ctx, s->view, &s->tool.brush);
		}
	}
}

static void cursor_pos_callback(struct context *ctx, double fx, double fy)
{
	int x = (int)floor(fx),
	    y = (int)floor(fy);

	struct session *sess = ctx->extra;
	struct view *v       = sess->view;

	session_macro_record(sess, "cursor/move %d %d", (int)fx, (int)fy);
	palette_hover(sess->palette, x, y);
	view_hover(sess, x, y);

	if (sess->mode == MODE_NORMAL) {
		switch (sess->tool.curr) {
		case TOOL_BRUSH:
			if (sess->tool.brush.drawing == DRAW_STARTED || sess->tool.brush.drawing == DRAW_DRAWING) {
				sess->tool.brush.drawing = DRAW_DRAWING;
				brush_tick(ctx, v, &sess->tool.brush, sess->fg, x, y);
			}
			break;
		case TOOL_SAMPLER:
			break;
		case TOOL_PAN:
			sess->x += x - sess->mx;
			sess->y += y - sess->my;
			break;
		}
	} else if (sess->mode == MODE_PIXEL) {
		/* Nb. This assumes the mode can't change while the mouse is down. */
		if (sess->mousedown) {
			sess->mselection.x2 = (float)fx;
			sess->mselection.y2 = (float)fy;

			struct point p1 = session_view_coords(
				sess, sess->view,
				(int)sess->mselection.x1,
				(int)sess->mselection.y1);
			struct point p2 = session_view_coords(
				sess, sess->view, x, y);

			rect_t s = rect_norm(rect(p1.x, p1.y, p2.x, p2.y));
			sess->selection = rect(s.x1, s.y1, s.x2 + 1, s.y2 + 1);
		}
	}
	sess->mx = x;
	sess->my = y;
}

/*** COMMANDS *****************************************************************/

static bool source_dir(struct session *, const char *);

static bool cmd_quit_forced(struct session *s, int argc, char *args[])
{
	return session_view_quit(s, s->view, true);
}

static bool cmd_quit(struct session *s, int argc, char *args[])
{
	if (s->view->filestatus == FILE_MODIFIED) {
		message(MSG_ERR, "Error: no write since last change");
		return false;
	}
	return cmd_quit_forced(s, argc, args);
}

static bool cmd_quit_all_forced(struct session *s, int argc, char *args[])
{
	struct view *v;

	while ((v = s->views)) {
		s->views = s->views->next;
		view_free(v);
	}
	ctx_closewindow(s->ctx);

	return true;
}

static bool session_edit(struct session *s, char *filepath)
{
	DIR               *dir;
	struct dirent     *ent;

	assert(filepath);

	if ((dir = opendir(filepath)) != NULL) { /* Load all files in directory */
		char path[256];

		striptrailing(filepath);

		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_name[0] == '.')
				continue;

			if (strcmp(fileext(ent->d_name), "tga") != 0)
				continue;

			if ((size_t)snprintf(path, sizeof(path), "%s/%s", filepath, ent->d_name) >= sizeof(path)) {
				message(MSG_ERR, "Error: path \"%s..\" is way too long!", path);
				closedir(dir);
				return false;
			}
			if (! session_view_load(s, path)) {
				message(MSG_ERR, "Error: couldn't open \"%s\"", path);
			}
		}
		if (source_dir(s, filepath)) {
			infof("px", "source %s/.pxrc", filepath);
		}
		closedir(dir);
	} else if (! session_view_load(s, filepath) && errno == ENOENT) {
		if (s->view) {
			session_view_blank(s, filepath, FILE_NEW, s->view->fw, s->view->fh);
		} else {
			session_view_blank(s, filepath, FILE_NEW, 64, 64);
		}
	}
	return true;
}

static bool cmd_edit(struct session *s, int argc, char *args[])
{
	if (argc > 1 && strlen(args[1]) > 0) {
		return session_edit(s, args[1]);
	} else {
		message(MSG_ERR, "Error: file name required");
		return false;
	}
}

static bool cmd_write(struct session *s, int argc, char *args[])
{
	if (argc == 1) return view_save_as(s->view, s->view->filename);
	else           return view_save_as(s->view, args[1]);
}

static bool cmd_write_quit(struct session *s, int argc, char *args[])
{
	return cmd_write(s, argc, args) && cmd_quit(s, argc, args);
}

static bool cmd_resize(struct session *s, int argc, char *args[])
{
	int w, h;
	if (sscanf(args[1], "%dx%d", &w, &h) != 2) {
		message(MSG_ERR, "Error: invalid command argument '%s'", args[1]);
		return false;
	}
	session_view_resize(s, s->view, w, h);
	session_view_center(s, s->view);
	return true;
}

static bool cmd_slice(struct session *s, int argc, char *args[])
{
	int w, h;
	if (sscanf(args[1], "%dx%d", &w, &h) != 2) {
		message(MSG_ERR, "Error: invalid command argument '%s'", args[1]);
		return false;
	}
	view_slice(s->view, w, h, s->ctx);
	return true;
}

static bool cmd_slice_all(struct session *s, int argc, char *args[])
{
	int w, h;
	if (sscanf(args[1], "%dx%d", &w, &h) != 2) {
		message(MSG_ERR, "Error: invalid command argument '%s'", args[1]);
		return false;
	}

	struct view *v = s->views;
	while (v) {
		view_slice(v, w, h, s->ctx);
		v = v->next;
	}
	return true;
}

static bool cmd_checker(struct session *s, int argc, char *args[])
{
	kb_toggle_checker(s, 0);
	return true;
}

static bool cmd_source(struct session *s, int argc, char *args[])
{
	if (! source(s, args[1])) {
		message(MSG_ERR, "Error: file \"%s\" not found", args[1]);
		return false;
	}
	return true;
}

static bool cmd_fullscreen(struct session *s, int argc, char *args[])
{
	ctx_fullscreen(s->ctx);
	return true;
}

static bool cmd_window(struct session *s, int argc, char *args[])
{
	int w, h;
	if (sscanf(args[1], "%dx%d", &w, &h) != 2) {
		message(MSG_ERR, "Error: invalid command argument '%s'", args[1]);
		return false;
	}
	ctx_windowsize(s->ctx, w, h);
	return true;
}

static bool cmd_echo(struct session *s, int argc, char *args[])
{
	if (argc > 1) {
		message(MSG_ECHO, "%s", args[1]);
	} else {
		message(MSG_ECHO, "");
	}
	return true;
}

static bool cmd_palette_add(struct session *s, int argc, char *args[])
{
	palette_addcolor(s->palette, hex2rgba(args[1]));
	palette_refresh(s->palette, s->ctx);
	session_palette_center(s, s->palette);

	return true;
}

static bool cmd_palette_clear(struct session *s, int argc, char *args[])
{
	palette_clear(s->palette);
	palette_refresh(s->palette, s->ctx);
	session_palette_center(s, s->palette);

	return true;
}

static bool cmd_palette_sample(struct session *s, int argc, char *args[])
{
	palette_free(s->palette);

	s->palette = palette_read(s->ctx, s->view);
	palette_refresh(s->palette, s->ctx);
	session_palette_center(s, s->palette);

	return true;
}

static bool cmd_grid(struct session *s, int argc, char *args[])
{
	if (argc == 1) {
		s->gridw = 0;
		s->gridh = 0;
	} else {
		int w, h;
		if (sscanf(args[1], "%dx%d", &w, &h) != 2) {
			message(MSG_ERR, "Error: invalid command argument '%s'", args[1]);
			return false;
		}
		s->gridw = w;
		s->gridh = h;
	}
	return true;
}

static bool cmd_pause(struct session *s, int argc, char *args[])
{
	s->paused = true;

	return true;
}

static bool cmd_zoom(struct session *s, int argc, char *args[])
{
	int z;
	if (sscanf(args[1], "%d", &z) != 1) {
		message(MSG_ERR, "Error: invalid command argument '%s'", args[1]);
		return false;
	}
	z /= 100;

	if (z < 1 || z > 240) {
		message(MSG_ERR, "Error: invalid zoom level");
		return false;
	}
	session_zoom(s, z);

	return true;
}

static bool cmd_center(struct session *s, int argc, char *args[])
{
	session_view_center(s, s->view);
	return true;
}

static bool cmd_fill(struct session *s, int argc, char *args[])
{
	if (args[1][0] != '#') {
		message(MSG_ERR, "Error: invalid argument: %s", args[1]);
		return false;
	}
	framebuffer_bind(s->view->fb);

	if (!rect_isempty(s->selection)) {
		fill_rect( s->ctx
			 , (int)s->selection.x1
			 , (int)s->selection.y1
			 , (int)s->selection.x2
			 , (int)s->selection.y2
			 , hex2rgba(args[1])
			 );
	} else {
		fill_rect(s->ctx, 0, 0, vw(s->view), vh(s->view), hex2rgba(args[1]));
	}
	return true;
}

static bool cmd_crop(struct session *s, int argc, char *args[])
{
	view_crop_framebuffer(s->view, s->selection, s->ctx);
	session_view_hcenter(s, s->view);
	return true;
}

static bool cmd_record(struct session *s, int argc, char *args[])
{
	char recpath[32];

	if (! timestamp(recpath, sizeof(recpath))) {
		message(MSG_ERR, "Error: couldn't create timestamp");
		return false;
	}

	if (s->recording) { /* Stop recording */
		int steps = session_finish_recording(s, recpath);
		message(MSG_INFO, "%d steps recorded to \"%s\"", steps, recpath);
	} else {
		session_start_recording(s, 0);
	}
	return true;
}

static bool cmd_play(struct session *s, int argc, char *args[])
{
	if (argc != 2) {
		message(MSG_ERR, "Error: invalid command invocation");
		return false;
	}
	FILE *stream = fopen(args[1], "r");

	if (! stream) {
		message(MSG_ERR, "Error: couldn't open \"%s\"", args[1]);
		return false;
	}

	s->play = stream;

	return true;
}

static bool cmd_cursormove(struct session *s, int argc, char *args[])
{
	if (argc != 3) {
		message(MSG_ERR, "Error: invalid command invocation");
		return false;
	}
	long x = strtol(args[1], NULL, 10);
	long y = strtol(args[2], NULL, 10);

	ctx_cursor_pos(s->ctx, x, y);

	return true;
}

static bool cmd_cursordown(struct session *s, int argc, char *args[])
{
	mouse_button_callback(s->ctx, 0, INPUT_PRESS, 0);
	return true;
}

static bool cmd_cursorup(struct session *s, int argc, char *args[])
{
	mouse_button_callback(s->ctx, 0, INPUT_RELEASE, 0);
	return true;
}

static bool cmd_sleep(struct session *s, int argc, char *args[])
{
	if (argc != 2) {
		message(MSG_ERR, "Error: invalid command invocation");
		return false;
	}
	long ms = strtoul(args[1], NULL, 10);

	struct timespec ts;
	ts.tv_sec  = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000 * 1000;

	nanosleep(&ts, NULL);

	return true;
}

static bool cmd_test_digest(struct session *s, int argc, char *args[])
{
	message(MSG_INFO, "digest = %lx", session_digest(s));
	return true;
}

static bool cmd_test_check(struct session *s, int argc, char *args[])
{
	unsigned long actual = session_digest(s);
	unsigned long expected = strtoul(args[1], NULL, 16);

	if (actual != expected) {
		message(MSG_ERR, "Test failed (%lx != %lx)", actual, expected);
		return false;
	}
	message(MSG_OK, "Test passed for %lx", expected);
	return true;
}

static bool cmd_test_record(struct session *s, int argc, char *args[])
{
	session_start_recording(s, REC_TEST);

	return true;
}

static bool cmd_test_play(struct session *s, int argc, char *args[])
{
	/* Currently, these two commands are identical */
	return cmd_play(s, argc, args);
}

static bool cmd_test_save(struct session *s, int argc, char *args[])
{
	char path[32];

	if (! timestamp(path, sizeof(path))) {
		message(MSG_ERR, "Error: couldn't create timestamp");
		return false;
	}
	session_macro_record(s, "test/check %lx", session_digest(s));

	int steps = session_finish_recording(s, path);
	message(MSG_INFO, "%d steps recorded to \"%s\"", steps, path);

	return true;
}

static bool cmd_test_discard(struct session *s, int argc, char *args[])
{
	session_finish_recording(s, "/dev/null");

	return true;
}

static bool command(struct session *s, char *str)
{
	size_t len = strlen(str);

	if (! len)
		return false;

	char *input = alloca(len + 1);
	memcpy(input, str, len + 1);

	if (!! strcmp(str, "record") && ! strprefix(str, "test/"))
		session_macro_record(s, str);

	int      argc     =  0;
	char    *argv[16] = {0}; /* 16 args max */
	char    *arg      = NULL;

	argv[argc++] = strtok(input, " ");
	while ((arg = strtok(NULL, " "))) {
		argv[argc++] = arg;
	}

	for (u32 i = 0; i < elems(commands); i ++) {
		if (! strcmp(argv[0], commands[i].name)) {
			if (argc - 1 < commands[i].argc) {
				message(MSG_ERR, "Error: %s: wrong number of arguments", argv[0]);
				return false;
			}
			return commands[i].callback(s, argc, argv);
		}
	}

	if (argv[0][0] == '#') { /* :#ff0000 */
		palette_addcolor(s->palette, hex2rgba(argv[0]));
		palette_refresh(s->palette, s->ctx);
		session_palette_center(s, s->palette);
		return true;
	}
	message(MSG_ERR, "Error: not an editor command: %s", argv[0]);
	return false;
}

static bool source(struct session *sess, const char *path)
{
	FILE *stream = fopen(path, "r");
	char  s[128];

	if (! stream)
		return false;

	while (fgetstr(s, sizeof(s), stream)) {
		if (strlen(s) == 0)
			continue;
		if (s[0] == ';')
			continue;

		command(sess, s);
	}
	fclose(stream);

	return true;
}

static bool source_dir(struct session *s, const char *dir)
{
	char     path[256];

	snprintf(path, sizeof(path), "%s/.pxrc", dir);

	return source(s, path);
}

static bool source_stdin(struct session *sess)
{
	char  s[128] = {0};

	while (fgets(s, sizeof(s), stdin)) {
		command(sess, s);
	}
	return true;
}

/******************************************************************************/

static void draw_current_colors(struct context *ctx, rgba_t fg, rgba_t bg)
{
	/* Foreground/background color */
	fill_rect(
		ctx,
		ctx->width - 360,     18 + (int)ctx->font->gh,
		ctx->width - 360 + 9, 18 + (int)ctx->font->gh + 9,
		fg
	);
	fill_rect(
		ctx,
		ctx->width - 340,     18 + (int)ctx->font->gh,
		ctx->width - 340 + 9, 18 + (int)ctx->font->gh + 9,
		bg
	);
	draw_boundary(GREY,
		ctx->width - 360,     18 + (int)ctx->font->gh,
		ctx->width - 360 + 9, 18 + (int)ctx->font->gh + 9
	);
	draw_boundary(GREY,
		ctx->width - 340,     18 + (int)ctx->font->gh,
		ctx->width - 340 + 9, 18 + (int)ctx->font->gh + 9
	);
}

static void help_show(struct context *ctx)
{
	framebuffer_bind(ctx->screen);
	framebuffer_clear();
	ctx_translation(ctx, ctx->width/2 - 160, 60);

	int y = 0;

	for (u32 i = 0; i < elems(bindings); i++) {
		const char *key = ctx_keyname(bindings[i].key);

		if (key && bindings[i].action == INPUT_PRESS) {
			if (bindings[i].mods) {
				ui_drawtext(ctx, NULL, 0, (y ++) * ctx->font->gh,
					RGBA_WHITE, "%-24s %s%s", bindings[i].name, ctx_modname(bindings[i].mods), key);
			} else {
				ui_drawtext(ctx, NULL, 0, (y ++) * ctx->font->gh,
					RGBA_WHITE, "%-24s %s", bindings[i].name, key);
			}
		}
	}
}

static void tools_init(struct tools *tools)
{
	tools->texture = texture_load("resources/tools.tga", GL_RGBA);
	tools->icons[TOOL_SAMPLER]       = (struct icon){ rect( 0,  0, 16, 16),  0 , 0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	tools->icons[TOOL_BRUSH]         = (struct icon){ rect(16, 16, 16, 16), -8, -8, GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_ALPHA };

	assert(tools->texture);
}

static bool parse_options(struct session *s, int argc, char *argv[])
{
	for (int i = 0; i < argc; i ++) {
		if (! strcmp(argv[i], "-u")) {
			if (i + 1 < argc) {
				if (! strcmp(argv[i + 1], "-")) {
					source_stdin(s);
				} else {
					source(s, argv[i + 1]);
				}
			}
			return true;
		}
	}
	return false;
}

int main(int argc, char *argv[])
{
	struct context *ctx = calloc(1, sizeof(*ctx));
	if (! ctx_init(ctx, 640, 480, true)) {
		fatal("main", "error initializing context");
	}

	info("main", "loading shader programs..");

	ctx_load_program(ctx, "text",        "shaders/text.vert",           "shaders/textured.frag");
	ctx_load_program(ctx, "texture",     "shaders/textured.vert",       "shaders/textured.frag");
	ctx_load_program(ctx, "constant",    "shaders/basic.vert",          "shaders/constant.frag");
	ctx_load_program(ctx, "framebuffer", "shaders/framebuffer.vert",    "shaders/framebuffer.frag");

	info("main", "loading font..");
	if (! load_font(ctx->font, "resources/glyphs.tga", 8, 14)) {
		ctx_destroy(ctx, "error loading font");
		return 1;
	}
	ctx->on_key    = key_callback;
	ctx->on_click  = mouse_button_callback;
	ctx->on_cursor = cursor_pos_callback;
	ctx->on_resize = window_size_callback;
	ctx->on_move   = window_pos_callback;
	ctx->on_char   = char_callback;

	ctx_cursor_hide(ctx);

	session = malloc(sizeof(*session));
	session_init(session, ctx);

	ctx->extra = session;

	if (argc > 1 && *argv[1] != '-') {
		session_edit(session, argv[1]);
	} else {
		session_view_blank(session, "", FILE_NONE, 128, 128);
	}

	assert(session->view);
	session->palette = palette(PAL_SWATCH_SIZE);
	tools_init(&session->tools);

	framebuffer_clear();
	session->fg = WHITE;

	ctx_identity(ctx);

	kb_brush(session, NULL);

	if (! parse_options(session, argc, argv)) {
		source_dir(session, getenv("HOME"));
		source_dir(session, ".");
	}

	while (ctx_loop(ctx)) {
		session_macro_play(session);

		framebuffer_bind(ctx->screen);
		framebuffer_clearcolor(0.0f, 0.0f, 0.0f, 0.f);

		ctx_identity(ctx);

		session_draw_views(session);
		session_draw_palette(session);
		session_draw_statusbar(session);
		session_draw_cmdline(session);

		if (session->help)
			help_show(ctx);

		session_draw_cursor(session, session->mx, session->my, session->tool.curr);

		ctx_present(ctx);

		if (session->paused && !session->play) {
			ctx_tick_wait(ctx);
		} else {
			ctx_tick(ctx);
		}
	}

	if (session->palette)
		palette_free(session->palette);
	if (session->paste)
		texture_free(session->paste);

	for (struct view *tmp, *v = session->views; v; ) {
		tmp = v->next;
		view_free(v);
		v = tmp;
	}

	ctx_destroy(ctx, "exiting");

#if defined(DEBUG)
	free(session->cmdline.in);
	free(session->checker.tex);
	free(session->tools.texture);
	free(session);
#endif

	return 0;
}

