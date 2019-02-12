//
// util.c
// utility functions and macros
//
#include <GL/glew.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#include "util.h"

void __attribute__((noreturn)) _fatalf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}

void _errorf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

bool timestamp(char *str, size_t size)
{
	time_t     t   = time(NULL);
	struct tm *tmp = localtime(&t);

	if (! tmp) {
		error("px", "Error: test/save: localtime faiiled");
		return false;
	}

	if (! strftime(str, size, "recording-%Y%m%d.px", tmp)) {
		error("px", "Error: test/save: strftime failed");
		return false;
	}
	return true;
}

//
// Read an entire file into memory.
//
char *readfile(const char *path)
{
	FILE   *fp;
	char   *buffer;
	long   size;

	if (! (fp = fopen(path, "rb"))) {
		fatalf("util", "error reading '%s'", path);
	}

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	buffer = malloc((size_t)size + 1);

	fread(buffer, (size_t)size, 1, fp);
	fclose(fp);

	buffer[size] = '\0';

	return buffer;
}

size_t freadstr(char **strp, FILE *fp)
{
	size_t len = (size_t)fgetc(fp);

	if (len > 0) {
		*strp = malloc(len + 1);
		fread(*strp, len, 1, fp);
	}
	(*strp)[len] = '\0';

	return len;
}

char *fgetstr(char *str, int n, FILE *stream)
{
	char *result = fgets(str, n, stream);

	if (! result)
		return result;

	if (str[strlen(str) - 1] == '\n')
		str[strlen(str) - 1] = '\0';

	return str;
}

const char *fileext(const char *path)
{
	const char *dot = strrchr(path, '.');
	if (!dot || dot == path) return "";
	return dot + 1;
}

void striptrailing(char *path)
{
	size_t len = strlen(path);
	if (path[len - 1] == '/')
		path[len - 1] = '\0';
}

int vasprintf(char **str, const char *fmt, va_list args)
{
	int size = 0;
	va_list tmp;

	va_copy(tmp, args);
	size = vsnprintf(NULL, 0, fmt, tmp);
	va_end(tmp);

	if (size < 0)
		return -1;

	*str = malloc((size_t)size + 1);

	if (NULL == *str)
		return -1;

	return vsprintf(*str, fmt, args);
}

int asprintf(char **str, const char *fmt, ...)
{
	int size = 0;

	va_list args;
	va_start(args, fmt);
	size = vasprintf(str, fmt, args);
	va_end(args);

	return size;
}

bool strprefix(const char *str, const char *pre)
{
	return !strncmp(pre, str, strlen(pre));
}

iter_t iter(void *current, void *(*next)(struct iter *), void (*init)(struct iter *))
{
	iter_t it;

	it.next  = next;
	it.value = current;
	it.init  = init;
	it.ok    = false;

	it.init(&it);

	return it;
}
