//
// linmath.h
// linear algebra library
//
#ifndef LINMATH__H
#define LINMATH__H

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <float.h>

#if defined(__SSE4_2__)
#include <smmintrin.h>
#endif

static const float PI = 3.14149265359f;

#define vnorm(A) _Generic((A), vec3_t: vec3norm, vec4_t: vec4norm)
#define vdot(A)  _Generic((A), vec3_t: vec3dot, vec4_t: vec4dot)

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#if defined(__SSE4_2__)
#define vec4dot(...) ((vec4_t){ vec4dot_m128((__VA_ARGS__).m128) })
#endif

#if defined(__cplusplus)
#define vec2(x, y)            {x, y}
#define vec3(x, y, z)         {x, y, z}
#define vec4(x, y, z, w)      {x, y, z, w}
#define line2(p1, p2)         {p1, p2}
#define rect(a, b, c, d)      {a, b, c, d}
#else
#define vec2(x, y)            ((vec2_t){x, y})
#define vec3(x, y, z)         ((vec3_t){x, y, z})
#define vec4(x, y, z, w)      ((vec4_t){x, y, z, w})
#define line2(p1, p2)         ((line2_t){p1, p2})
#define rect(a, b, c, d)      ((rect_t){a, b, c, d})
#endif

#define vec4identity          vec4(1.f, 1.f, 1.f, 1.f)

typedef struct vec2 vec2_t;
typedef struct vec3 vec3_t;
typedef struct vec4 vec4_t;
typedef struct mat4 mat4_t;
typedef struct line2 line2_t;
typedef struct rect rect_t;

struct vec2 {
	float x, y;
};

struct vec3 {
	float x, y, z;
};

struct vec4 {
#if defined(__cplusplus)
	float x, y, z, w;
#else
	union {
		struct {
			float x, y, z, w;
		};
		struct {
			float r, g, b, a;
		};
		float n[4];
#if defined(__SSE4_2__)
		__m128 m128;
#endif
	};
#endif
};

struct line2 {
#if defined(__cplusplus)
	vec2_t p1, p2;
#else
	union {
		struct { vec2_t p1, p2; };
		struct { float x1, y1, x2, y2; };
#if defined(__SSE4_2__)
		__m128 m128;
#endif
	};
#endif
};

struct mat4 {
#if defined(__cplusplus)
	vec4_t cols[4];
#else
	union {
		vec4_t cols[4];
		struct {
			float a1, a2, a3, a4;
			float b1, b2, b3, b4;
			float c1, c2, c3, c4;
			float d1, d2, d3, d4;
		};
	};
#endif
};

struct rect {
#if defined(__cplusplus)
	float x1, y1, x2, y2;
#else
	union {
		struct { float x, y, w, h; };
		struct { float x1, y1, x2, y2; };
#if defined(__SSE4_2__)
		__m128 m128;
#endif
	};
#endif
};

static inline int rect_w(rect_t *r)
{
	return (int)floor(fabsf(r->x2 - r->x1));
}

static inline int rect_h(rect_t *r)
{
	return (int)floor(fabsf(r->y2 - r->y1));
}

static inline line2_t rect_left(rect_t r)
{
	line2_t l;

	l.p1 = vec2(r.x1, r.y1);
	l.p2 = vec2(r.x1, r.y2);

	return l;
}

static inline line2_t rect_right(rect_t r)
{
	line2_t l;

	l.p1 = vec2(r.x2, r.y1);
	l.p2 = vec2(r.x2, r.y2);

	return l;
}

static inline line2_t rect_bottom(rect_t r)
{
	line2_t l;

	l.p1 = vec2(r.x1, r.y1);
	l.p2 = vec2(r.x2, r.y1);

	return l;
}

static inline line2_t rect_top(rect_t r)
{
	line2_t l;

	l.p1 = vec2(r.x1, r.y2);
	l.p2 = vec2(r.x2, r.y2);

	return l;
}

static inline rect_t rect_translate(rect_t r, vec2_t v)
{
	return rect(
		r.x1 + v.x,
		r.y1 + v.y,
		r.x2 + v.x,
		r.y2 + v.y
	);
}

static inline vec2_t rect_center(rect_t *r)
{
	return vec2(
		(r->x1 + r->x2) / 2,
		(r->y1 + r->y2) / 2
	);
}

static inline bool rect_intersects(rect_t *a, rect_t *b)
{
	return a->y2 > b->y1 && a->y1 < b->y2 && a->x1 < b->x2 && a->x2 > b->x1;
}

static inline bool rect_isempty(rect_t r)
{
	return r.x1 == r.x2 || r.y1 == r.y2;
}

static inline rect_t rect_flipy(rect_t r)
{
	return rect(r.x1, -r.y1, r.x2, -r.y2);
}

static inline vec2_t rect_min(rect_t r)
{
	return vec2(min(r.x1, r.x2), min(r.y1, r.y2));
}

static inline vec2_t rect_max(rect_t r)
{
	return vec2(max(r.x1, r.x2), max(r.y1, r.y2));
}

static inline rect_t rect_norm(rect_t r)
{
	vec2_t min = rect_min(r);
	vec2_t max = rect_max(r);

	return rect(min.x, min.y, max.x, max.y);
}

static int rect_snprint(char *str, size_t size, rect_t *r)
{
	return snprintf(str, size, "%.0f,%.0f %.0f,%.0f", r->x1, r->y1, r->x2, r->y2);
}

static bool line2_intersects(line2_t a, line2_t b, float *ix, float *iy)
{
	float s1x, s1y, s2x, s2y;

	s1x = a.p2.x - a.p1.x;     s1y = a.p2.y - a.p1.y;
	s2x = b.p2.x - b.p1.x;     s2y = b.p2.y - b.p1.y;

	float s, t;
	float det = (-s2x * s1y + s1x * s2y);

	if (fabs(det) <= FLT_EPSILON) /* Segments are collinear */
		return false;

	s = (-s1y * (a.p1.x - b.p1.x) + s1x * (a.p1.y - b.p1.y)) / det;
	t = ( s2x * (a.p1.y - b.p1.y) - s2y * (a.p1.x - b.p1.x)) / det;

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
		if (ix != NULL)
			*ix = a.p1.x + (t * s1x);
		if (iy != NULL)
			*iy = a.p1.y + (t * s1y);
		return true;
	}
	return false;
}

static bool line2_rect_intersects(line2_t l, rect_t r, float *ix, float *iy)
{
	return     line2_intersects(l, rect_left(r),   ix, iy)
		|| line2_intersects(l, rect_right(r),  ix, iy)
		|| line2_intersects(l, rect_top(r),    ix, iy)
		|| line2_intersects(l, rect_bottom(r), ix, iy);
}

static inline vec2_t vec2add(vec2_t a, vec2_t b)
{
	return vec2(
		a.x + b.x,
		a.y + b.y
	);
}

static inline vec2_t vec2sub(vec2_t a, vec2_t b)
{
	return vec2(
		a.x - b.x,
		a.y - b.y
	);
}

static inline vec2_t vec2scale(vec2_t a, float s)
{
	return vec2(
		a.x * s,
		a.y * s
	);
}

static inline float vec2dot(vec2_t a, vec2_t b)
{
	return a.x * b.x + a.y * b.y;
}

static inline float vec2len(vec2_t v)
{
	return sqrtf(vec2dot(v, v));
}

static inline vec2_t vec2norm(vec2_t v)
{
	float k = 1.0f / vec2len(v);
	return vec2scale(v, k);
}

#if !defined(__cplusplus)
static inline vec3_t vec3add(vec3_t a, vec3_t b)
{
	return vec3(
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	);
}

static inline vec3_t vec3sub(vec3_t a, vec3_t b)
{
	return vec3(
		a.x - b.x,
		a.y - b.y,
		a.z - b.z
	);
}

static inline vec3_t vec3scale(vec3_t a, float s)
{
	return vec3(
		a.x * s,
		a.y * s,
		a.z * s
	);
}

static inline vec3_t vec3rotateY(vec3_t a, float angle)
{
	return vec3(
		 a.x * cosf(angle) + a.z * sinf(angle),
		 a.y,
		-a.x * sinf(angle) + a.z * cosf(angle)
	);
}

static inline float vec3dot(vec3_t a, vec3_t b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec4_t mat4row(mat4_t m, int i)
{
	return vec4(
		m.cols[0].n[i],
		m.cols[1].n[i],
		m.cols[2].n[i],
		m.cols[3].n[i]
	);
}

static inline vec3_t vec3cross(vec3_t a, vec3_t b)
{
	return vec3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

static inline float vec3len(vec3_t v)
{
	return sqrtf(vec3dot(v, v));
}

static inline vec3_t vec3norm(vec3_t v)
{
	float k = 1.0f / vec3len(v);
	return vec3scale(v, k);
}

static inline vec4_t vec4add(vec4_t a, vec4_t b)
{
	return vec4(
		a.x + b.x,
		a.y + b.y,
		a.z + b.z,
		a.w + b.w
	);
}

static inline vec4_t vec4scale(vec4_t v, float s)
{
	return vec4(
		v.x * s,
		v.y * s,
		v.z * s,
		v.w * s
	);
}

static inline float vec4dot(vec4_t a, vec4_t b)
{
	return b.x * a.x + b.y * a.y + b.z * a.z + b.w * a.w;
}

static inline vec3_t vec3transform(vec3_t a, mat4_t m)
{
	return vec3(
		vec4dot(mat4row(m, 0), vec4(a.x, a.y, a.z, 1.0f)),
		vec4dot(mat4row(m, 1), vec4(a.x, a.y, a.z, 1.0f)),
		vec4dot(mat4row(m, 2), vec4(a.x, a.y, a.z, 1.0f))
	);
}

static inline void mat4transform(mat4_t *m, float tx, float ty, float sx, float sy)
{
	/* Translate */
	m->cols[3].x += tx;
	m->cols[3].y += ty;

	/* Scale */
	m->cols[0].x *= sx;
	m->cols[1].y *= sy;
}

static inline void mat4print(mat4_t m, FILE *fp)
{
	fprintf(fp, "%.3f %.3f %.3f %.3f\n", m.cols[0].x, m.cols[1].x, m.cols[2].x, m.cols[3].x);
	fprintf(fp, "%.3f %.3f %.3f %.3f\n", m.cols[0].y, m.cols[1].y, m.cols[2].y, m.cols[3].y);
	fprintf(fp, "%.3f %.3f %.3f %.3f\n", m.cols[0].z, m.cols[1].z, m.cols[2].z, m.cols[3].z);
	fprintf(fp, "%.3f %.3f %.3f %.3f\n", m.cols[0].w, m.cols[1].w, m.cols[2].w, m.cols[3].w);
}

static inline mat4_t mat4identity() {
	static mat4_t identity = (mat4_t){
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	return identity;
}

static inline mat4_t mat4add(mat4_t a, mat4_t b)
{
	mat4_t out;

	for (int i = 0; i < 4; i++) {
		out.cols[i].x = a.cols[i].x + b.cols[i].x;
		out.cols[i].y = a.cols[i].y + b.cols[i].y;
		out.cols[i].z = a.cols[i].z + b.cols[i].z;
		out.cols[i].w = a.cols[i].w + b.cols[i].w;
	}
	return out;
}

static inline mat4_t mat4scale(mat4_t a, float x, float y)
{
	mat4_t out = a;

	out.cols[0].x = x;
	out.cols[1].y = y;
	out.cols[2].z = 1;

	return out;
}

static inline vec3_t mat4scaling(mat4_t *m)
{
	return vec3(m->cols[0].x, m->cols[1].y, m->cols[2].z);
}

static inline mat4_t mat4mul(mat4_t a, mat4_t b)
{
	mat4_t out;

	for (int c = 0; c < 4; ++c) {
		for (int r = 0; r < 4; ++r) {
			out.cols[c].n[r] = 0.0f;
			for (int k = 0; k < 4; ++k) {
				out.cols[c].n[r] += a.cols[k].n[r] * b.cols[c].n[k];
			}
		}
	}
	return out;
}

static inline mat4_t mat4translate(mat4_t a, vec3_t t)
{
	mat4_t out = a;
	out.cols[3].x = t.x;
	out.cols[3].y = t.y;
	out.cols[3].z = t.z;
	return out;
}

static inline vec3_t mat4translation(mat4_t *a)
{
	return vec3(a->cols[3].x, a->cols[3].y, a->cols[3].z);
}

static inline mat4_t mat4rotateX(mat4_t M, float angle)
{
	float s = sinf(angle);
	float c = cosf(angle);
	mat4_t R = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f,    c,    s, 0.0f,
		0.0f,   -s,    c, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	return mat4mul(M, R);
}

static inline mat4_t mat4rotateY(mat4_t M, float angle)
{
	float s = sinf(angle);
	float c = cosf(angle);
	mat4_t R = {
		   c, 0.0f,    s, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		  -s, 0.0f,    c, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	return mat4mul(M, R);
}

static inline mat4_t mat4rotateZ(mat4_t M, float angle)
{
	float s = sinf(angle);
	float c = cosf(angle);
	mat4_t R = {
		   c,    s, 0.0f, 0.0f,
		  -s,    c, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	return mat4mul(M, R);
}

static inline mat4_t mat4invert(mat4_t M)
{
	mat4_t T;
	float s[6];
	float c[6];

	s[0] = M.cols[0].x * M.cols[1].y - M.cols[1].x * M.cols[0].y;
	s[1] = M.cols[0].x * M.cols[1].z - M.cols[1].x * M.cols[0].z;
	s[2] = M.cols[0].x * M.cols[1].w - M.cols[1].x * M.cols[0].w;
	s[3] = M.cols[0].y * M.cols[1].z - M.cols[1].y * M.cols[0].z;
	s[4] = M.cols[0].y * M.cols[1].w - M.cols[1].y * M.cols[0].w;
	s[5] = M.cols[0].z * M.cols[1].w - M.cols[1].z * M.cols[0].w;

	c[0] = M.cols[2].x * M.cols[3].y - M.cols[3].x * M.cols[2].y;
	c[1] = M.cols[2].x * M.cols[3].z - M.cols[3].x * M.cols[2].z;
	c[2] = M.cols[2].x * M.cols[3].w - M.cols[3].x * M.cols[2].w;
	c[3] = M.cols[2].y * M.cols[3].z - M.cols[3].y * M.cols[2].z;
	c[4] = M.cols[2].y * M.cols[3].w - M.cols[3].y * M.cols[2].w;
	c[5] = M.cols[2].z * M.cols[3].w - M.cols[3].z * M.cols[2].w;

	// Assumes it is invertible
	float idet = 1.0f /
		(s[0] * c[5] - s[1] * c[4] + s[2] * c[3] +
		 s[3] * c[2] - s[4] * c[1] + s[5] * c[0]);

	T.cols[0].x = ( M.cols[1].y * c[5] - M.cols[1].z * c[4] + M.cols[1].w * c[3]) * idet;
	T.cols[0].y = (-M.cols[0].y * c[5] + M.cols[0].z * c[4] - M.cols[0].w * c[3]) * idet;
	T.cols[0].z = ( M.cols[3].y * s[5] - M.cols[3].z * s[4] + M.cols[3].w * s[3]) * idet;
	T.cols[0].w = (-M.cols[2].y * s[5] + M.cols[2].z * s[4] - M.cols[2].w * s[3]) * idet;
	T.cols[1].x = (-M.cols[1].x * c[5] + M.cols[1].z * c[2] - M.cols[1].w * c[1]) * idet;
	T.cols[1].y = ( M.cols[0].x * c[5] - M.cols[0].z * c[2] + M.cols[0].w * c[1]) * idet;
	T.cols[1].z = (-M.cols[3].x * s[5] + M.cols[3].z * s[2] - M.cols[3].w * s[1]) * idet;
	T.cols[1].w = ( M.cols[2].x * s[5] - M.cols[2].z * s[2] + M.cols[2].w * s[1]) * idet;
	T.cols[2].x = ( M.cols[1].x * c[4] - M.cols[1].y * c[2] + M.cols[1].w * c[0]) * idet;
	T.cols[2].y = (-M.cols[0].x * c[4] + M.cols[0].y * c[2] - M.cols[0].w * c[0]) * idet;
	T.cols[2].z = ( M.cols[3].x * s[4] - M.cols[3].y * s[2] + M.cols[3].w * s[0]) * idet;
	T.cols[2].w = (-M.cols[2].x * s[4] + M.cols[2].y * s[2] - M.cols[2].w * s[0]) * idet;
	T.cols[3].x = (-M.cols[1].x * c[3] + M.cols[1].y * c[1] - M.cols[1].z * c[0]) * idet;
	T.cols[3].y = ( M.cols[0].x * c[3] - M.cols[0].y * c[1] + M.cols[0].z * c[0]) * idet;
	T.cols[3].z = (-M.cols[3].x * s[3] + M.cols[3].y * s[1] - M.cols[3].z * s[0]) * idet;
	T.cols[3].w = ( M.cols[2].x * s[3] - M.cols[2].y * s[1] + M.cols[2].z * s[0]) * idet;

	return T;
}

static inline mat4_t mat4lookAt(vec3_t eye, vec3_t center, vec3_t up)
{
	mat4_t out;

	vec3_t f = vec3norm(vec3sub(center, eye));
	vec3_t s = vec3norm(vec3cross(f, up));
	vec3_t t = vec3cross(s, f);

	out.cols[0].x =  s.x;
	out.cols[0].y =  t.x;
	out.cols[0].z = -f.x;
	out.cols[0].w =  0.f;

	out.cols[1].x =  s.y;
	out.cols[1].y =  t.y;
	out.cols[1].z = -f.y;
	out.cols[1].w =  0.f;

	out.cols[2].x =  s.z;
	out.cols[2].y =  t.z;
	out.cols[2].z = -f.z;
	out.cols[2].w =  0.f;

	out.cols[3].x = -vec3dot(s, eye);
	out.cols[3].y = -vec3dot(t, eye);
	out.cols[3].z =  vec3dot(f, eye);
	out.cols[3].w =  1.f;

	return out;
}

static inline mat4_t mat4ortho(float r, float t)
{
	mat4_t out;

	float n = -1, f = 1;  // Near and far
	float l =  0, b = 0;  // Left and bottom

	out.cols[0].x = 2.f / (r - l);
	out.cols[0].y = out.cols[0].z = out.cols[0].w = 0.f;

	out.cols[1].y = 2.f / (t - b);
	out.cols[1].x = out.cols[1].z = out.cols[1].w = 0.f;

	out.cols[2].z = -2.f / (f - n);
	out.cols[2].x = out.cols[2].y = out.cols[2].w = 0.f;

	out.cols[3].x = -(r + l) / (r - l);
	out.cols[3].y = -(t + b) / (t - b);
	out.cols[3].z = -(f + n) / (f - n);
	out.cols[3].w = 1.f;

	return out;
}
#endif

#if defined(__cplusplus)
#undef min
#undef max
#endif

#endif // LINMATH_H
