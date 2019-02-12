//
// program.h
// shader programs
//

enum attr {
	VERTEX_ATTR = 0,
	MULTIPLY_ATTR = 1
};

struct program {
	const char      *name;
	GLuint           handle;
	bool             bound;
};

struct program               *program(const char *, GLenum);
struct program               *program_load(const char *, const char *, const char *);
void                          program_free(struct program *);
void                          program_use(struct program *);

extern void                   set_uniform_mat4(struct program *, const char *, mat4_t *);
extern void                   set_uniform_vec2(struct program *, const char *, vec2_t *);
extern void                   set_uniform_vec3(struct program *, const char *, vec3_t *);
extern void                   set_uniform_vec4(struct program *, const char *, vec4_t *);
extern void                   set_uniform_i32(struct program *, const char *, int32_t);
