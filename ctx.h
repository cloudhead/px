//
// ctx.h
// window context management
//
#include <GLFW/glfw3.h>
#include <stdbool.h>

#include "linmath.h"
#include "cursor.h"

typedef struct list *list_t;
struct               font;

struct blend {
	GLenum            sfactor, dfactor;
	vec4_t            color;
};

struct context {
	GLFWwindow               *win;
	const GLFWvidmode        *vidmode;
	struct font              *font;
	double                    lastframe;
	double                    frametime;
	int                       winw, winh;
	int                       width, height;
	double                    cursorx, cursory;
	list_t                    transforms;
	mat4_t                   *transform;
	mat4_t                    ortho;
	struct blend              blend;
	struct framebuffer       *screen;
	double                    dpi;
	bool                      hidpi;

	list_t                    programs;
	struct program           *program;

	// Input callbacks

	void                    (*on_key)     (struct context *, int, int, int, int);
	void                    (*on_click)   (struct context *, int, int, int);
	void                    (*on_cursor)  (struct context *, double, double);
	void                    (*on_focus)   (struct context *, bool);
	void                    (*on_resize)  (struct context *, int, int);
	void                    (*on_move)    (struct context *, int, int);
	void                    (*on_char)    (struct context *, unsigned int);

	void                     *extra;
};

bool               ctx_init(struct context *, int, int, bool);
void               ctx_destroy(struct context *, const char *);
void               ctx_present(struct context *);
void               ctx_tick(struct context *);
void               ctx_tick_wait(struct context *);
void               ctx_poll(struct context *);
void               ctx_closewindow(struct context *);
void               ctx_fullscreen(struct context *);
bool               ctx_windowsize(struct context *, int, int);
rect_t             ctx_windowrect(struct context *);
bool               ctx_keydown(struct context *, int);
bool               ctx_loop(struct context *);
void               ctx_prepare(struct context *);
void               ctx_load_program(struct context *, const char *, const char *, const char *);
void               ctx_program(struct context *, const char *);
void               ctx_scale(struct context *, float, float);
void               ctx_translation(struct context *, float, float);
void               ctx_translate(struct context *, float, float);
void               ctx_transform(struct context *, mat4_t *);
void               ctx_set_transform(struct context *, mat4_t *);
void               ctx_identity(struct context *);
void               ctx_cursor(struct context *, cursor_t *);
void               ctx_cursor_pos(struct context *, double, double);
void               ctx_cursor_hide(struct context *);
double             ctx_time(struct context *);
void               ctx_blend(struct context *, vec4_t, GLenum, GLenum);
void               ctx_blend_alpha(struct context *);
void               ctx_save(struct context *);
void               ctx_restore(struct context *);
void               ctx_texture_draw(struct context *, struct texture *, float, float);
void               ctx_texture_draw_rect(struct context *, struct texture *, int, int, int, int, float, float);

const char        *ctx_modname(int);
const char        *ctx_keyname(int);

#define KEY_LSHIFT       GLFW_KEY_LEFT_SHIFT
#define KEY_LCTRL        GLFW_KEY_LEFT_CONTROL
#define KEY_ENTER        GLFW_KEY_ENTER
#define KEY_BACKSPACE    GLFW_KEY_BACKSPACE
#define KEY_ESCAPE       GLFW_KEY_ESCAPE
#define KEY_SPACE        GLFW_KEY_SPACE
#define KEY_W            GLFW_KEY_W
#define KEY_A            GLFW_KEY_A
#define KEY_S            GLFW_KEY_S
#define KEY_D            GLFW_KEY_D
#define KEY_UP           GLFW_KEY_UP
#define KEY_DOWN         GLFW_KEY_DOWN
#define KEY_LEFT         GLFW_KEY_LEFT
#define KEY_RIGHT        GLFW_KEY_RIGHT
#define KEY_MINUS        GLFW_KEY_MINUS
#define KEY_EQUAL        GLFW_KEY_EQUAL
#define KEY_F1           GLFW_KEY_F1
#define KEY_F2           GLFW_KEY_F2
#define KEY_F3           GLFW_KEY_F3
#define KEY_F4           GLFW_KEY_F4
#define KEY_F5           GLFW_KEY_F5
#define KEY_F6           GLFW_KEY_F6
#define KEY_F7           GLFW_KEY_F7
#define KEY_F8           GLFW_KEY_F8
#define KEY_F9           GLFW_KEY_F9
#define KEY_1            GLFW_KEY_1
#define KEY_2            GLFW_KEY_2
#define KEY_3            GLFW_KEY_3
#define KEY_4            GLFW_KEY_4

#define INPUT_PRESS      GLFW_PRESS
#define INPUT_RELEASE    GLFW_RELEASE
#define INPUT_REPEAT     GLFW_REPEAT
#define INPUT_LMOUSE     GLFW_MOUSE_BUTTON_LEFT
#define INPUT_RMOUSE     GLFW_MOUSE_BUTTON_RIGHT
