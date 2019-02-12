//
// util.h
// utility functions and macros
//
#ifndef UTIL_H
#define UTIL_H

#define elems(a) (sizeof(a) / sizeof(a[0]))

// Logging

void _fatalf(const char *, ...);
void _errorf(const char *, ...);

#define infof(mod, fmt, ...)     fprintf(stderr, "[%s] %s:%d: " fmt "\n", mod, __FILE__, __LINE__, __VA_ARGS__)
#define info(mod, str)           fprintf(stderr, "[%s] %s:%d: " str "\n",mod,  __FILE__, __LINE__)
#define fatalf(mod, fmt, ...)   _fatalf("[%s] %s:%d: " fmt "\n", mod, __FILE__, __LINE__, __VA_ARGS__)
#define fatal(mod, str)         _fatalf("[%s] %s:%d: " str "\n", mod, __FILE__, __LINE__)
#define errorf(mod, fmt, ...)   _errorf("[%s] %s:%d: " fmt "\n", mod, __FILE__, __LINE__, __VA_ARGS__)
#define error(mod, str)         _errorf("[%s] %s:%d: " str "\n", mod, __FILE__, __LINE__)

extern char *readfile(const char *);
extern size_t freadstr(char **, FILE *);
bool timestamp(char *, size_t);
const char *fileext(const char *);
char *fgetstr(char *, int, FILE *);
void striptrailing(char *);
int vasprintf(char **, const char *, va_list);
int asprintf(char **, const char *, ...);
bool strprefix(const char *, const char *);

typedef struct iter {
	void *(*next) (struct iter *);
	void  (*init) (struct iter *);
	void *value;
	bool  ok;
} iter_t;

iter_t iter(void *, void *(*)(struct iter *), void (*)(struct iter *));

#endif
