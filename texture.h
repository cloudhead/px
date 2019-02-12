//
// texture.h
// texture utilities
//
struct texture {
	void    *data;
	GLuint   handle;
	GLuint   sampler;
	int      w, h;
};

GLuint gen_sampler(GLuint minFilter, GLuint magFilter);

struct texture *texture(const void *pixels, int w, int h, GLint format);
struct texture *texture_load(const char *path, GLint format);
struct texture *texture_read(rect_t);
void            texture_repeat(float, float);
void            texture_free(struct texture *);
void            texture_bind(struct texture *);
