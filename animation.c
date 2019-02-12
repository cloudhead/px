//
// animation.c
// general animation library
//
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "assert.h"
#include "color.h"
#include "text.h"
#include "sprite.h"
#include "animation.h"

// Set an animation to its initial state.
void animation_set(struct animation *a, uint16_t len, float scaling)
{
	assert(len);

	a->state        = ANIM_PLAYING;
	a->scaling      = scaling;
	a->len          = len;

	animation_reset(a);
}

void animation_reset(struct animation *a)
{
	a->elapsed    = 0.f;
	a->cursor     = 0;
}

void animation_play(struct animation *a)
{
	if (a->state == ANIM_STOPPED)
		animation_reset(a);

	a->state = ANIM_PLAYING;
}

void animation_stop(struct animation *a)
{
	a->state = ANIM_STOPPED;
}

// Step an animation by time delta `dt`.
void animation_step(struct animation *a, double dt)
{
	if (a->state != ANIM_PLAYING)
		return;

	assert(a->scaling > 0);

	double elapsed       = a->elapsed + dt;
	double fraction      = elapsed / a->scaling;
	uint16_t cursor      = (int)(floor(fraction)) % a->len;
	bool looped          = cursor < a->cursor;

	a->elapsed           = looped ? 0.f : elapsed;
	a->cursor            = cursor;
}

uint16_t animation_value(struct animation *a)
{
	return a->cursor;
}
