//
// ctx.h
// window context management
//
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "linmath.h"
#include "color.h"
#include "assert.h"
#include "util.h"
#include "text.h"
#include "cursor.h"
#include "texture.h"
#include "gl.h"
#include "ctx.h"
#include "program.h"
#include "polygon.h"
#include "framebuffer.h"
#include "list.h"
#include "sprite.h"

//////////////
// INTERNAL //

static void ctx_update_cursor_pos(struct context *);

static void ctx_win_center(struct context *ctx)
{
	glfwSetWindowPos(ctx->win,
		ctx->vidmode->width/2 - ctx->winw/2,
		ctx->vidmode->height/2 - ctx->winh/2);
}

static void ctx_set_cursor_pos(struct context *ctx, double x, double y)
{
	if (ctx->hidpi) {
		x /= 2;
		y /= 2;
	}
	ctx->cursorx = x;
	ctx->cursory = ctx->height - y;
}

static void ctx_update_cursor_pos(struct context *ctx)
{
	double mx, my;

	glfwGetCursorPos(ctx->win, &mx, &my);
	ctx_set_cursor_pos(ctx, mx, my);
}

static void ctx_setup_program_unifs(struct context *ctx, struct program *p)
{
	set_uniform_mat4(p, "transform", ctx->transform);
	set_uniform_i32(p, "sampler", 0); // Set sampler uniform to texture unit 0
}

static void ctx_setup_ortho(struct context *ctx)
{
	struct program *prev = ctx->program;
	struct program *p    = NULL;
	list_t         ps    = ctx->programs;

	while (list_next(&ps, (void **)&p)) {
		program_use(p);
		set_uniform_mat4(p, "ortho",  &ctx->ortho);
	}
	program_use(prev);
}

static void ctx_setup_program(struct context *ctx, struct program *p)
{
	struct program *prev = ctx->program;

	assert(p);

	program_use(p);
	ctx_setup_program_unifs(ctx, p);
	program_use(prev);

	// XXX
	ctx_setup_ortho(ctx);
}

///////////////
// CALLBACKS //

static void error_callback(int error, const char *description)
{
	infof("glfw", "error: %s", description);
}

static void key_callback(GLFWwindow *win, int key, int scan, int action, int mods)
{
	struct context *ctx = glfwGetWindowUserPointer(win);

	if (ctx->on_key)
		ctx->on_key(ctx, key, scan, action, mods);
}

static void mouse_button_callback(GLFWwindow *win, int button, int action, int mods)
{
	struct context *ctx = glfwGetWindowUserPointer(win);

	if (ctx->on_click)
		ctx->on_click(ctx, button, action, mods);
}

static void cursor_pos_callback(GLFWwindow *win, double x, double y)
{
	struct context *ctx = glfwGetWindowUserPointer(win);
	ctx_set_cursor_pos(ctx, x, y);

	if (ctx->on_cursor)
		ctx->on_cursor(ctx, ctx->cursorx, ctx->cursory);
}

static void focus_callback(GLFWwindow *win, int focus)
{
	struct context *ctx = glfwGetWindowUserPointer(win);

	if (ctx->on_focus)
		ctx->on_focus(ctx, !! focus);
}

static void framebuffer_size_callback(GLFWwindow *win, int w, int h)
{
	struct context *ctx = glfwGetWindowUserPointer(win);

	infof("ctx", "resizing context to %dx%d", w, h);

	framebuffer_free(ctx->screen);

	ctx->width   = ctx->hidpi ? w / 2 : w;
	ctx->height  = ctx->hidpi ? h / 2 : h;
	ctx->screen  = framebuffer_screen(ctx->width, ctx->height, NULL);
	ctx->winw    = w;
	ctx->winh    = h;
	ctx->ortho   = mat4ortho(w, h);

	gl_viewport(w, h);

	ctx_setup_ortho(ctx);
	ctx_update_cursor_pos(ctx);

	if (ctx->on_resize)
		ctx->on_resize(ctx, ctx->width, ctx->height);
}

static void window_pos_callback(GLFWwindow *win, int x, int y)
{
	struct context *ctx = glfwGetWindowUserPointer(win);
	ctx_update_cursor_pos(ctx);

	if (ctx->on_move)
		ctx->on_move(ctx, x, y);
}

static void char_callback(GLFWwindow *win, unsigned int codepoint)
{
	struct context *ctx = glfwGetWindowUserPointer(win);

	if (ctx->on_char)
		ctx->on_char(ctx, codepoint);
}

static void ctx_free_programs(struct context *ctx)
{
	struct program *p = NULL;
	list_t         ps = ctx->programs;

	while ((p = list_pop(&ps)) && p) {
		program_free(p);
	}
}

/////////
// API //

bool ctx_init(struct context *ctx, int w, int h, bool debug)
{
	assert(! ctx->win);

	glfwSetErrorCallback(error_callback);

	if (! glfwInit()) {
		return false;
	}

	GLFWmonitor          *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode    *mode    = glfwGetVideoMode(monitor);

	int mw, mh;
	glfwGetMonitorPhysicalSize(monitor, &mw, &mh);

	ctx->dpi            = mode->width / (mw / 25.4);
	ctx->hidpi          = ctx->dpi > 180.0;
	ctx->vidmode        = mode;

	if (ctx->hidpi) {
		w *= 2;
		h *= 2;
	}

	glfwWindowHint(GLFW_RESIZABLE, true);
	glfwWindowHint(GLFW_SRGB_CAPABLE, true);

	// Require OpenGL 3.3 or later
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	// Only support new core functionality
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

	glfwWindowHint(GLFW_SAMPLES, 16);

	if ((ctx->win = glfwCreateWindow(w, h, "px", NULL, NULL)) == NULL) {
		return false;
	}
	glfwMakeContextCurrent(ctx->win);
	glfwSetWindowUserPointer(ctx->win, ctx);
	glfwSetInputMode(ctx->win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwSetInputMode(ctx->win, GLFW_STICKY_KEYS, true);
	glfwSetKeyCallback(ctx->win, key_callback);
	glfwSetMouseButtonCallback(ctx->win, mouse_button_callback);
	glfwSetCursorPosCallback(ctx->win, cursor_pos_callback);
	glfwSetCursorEnterCallback(ctx->win, focus_callback);
	glfwSetFramebufferSizeCallback(ctx->win, framebuffer_size_callback);
	glfwSetWindowPosCallback(ctx->win, window_pos_callback);
	glfwSetCharCallback(ctx->win, char_callback);
	glfwSetWindowAspectRatio(ctx->win, w, h);

	glfwGetFramebufferSize(ctx->win, &ctx->winw, &ctx->winh);
	ctx_win_center(ctx);
	gl_init(ctx->winw, ctx->winh, debug);

	int vw = ctx->winw, /* Virtual screen */
	    vh = ctx->winh;

	if (ctx->hidpi) {
		vw /= 2;
		vh /= 2;

		/* We can't create odd-sized framebuffer textures, so we make
		 * the screen framebuffer even in case the virtual screen isn't. */
		if (vw % 2 != 0) vw ++;
		if (vh % 2 != 0) vh ++;
	}
	infof("ctx", "real screen size: %dx%d", ctx->winw, ctx->winh);
	infof("ctx", "virtual screen size: %dx%d", vw, vh);

	ctx->lastframe      = 0;
	ctx->frametime      = 0;
	ctx->transforms     = NULL;
	ctx->ortho          = mat4ortho(ctx->winw, ctx->winh);
	ctx->font           = malloc(sizeof(*ctx->font));

	ctx->screen         = framebuffer_screen(vw, vh, NULL);
	ctx->width          = ctx->hidpi ? ctx->winw / 2 : ctx->winw;
	ctx->height         = ctx->hidpi ? ctx->winh / 2 : ctx->winh;

	ctx_update_cursor_pos(ctx);

	ctx_blend_alpha(ctx);
	ctx_save(ctx);

	glfwSetTime(0);

	infof("ctx", "dpi = %f", ctx->dpi);

	return true;
}

void ctx_blend_alpha(struct context *ctx)
{
	ctx->blend.color    = vec4(0, 0, 0, 0);
	ctx->blend.sfactor  = GL_SRC_ALPHA;
	ctx->blend.dfactor  = GL_ONE_MINUS_SRC_ALPHA;
}

void ctx_blend
	( struct context *ctx
	, vec4_t color
	, GLenum sfactor
	, GLenum dfactor
	)
{
	ctx->blend.color   = color;
	ctx->blend.sfactor = sfactor;
	ctx->blend.dfactor = dfactor;
}

void ctx_save(struct context *ctx)
{
	mat4_t *m = malloc(sizeof(*m));

	if (ctx->transforms == NULL) {
		*m = mat4identity();
	} else {
		*m = *(mat4_t *)ctx->transforms->head;
	}
	list_push(&ctx->transforms, m);
	ctx->transform = m;
}

void ctx_restore(struct context *ctx)
{
	if (list_length(ctx->transforms) > 1) {
		free(list_pop(&ctx->transforms));
		ctx->transform = ctx->transforms->head;
	}
}

void ctx_destroy(struct context *ctx, const char *reason)
{
	glfwDestroyWindow(ctx->win);
	glfwTerminate();

	infof("ctx", "terminating (reason: %s)", reason);

	ctx_free_programs(ctx);
	font_free(ctx->font);
	framebuffer_free(ctx->screen);
	list_consume(&ctx->transforms, free);

	free(ctx);
}

void ctx_scale(struct context *ctx, float x, float y)
{
	mat4transform(ctx->transform, 0, 0, x, y);

	if (ctx->program)
		set_uniform_mat4(ctx->program, "transform", ctx->transform);
}

void ctx_translation(struct context *ctx, float x, float y)
{
	*ctx->transform = mat4translate(*ctx->transform, vec3(x, y, 1.f));

	if (ctx->program)
		set_uniform_mat4(ctx->program, "transform", ctx->transform);
}

void ctx_translate(struct context *ctx, float x, float y)
{
	mat4transform(ctx->transform, x, y, 1, 1);

	if (ctx->program)
		set_uniform_mat4(ctx->program, "transform", ctx->transform);
}

void ctx_transform(struct context *ctx, mat4_t *tr)
{
	*ctx->transform = mat4mul(*ctx->transform, *tr);

	if (ctx->program)
		set_uniform_mat4(ctx->program, "transform", ctx->transform);
}

void ctx_set_transform(struct context *ctx, mat4_t *tr)
{
	*ctx->transform = *tr;

	if (ctx->program)
		set_uniform_mat4(ctx->program, "transform", ctx->transform);
}

void ctx_identity(struct context *ctx)
{
	*ctx->transform = mat4identity();

	if (ctx->program)
		set_uniform_mat4(ctx->program, "transform", ctx->transform);
}

void ctx_load_program(struct context *ctx, const char *name, const char *vertpath, const char *fragpath)
{
	struct program *p = program_load(name, vertpath, fragpath);

	if (! p) fatalf("ctx", "failed to load program '%s'", name);

	ctx_setup_program(ctx, p);
	list_push(&ctx->programs, p);
}

void ctx_program(struct context *ctx, const char *name)
{
	struct program *p = NULL;
	list_t         ps = ctx->programs;

	if (! name) {
		program_use(NULL);
		ctx->program = NULL;
		return;
	}

	while (list_next(&ps, (void **)&p)) {
		if (! strcmp(name, p->name)) {
			ctx->program = p;
			program_use(p);
			ctx_setup_program_unifs(ctx, p);
			break;
		}
	}
}

void ctx_present(struct context *ctx)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	gl_viewport(ctx->winw, ctx->winh);
	gl_clear(0.f, 0.f, 0.f, 1.f);

	if (ctx->hidpi)
		ctx_scale(ctx, 2, 2);

	framebuffer_draw(ctx->screen, ctx);
	glfwSwapBuffers(ctx->win);
}

void ctx_tick(struct context *ctx)
{
	if (glfwGetWindowAttrib(ctx->win, GLFW_FOCUSED)) {
		glfwPollEvents();
	} else {
		glfwWaitEvents();
	}
}

void ctx_poll(struct context *ctx)
{
	glfwPollEvents();
}

void ctx_tick_wait(struct context *ctx)
{
	glfwWaitEvents();
}

void ctx_closewindow(struct context *ctx)
{
	glfwSetWindowShouldClose(ctx->win, true);
}

void ctx_fullscreen(struct context *ctx)
{
	glfwSetWindowPos(ctx->win, 0, 0);
	glfwSetWindowSize(ctx->win, ctx->vidmode->width, ctx->vidmode->height);

	ctx->winw = ctx->vidmode->width;
	ctx->winh = ctx->vidmode->height;
}

bool ctx_windowsize(struct context *ctx, int w, int h)
{
	if (w < 640 || h < 480)
		return false;

	if (ctx->hidpi) {
		w *= 2;
		h *= 2;
	}

	glfwSetWindowSize(ctx->win, w, h);
	ctx->winw = w;
	ctx->winh = h;

	return true;
}

rect_t ctx_windowrect(struct context *ctx)
{
	return rect(0, 0, ctx->width, ctx->height);
}

bool ctx_keydown(struct context *ctx, int key)
{
	return glfwGetKey(ctx->win, key) == GLFW_PRESS;
}

bool ctx_loop(struct context *ctx)
{
	double t = glfwGetTime();

	ctx->frametime = (t - ctx->lastframe) * 1000.0f;
	ctx->lastframe = t;

	return ! glfwWindowShouldClose(ctx->win);
}

void ctx_cursor(struct context *ctx, cursor_t *c)
{
	glfwSetCursor(ctx->win, c);
}

void ctx_cursor_pos(struct context *ctx, double x, double y)
{
	if (ctx->hidpi) {
		x *= 2;
		y *= 2;
	}
	glfwSetCursorPos(ctx->win, x, ctx->winh - y);
	cursor_pos_callback(ctx->win, x, ctx->winh - y);
}

void ctx_cursor_hide(struct context *ctx)
{
	glfwSetInputMode(ctx->win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

double ctx_time(struct context *ctx)
{
	return glfwGetTime();
}

const char *ctx_modname(int mods)
{
	if (mods & GLFW_MOD_SHIFT) {
		return "<Shift>";
	} else if (mods & GLFW_MOD_CONTROL) {
		return "<Ctrl>";
	}
	assert(0);
	return NULL;
}

const char *ctx_keyname(int key)
{
	switch (key) {
	case GLFW_KEY_SPACE:
		return "<space>";
	case GLFW_KEY_ENTER:
		return "<return>";
	case GLFW_KEY_LEFT:
		return "<left>";
	case GLFW_KEY_RIGHT:
		return "<right>";
	case GLFW_KEY_DOWN:
		return "<down>";
	case GLFW_KEY_UP:
		return "<up>";
	}
	return glfwGetKeyName(key, 0);
}

/* TODO: Do these functions belong in this module? */
void ctx_texture_draw_rect(struct context *ctx, struct texture *t, int x, int y, int w, int h, float sx, float sy)
{
	/* TODO: Shouldn't be implement via a batch */
	struct spritebatch sb;
	spritebatch_init(&sb, t);
	spritebatch_add(&sb, rect(x, y, x + w, y + h), rect(sx, sy, sx + w, sy + h), 1, 1, vec4identity);
	spritebatch_draw(&sb, ctx);
	spritebatch_release(&sb);
}

void ctx_texture_draw(struct context *ctx, struct texture *t, float sx, float sy)
{
	ctx_texture_draw_rect(ctx, t, 0, 0, t->w, t->h, sx, sy);
}
