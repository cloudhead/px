#version 330 core

in      vec4       multiply;
in      vec2       texCoord;
out     vec4       fragColor;

uniform sampler2D  sampler;

void main()
{
	vec2 v = vec2(texCoord.s, 1 - texCoord.t);
	fragColor = texture(sampler, v) * multiply;
}
