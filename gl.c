#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <GL/glew.h>

#include "linmath.h"
#include "gl.h"
#include "util.h"

static void initialize_debug_callback(void);

void _gl_errors(const char *file, int line)
{
	GLenum err;

	while ((err = glGetError()) != GL_NO_ERROR) {
		char *error = NULL;

		switch (err) {
		case GL_INVALID_OPERATION:              error = "INVALID_OPERATION";              break;
		case GL_INVALID_ENUM:                   error = "INVALID_ENUM";                   break;
		case GL_INVALID_VALUE:                  error = "INVALID_VALUE";                  break;
		case GL_OUT_OF_MEMORY:                  error = "OUT_OF_MEMORY";                  break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
		}
		fprintf(stderr, "%s:%d: GL(error): %s\n", file, line, error);
	}
}

inline void gl_clear(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void gl_viewport(int w, int h)
{
	glViewport(0, 0, w, h);
}

inline void gl_draw_triangles(int attrib, int nverts, int arity)
{
	gl_draw_triangles_blend(attrib, nverts, arity, vec4(0, 0, 0, 0), GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

inline void gl_draw_triangles_blend(int attrib, int nverts, int arity, vec4_t bcolor, GLenum sfactor, GLenum dfactor)
{
	glEnable(GL_BLEND);
	glBlendColor(bcolor.r, bcolor.g, bcolor.b, bcolor.a);
	glBlendFunc(sfactor, dfactor);
	glEnableVertexAttribArray(attrib);
	glVertexAttribPointer(
		attrib,                                       // Vertex attribute location
		arity,                                        // Number of vertex components
		GL_FLOAT,                                     // Type
		GL_FALSE,                                     // Normalized?
		0,                                            // Stride
		(void*)0                                      // Offset
	);
	glDrawArrays(GL_TRIANGLES, 0, nverts);                // Draw from vertex 0 to N
	glDisableVertexAttribArray(attrib);
	glDisable(GL_BLEND);
}

void gl_init(int w, int h, bool debug)
{
	glewExperimental = true;
	glewInit();

	glViewport(0, 0, w, h);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (debug)
		initialize_debug_callback();
}

static void debug_callback(
	GLenum        source,
	GLenum        type,
	GLuint        id,
	GLenum        severity,
	GLsizei       length,
	const GLchar *message,
	const void   *user_param
) {
	char *stype = NULL, *sseverity = NULL, *ssource = NULL;

	switch (source) {
		case GL_DEBUG_SOURCE_API:             ssource = "API";             break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   ssource = "WINDOW SYSTEM";   break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: ssource = "SHADER COMPILER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     ssource = "THIRD PARTY";     break;
		case GL_DEBUG_SOURCE_APPLICATION:     ssource = "APPLICATION";     break;
		case GL_DEBUG_SOURCE_OTHER:           ssource = "OTHER";           break;
	}

	switch (type) {
		case GL_DEBUG_TYPE_ERROR:               stype = "ERROR";               break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: stype = "DEPRECATED_BEHAVIOR"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  stype = "UNDEFINED_BEHAVIOR";  break;
		case GL_DEBUG_TYPE_PORTABILITY:         stype = "PORTABILITY";         break;
		case GL_DEBUG_TYPE_PERFORMANCE:         stype = "PERFORMANCE";         return; // Don't care about performance for now
		case GL_DEBUG_TYPE_OTHER:               stype = "OTHER";               return; // Don't care about other.
	}

	switch (severity) {
		case GL_DEBUG_SEVERITY_LOW:    sseverity = "LOW";    break;
		case GL_DEBUG_SEVERITY_MEDIUM: sseverity = "MEDIUM"; break;
		case GL_DEBUG_SEVERITY_HIGH:   sseverity = "HIGH";   break;
	}

	infof("gl", "GL(debug, source=%s, severity=%s, id=%d, type=%s): %s", ssource, sseverity, id, stype, message);
}

static void initialize_debug_callback(void)
{
	if (glDebugMessageCallback) {
		info("gl", "registering debug callback");

		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(debug_callback, NULL);

		GLuint unused_ids = 0;
		glDebugMessageControl(
			GL_DONT_CARE,
			GL_DONT_CARE,
			GL_DONT_CARE,
			0,
			&unused_ids,
			true
		);
	}
}
