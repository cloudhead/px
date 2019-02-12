//
// animation.h
//
enum animstate {
	ANIM_STOPPED = 0,
	ANIM_PLAYING = 1,
	ANIM_PAUSED  = 2
};

struct animation {
	double                   elapsed;      // Elapsed time since the beginning of the animation
	float                    scaling;      // Animation scaling (1/speed)
	uint16_t                 len;          // Animation length
	uint16_t                 cursor;       // Current animation frame
	enum animstate           state;        // Current animation state
};

void                 animation_reset(struct animation *);
void                 animation_play(struct animation *);
void                 animation_stop(struct animation *);
void                 animation_step(struct animation *, double);
void                 animation_set(struct animation *, uint16_t, float);
uint16_t             animation_value(struct animation *);
