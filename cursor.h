/*
 * cursor.h
 * hardware cursor handling
 */
#include <GLFW/glfw3.h>

typedef GLFWcursor cursor_t;
cursor_t *load_cursor(const char *, int, int);
