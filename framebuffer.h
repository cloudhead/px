//
// framebuffer.h
//
struct framebuffer {
	GLuint                 handle;
	bool                   screen;
	struct texture        *tex;
	struct polygon         quad;
};

struct framebuffer             *framebuffer(int, int, void *);
struct framebuffer             *framebuffer_screen(int, int, void *);
struct framebuffer             *framebuffer_from(struct texture *);
void                            framebuffer_free(struct framebuffer *);
void                            framebuffer_attach(struct framebuffer *, struct texture *);
void                            framebuffer_bind(struct framebuffer *);
void                            framebuffer_draw(struct framebuffer *, struct context *ctx);
void                            framebuffer_draw_rect(struct context *, struct framebuffer *, rect_t, float, float);
void                            framebuffer_clear(void);
void                            framebuffer_clearcolor(float, float, float, float);
rgba_t                          framebuffer_sample(struct framebuffer *, int, int);
void                            framebuffer_read(struct framebuffer *, rect_t, rgba_t *);
