/*
 * color.h
 * color utilities
 */
typedef struct rgba  rgba_t;
typedef struct hsla  hsla_t;

#define rgba2vec4(rgba)      vec4((rgba).r/255.f, (rgba).g/255.f, \
		                  (rgba).b/255.f, (rgba).a/255.f)

#if defined(__cplusplus)
#define rgba(r,g,b,a)        rgba({r, g, b, a})
#else
#define rgba(r,g,b,a)        ((struct rgba){r, g, b, a})
#endif

#define RGBA_WHITE           rgba(255, 255, 255, 255)
#define RGBA_GREY            rgba(127, 127, 127, 255)
#define RGBA_RED             rgba(255, 0, 0, 255)
#define RGBA_BLACK           rgba(0, 0, 0, 255)
#define RGBA_TRANSPARENT     rgba(0, 0, 0, 0)

struct rgba {
	uint8_t r, g, b, a;
};

struct hsla {
	float h, s, l, a;
};

rgba_t hsla2rgba(hsla_t);
hsla_t rgba2hsla(rgba_t);
rgba_t hex2rgba(const char *);

int    rgbacmp(rgba_t, rgba_t);
