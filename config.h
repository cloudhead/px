/*
 * config.h
 * user configuration
 */
static struct binding bindings[] = {
	/* --------------------------------------------------------------------------------------------------------------------------- */
	/* name                mode         modifier                key                    action         callback            argument */
	/* --------------------------------------------------------------------------------------------------------------------------- */

	/* ALL MODES */
	{"command",            MODE_ANY,    GLFW_MOD_SHIFT,         GLFW_KEY_SEMICOLON,    GLFW_PRESS,    kb_cmdmode,         { 0 }},
	{"pixel mode",         MODE_ANY,    0,                      GLFW_KEY_V,            GLFW_PRESS,    kb_pixelmode,       { 0 }},
	{"presentation mode",  MODE_ANY,    0,                      GLFW_KEY_F11,          GLFW_PRESS,    kb_presentmode,     { 0 }},
	{"zoom in",            MODE_ANY,    0,                      '.',                   GLFW_PRESS,    kb_zoom,            { .i = +1 }},
	{"zoom out",           MODE_ANY,    0,                      ',',                   GLFW_PRESS,    kb_zoom,            { .i = -1 }},
	{"create frame",       MODE_ANY,    GLFW_MOD_CONTROL,       GLFW_KEY_F,            GLFW_PRESS,    kb_create_frame,    { 0 }},
	{"create view",        MODE_ANY,    GLFW_MOD_CONTROL,       GLFW_KEY_N,            GLFW_PRESS,    kb_create_view,     { 0 }},
	{"save",               MODE_ANY,    GLFW_MOD_CONTROL,       GLFW_KEY_S,            GLFW_PRESS,    kb_save,            { 0 }},
	{"undo",               MODE_ANY,    0,                      GLFW_KEY_U,            GLFW_PRESS,    kb_undo,            { 0 }},
	{"redo",               MODE_ANY,    GLFW_MOD_CONTROL,       GLFW_KEY_R,            GLFW_PRESS,    kb_redo,            { 0 }},
	{"pause",              MODE_ANY,    0,                      GLFW_KEY_ENTER,        GLFW_PRESS,    kb_pause,           { 0 }},
	{"nudge up",           MODE_ANY,    0,                      GLFW_KEY_UP,           GLFW_PRESS,    kb_nudge,           { .p = {0, +1} }},
	{"nudge down",         MODE_ANY,    0,                      GLFW_KEY_DOWN,         GLFW_PRESS,    kb_nudge,           { .p = {0, -1} }},
	{"nudge left",         MODE_ANY,    0,                      GLFW_KEY_LEFT,         GLFW_PRESS,    kb_nudge,           { .p = {-1, 0} }},
	{"nudge right",        MODE_ANY,    0,                      GLFW_KEY_RIGHT,        GLFW_PRESS,    kb_nudge,           { .p = {+1, 0} }},
	{"pan",                MODE_ANY,    0,                      GLFW_KEY_SPACE,        GLFW_PRESS,    kb_pan,             { true }},
	{"pan",                MODE_ANY,    0,                      GLFW_KEY_SPACE,        GLFW_RELEASE,  kb_pan,             { false }},
	{"onion",              MODE_ANY,    0,                      GLFW_KEY_O,            GLFW_PRESS,    kb_onion,           { true }},
	{"fps +",              MODE_ANY,    GLFW_MOD_SHIFT,         GLFW_KEY_EQUAL,        GLFW_PRESS,    kb_adjust_fps,      { .i = +1 }},
	{"fps -",              MODE_ANY,    0,                      GLFW_KEY_MINUS,        GLFW_PRESS,    kb_adjust_fps,      { .i = -1 }},
	{"flip x",             MODE_ANY,    0,                      GLFW_KEY_BACKSLASH,    GLFW_PRESS,    kb_flipx,           { true }},
	{"flip x",             MODE_ANY,    0,                      GLFW_KEY_BACKSLASH,    GLFW_RELEASE,  kb_flipx,           { false }},
	{"flip y",             MODE_ANY,    GLFW_MOD_SHIFT,         GLFW_KEY_BACKSLASH,    GLFW_PRESS,    kb_flipy,           { true }},
	{"flip y",             MODE_ANY,    GLFW_MOD_SHIFT,         GLFW_KEY_BACKSLASH,    GLFW_RELEASE,  kb_flipy,           { false }},
	{"help",               MODE_ANY,    0,                      GLFW_KEY_SLASH,        GLFW_PRESS,    kb_help,            { true }},
	{"help",               MODE_ANY,    0,                      GLFW_KEY_SLASH,        GLFW_RELEASE,  kb_help,            { false }},

	/* NORMAL MODE */
	{"brush",              MODE_NORMAL, 0,                      GLFW_KEY_B,            GLFW_PRESS,    kb_brush,           { 0 }},
	{"eraser",             MODE_NORMAL, 0,                      GLFW_KEY_E,            GLFW_PRESS,    kb_eraser,          { 0 }},
	{"move left",          MODE_NORMAL, 0,                      GLFW_KEY_H,            GLFW_PRESS,    kb_move,            { .p = {-1, 0} }},
	{"move right",         MODE_NORMAL, 0,                      GLFW_KEY_L,            GLFW_PRESS,    kb_move,            { .p = {+1, 0} }},
	{"next view",          MODE_NORMAL, 0,                      GLFW_KEY_J,            GLFW_PRESS,    kb_next_view,       { 0 }},
	{"prev view",          MODE_NORMAL, 0,                      GLFW_KEY_K,            GLFW_PRESS,    kb_prev_view,       { 0 }},
	{"center view",        MODE_NORMAL, 0,                      GLFW_KEY_Z,            GLFW_PRESS,    kb_center_view,     { 0 }},
	{"swap colors",        MODE_NORMAL, 0,                      GLFW_KEY_X,            GLFW_PRESS,    kb_swap_colors,     { 0 }},
	{"brush size +",       MODE_NORMAL, 0,                      ']',                   GLFW_PRESS,    kb_brush_size,      { .i = +1 }},
	{"brush size -",       MODE_NORMAL, 0,                      '[',                   GLFW_PRESS,    kb_brush_size,      { .i = -1 }},

	/* PIXEL MODE */
	{"move left",          MODE_PIXEL, 0,                      GLFW_KEY_H,            GLFW_PRESS,    kb_px_cursor,        { .vec = { -1,  0 } }},
	{"move right",         MODE_PIXEL, 0,                      GLFW_KEY_L,            GLFW_PRESS,    kb_px_cursor,        { .vec = { +1,  0 } }},
	{"move down",          MODE_PIXEL, 0,                      GLFW_KEY_J,            GLFW_PRESS,    kb_px_cursor,        { .vec = {  0, -1 } }},
	{"move up",            MODE_PIXEL, 0,                      GLFW_KEY_K,            GLFW_PRESS,    kb_px_cursor,        { .vec = {  0, +1 } }},
	{"move select left",   MODE_PIXEL, GLFW_MOD_SHIFT,         GLFW_KEY_H,            GLFW_PRESS,    kb_px_cursor_select, { .vec = { -1,  0 } }},
	{"move select right",  MODE_PIXEL, GLFW_MOD_SHIFT,         GLFW_KEY_L,            GLFW_PRESS,    kb_px_cursor_select, { .vec = { +1,  0 } }},
	{"move select down",   MODE_PIXEL, GLFW_MOD_SHIFT,         GLFW_KEY_J,            GLFW_PRESS,    kb_px_cursor_select, { .vec = {  0, -1 } }},
	{"move select up",     MODE_PIXEL, GLFW_MOD_SHIFT,         GLFW_KEY_K,            GLFW_PRESS,    kb_px_cursor_select, { .vec = {  0, +1 } }},
	{"select next frame",  MODE_PIXEL, 0,                      GLFW_KEY_W,            GLFW_PRESS,    kb_px_move_frame,    { .i = +1 }},
	{"select prev frame",  MODE_PIXEL, 0,                      GLFW_KEY_B,            GLFW_PRESS,    kb_px_move_frame,    { .i = -1 }},
	{"select + frame",     MODE_PIXEL, GLFW_MOD_SHIFT,         GLFW_KEY_W,            GLFW_PRESS,    kb_px_select_frame,  { .i = +1 }},
	{"select - frame",     MODE_PIXEL, GLFW_MOD_SHIFT,         GLFW_KEY_B,            GLFW_PRESS,    kb_px_select_frame,  { .i = -1 }},
	{"copy",               MODE_PIXEL, 0,                      GLFW_KEY_Y,            GLFW_PRESS,    kb_px_copy,          { 0 }},
	{"cut",                MODE_PIXEL, 0,                      GLFW_KEY_X,            GLFW_PRESS,    kb_px_cut,           { 0 }},
	{"paste",              MODE_PIXEL, 0,                      GLFW_KEY_P,            GLFW_PRESS,    kb_px_paste,         { 0 }},
};
