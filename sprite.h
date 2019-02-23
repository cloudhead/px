//
// sprite.h
// sprite rendering and manipulation
//
#include <GL/glew.h>

#include "linmath.h"

#ifndef RESOURCE_PATH
#define RESOURCE_PATH "assets"
#endif

struct context;
struct texture;

struct sprite *sprite(struct texture *, rect_t);

//////////////////
// SPRITE BATCH //

#define SPRITEBATCH_INITIAL_SIZE 128

struct spritebatch {
	GLuint            vbo, vao;
	struct texture   *tex;
	struct vertex    *data;
	size_t            cap, len;
};

bool spritebatch_init(struct spritebatch *, struct texture *);
void spritebatch_grow(struct spritebatch *);
void spritebatch_shrink(struct spritebatch *);
void spritebatch_release(struct spritebatch *);
void spritebatch_draw(struct spritebatch *, struct context *);
bool spritebatch_add(struct spritebatch *, rect_t, rect_t, float, float, vec4_t);
void spritebatch_reset(struct spritebatch *);

void spritebatch_inittext(struct spritebatch *, struct font *);
void spritebatch_addtext(struct spritebatch *, struct font *, float *, float, float, bool, const char *, ...);
void spritebatch_vaddtext(struct spritebatch *, struct font *, float *, float, float, bool, const char *, va_list);
void spritebatch_drawtext(struct spritebatch *, rgba_t, struct context *);

size_t spritebatch_size(struct spritebatch *);

struct sequence {
	char            name[16];
	uint8_t         values[24];
	uint8_t         len;
};

// Sprite draw options

#define FLIPX  (1 << 0) // 00000001
#define FLIPY  (1 << 1) // 00000010

struct sprite {
	char                     name[16];
	rect_t                   rect;
	struct texture          *texture;

	int                      nframes;
	int                      fw, fh;
};

void                 draw_sprite(struct spritebatch *, struct sprite *, int, int, float, float, float, float, float, float, vec4_t, unsigned int);
bool                 load_sprite(struct sprite *, const char *, int, int);
