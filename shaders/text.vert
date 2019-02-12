#version 330 core

uniform mat4 ortho;

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 _multiply;

out vec2 texCoord;
out vec4 multiply;

void main()
{
	gl_Position = ortho * vec4(vertex.xy, 0.0, 1.0);
	texCoord    = vertex.zw;
	multiply    = vec4(1, 1, 1, 1);
}
