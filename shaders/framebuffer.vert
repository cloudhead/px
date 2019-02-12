#version 330 core

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 color;

out vec2 texCoord;
out vec4 multiply;

void main()
{
	gl_Position = vec4(vertex.xy, 0.0, 1.0);
	texCoord    = vertex.zw;
	multiply    = color;
}
