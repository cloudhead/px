#version 330 core

uniform mat4 ortho;
uniform mat4 transform;

layout(location = 0) in vec2 vertex;

void main()
{
	gl_Position = ortho * transform * vec4(vertex.xy, 0.0, 1.0);
}
