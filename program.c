//
// program.c
// shader programs
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <assert.h>

#include "util.h"
#include "linmath.h"
#include "program.h"

//
// Display compilation errors from the OpenGL shader compiler
//
static void printlog_shader(GLuint shader)
{
	char log[512];
	glGetShaderInfoLog(shader, sizeof(log), NULL, log);
	errorf("program", "%s", log);
}

static void printlog_program(GLuint program)
{
	char log[512];
	glGetProgramInfoLog(program, sizeof(log), NULL, log);
	errorf("program", "%s", log);
}

void program_use(struct program *s)
{
	if (s) { glUseProgram(s->handle); }
	else   { glUseProgram(0); }
}

void program_free(struct program *s)
{
	glDeleteProgram(s->handle);
	free(s);
}

//
// Compile the shader from file `filename`
//
static GLuint shader_load(const char *filename, GLenum type)
{
	GLchar *source = readfile(filename);

	if (source == NULL) {
		fprintf(stderr, "error opening %s", filename);
		return 0;
	}
	GLuint handle = glCreateShader(type);

	glShaderSource(handle, 1, (const char * const *)&source, NULL);
	glCompileShader(handle);

	free(source);

	GLint status;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE) {
		errorf("program", "compilation error with shader '%s': ", filename);
		printlog_shader(handle);
		glDeleteShader(handle);

		return 0;
	}
	return handle;
}

struct program *program(const char *name, GLuint handle)
{
	struct program *p = malloc(sizeof(*p));

	p->name   = name;
	p->handle = handle;

	return p;
}

struct program *program_load(const char *name, const char *vertpath, const char *fragpath)
{
	GLuint vert, frag;

	if (! (vert = shader_load(vertpath, GL_VERTEX_SHADER)))
		return NULL;

	if (! (frag = shader_load(fragpath, GL_FRAGMENT_SHADER)))
		return NULL;

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);

	GLint status;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);

	if (status != GL_TRUE) {
		errorf("program", "linking error with program '%s'", name);
		printlog_program(prog);
		glDeleteProgram(prog);

		return NULL;
	}

	return program(name, prog);
}

////////////////////////////////////////////////////////////////////////////////

void set_uniform_mat4(struct program *s, const char *name, mat4_t *m)
{
	GLint loc;

	if ((loc = glGetUniformLocation(s->handle, name)) == -1) {
		// TODO(cloudhead): Log error.
		return;
	}
	glUniformMatrix4fv(loc, 1, GL_FALSE, (float *)m);
}

void set_uniform_vec2(struct program *s, const char *name, vec2_t *v)
{
	GLint loc;

	if ((loc = glGetUniformLocation(s->handle, name)) == -1) {
		// TODO(cloudhead): Log error.
		return;
	}
	glUniform2fv(loc, 1, (float *)v);
}

void set_uniform_vec3(struct program *s, const char *name, vec3_t *v)
{
	GLint loc;

	if ((loc = glGetUniformLocation(s->handle, name)) == -1) {
		// TODO(cloudhead): Log error.
		return;
	}
	glUniform3fv(loc, 1, (float *)v);
}

void set_uniform_vec4(struct program *s, const char *name, vec4_t *v)
{
	GLint loc;

	if ((loc = glGetUniformLocation(s->handle, name)) == -1) {
		// TODO(cloudhead): Log error.
		return;
	}
	glUniform4fv(loc, 1, (float *)v);
}

void set_uniform_i32(struct program *s, const char *name, GLint i)
{
	GLint loc;

	if ((loc = glGetUniformLocation(s->handle, name)) == -1) {
		// TODO(cloudhead): Log error.
		return;
	}
	glUniform1i(loc, i);
}
