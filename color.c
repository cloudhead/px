/*
 * color.c
 * color utilities
 */
#include <inttypes.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "linmath.h"
#include "color.h"

static float hue(float h, float m1, float m2)
{
    h = h < 0 ? h + 1 : (h > 1 ? h - 1 : h);

    if      (h * 6 < 1) return m1 + (m2 - m1) * h * 6.f;
    else if (h * 2 < 1) return m2;
    else if (h * 3 < 2) return m1 + (m2 - m1) * (2.f/3.f - h) * 6.f;
    else                return m1;
}

rgba_t hsla2rgba(struct hsla hsla)
{
	float h = (float)hsla.h,
	      s = (float)hsla.s,
	      l = (float)hsla.l,
	      a = (float)hsla.a;

	h = fmodf(h, 1.f);

	float m2 = l <= 0.5f ? l * (s + 1) : l + s - l * s;
	float m1 = l * 2.f - m2;

	return (struct rgba){
		(uint8_t)(hue(h + 1.f/3.f, m1, m2) * 255.f),
		(uint8_t)(hue(h,           m1, m2) * 255.f),
		(uint8_t)(hue(h - 1.f/3.f, m1, m2) * 255.f),
		(uint8_t)(a * 255.f)
	};
}

hsla_t rgba2hsla(struct rgba rgba)
{
	float r = (float)rgba.r/255.f,
	      g = (float)rgba.g/255.f,
	      b = (float)rgba.b/255.f,
	      a = (float)rgba.a/255.f;

	float mx = max(max(r, g), b),
	      mn = min(min(r, g), b);

	float h, s,
	      l = (mx + mn) / 2.f,
	      d = mx - mn;

	if (mx == mn) {
		h = s = 0.f;
	} else {
		s = l > 0.5f ? d / (2.f - mx - mn) : d / (mx + mn);

		if      (r == mx) h = (g - b) / d + (g < b ? 6.f : 0.f);
		else if (g == mx) h = (b - r) / d + 2.f;
		else              h = (r - g) / d + 4.f;

		h /= 6;
	}
	return (struct hsla){h, s, l, a};
}

rgba_t hex2rgba(const char *hex)
{
	assert(hex[0] == '#');

	uint32_t   c      = (uint32_t)strtol(&hex[1], NULL, 16);
	uint8_t   *bgr    = (uint8_t *)&c;

	return rgba(bgr[2], bgr[1], bgr[0], 0xff);
}

inline int rgbacmp(rgba_t a, rgba_t b)
{
	return memcmp(&a, &b, sizeof(rgba_t));
}
