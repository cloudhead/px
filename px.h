//
// px.h
//

#define MAX_INPUT               256
#define MAX_FILENAME            256

#define MSG_COLOR_WHITE             RGBA_WHITE
#define MSG_COLOR_LIGHT             rgba(190, 255, 230, 255)
#define MSG_COLOR_RED               rgba(255, 50, 100, 255)
#define MSG_COLOR_GREY              rgba(255, 255, 255, 160)
#define MSG_COLOR_GREEN             rgba(90, 255, 90, 255)

enum msgtype {
	MSG_INFO         = 1,
	MSG_ECHO         = 2,
	MSG_ERR          = 3,
	MSG_REPLAY       = 4,
	MSG_OK           = 5
};

struct palette {
	struct framebuffer      *fb;
	rgba_t                   colors[256];
	int                      ncolors;
	int                      cellsize;
	int                      x, y;
	int                      hover;
};

struct point {
	int x;
	int y;
};

enum dstate {
	DRAW_NONE         = 0,
	DRAW_STARTED      = 1,
	DRAW_DRAWING      = 2,
	DRAW_ENDED        = 3
};

enum filestatus {
	FILE_NONE,
	FILE_SAVED,
	FILE_NEW,
	FILE_MODIFIED
};

enum inputmode {
	MODE_ANY     = 0,
	MODE_NORMAL  = 1,
	MODE_PIXEL   = 2,
	MODE_COMMAND = 3,
	MODE_PRESENT = 4
};

enum recopt {
	REC_TEST = 1 << 0
};

struct brush {
	int                       size;
	enum   dstate             drawing;
	struct point              curr;
	struct point              prev;
	struct polygon            quad;
	GLenum                    sblend, dblend;

	bool                      erase;
	bool                      multi;
};

struct snapshot {
	struct texture           *pixels;
	int                       x, y;
	int                       w, h;
	int                       nframes;
	bool                      saved;

	struct snapshot          *next, *prev;
};

struct view {
	struct framebuffer       *fb;
	int                       fw, fh;
	int                       x, y;
	int                       nframes;
	bool                      flipx, flipy;
	bool                      hover;
	struct snapshot          *snapshot;
	struct view              *prev, *next;

	char                     filename[MAX_FILENAME];
	enum filestatus          filestatus;
};

struct icon {
	rect_t                    rect;
	int                       ax, ay;

	/* Source and destination blend factors */
	GLenum                    sfactor, dfactor;
};

struct tools {
	struct texture            *texture;
	struct icon                icons[6];
};

struct input {
	char                       field[MAX_INPUT];
	unsigned                   cursor;
};

struct cmdline {
	struct input            *in;
};

struct checker {
	bool                     active;
	struct texture          *tex;
};

enum tooltype {
	TOOL_BRUSH,
	TOOL_SAMPLER,
	TOOL_PAN
};

struct tool {
	enum tooltype                  curr, prev;
	struct brush                   brush;
};

struct buffer {
	struct framebuffer            *fb;
	rect_t                         rect;
};

struct macro {
	struct macro *next;
	char         *cmd;
};

struct session {
	int                      w, h;
	int                      x, y;
	int                      mx, my;
	bool                     mousedown;
	rect_t                   mselection;
	int                      zoom;
	int                      fps;
	rect_t                   selection;
	bool                     paused;
	bool                     onion;
	bool                     help;
	bool                     recording;
	unsigned                 recopts;
	double                   started;
	rgba_t                   fg, bg;
	int                      gridw, gridh;

	struct macro            *macros;   /* Head of macro list */
	struct macro            *macro;    /* Current macro pointer */
	struct timeval           macro_tv; /* Time of last macro */
	FILE                    *play;

	struct view             *views;
	struct view             *view;
	struct context          *ctx;
	struct cmdline           cmdline;
	struct checker           checker;
	struct palette          *palette;
	struct tools             tools;
	struct tool              tool;
	struct texture          *paste;                 /* Paste buffer */
	enum inputmode           mode;

	char                     message[256];
	rgba_t                   messagecolor;
};

union arg {
	bool                 b;
	int                  i;
	unsigned int         ui;
	float                f;
	const void          *v;
	struct point         p;
	vec2_t               vec;
};

struct binding {
	const char          *name;
	enum inputmode       mode;
	int                  mods;
	int                  key;
	int                  action;
	void               (*callback)(struct session *, const union arg *);
	const union arg      arg;
};

struct command {
	const char            *name;
	const char            *description;
	bool                 (*callback)(struct session *, int, char **);
	int                    argc;
};
