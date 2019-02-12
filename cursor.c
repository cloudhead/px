/*
 * cursor.c
 * hardware cursor handling
 */
#include <GLFW/glfw3.h>
#include <stdbool.h>

#include "cursor.h"
#include "color.h"
#include "tga.h"

cursor_t *load_cursor(const char *path, int xhot, int yhot)
{
	struct tga tga;
	tga_load(&tga, path);

	GLFWimage image;
	image.width  = tga.width;
	image.height = tga.height;
	image.pixels = (unsigned char *)tga.data;

	GLFWcursor *cursor = glfwCreateCursor(&image, xhot, yhot);
	tga_release(&tga);

	return cursor;
}

