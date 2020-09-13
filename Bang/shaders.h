#pragma once

const char* FRAGMENT_SHADER = R"(
#version 410 core

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 uvCoords;

layout(location = 0) out vec4 color;

uniform sampler2D mainTex;
uniform bool font;

void main()
{
	vec4 t = texture(mainTex, uvCoords);
	if(font)
	{	
		color = vec4(1, 1, 1, t.r) * fragColor;
	}
	else
	{
		color = t * fragColor;
	}
	if(color.a < 0.1) discard;
}
)";

const char* VERTEX_SHADER = R"(
#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 uvCoords;

uniform mat4 VP;
uniform mat4 M;

void main()
{
	gl_Position = VP * M * vec4(position, 1.0);
	
	fragColor = color;
	uvCoords = uv;
}
)";




const char* PARTICLE_FRAGMENT_SHADER = R"(
#version 410 core

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 uvCoords;

layout(location = 0) out vec4 color;

uniform sampler2D mainTex;

void main()
{
	color = fragColor * texture(mainTex, uvCoords);
}
)";

const char* PARTICLE_VERTEX_SHADER = R"(
#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 xyzs;
layout(location = 2) in vec4 color;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 uvCoords;

uniform mat4 VP;

void main()
{
	float size = xyzs.w;
	vec3 center = xyzs.xyz;
	
	vec3 position_worldspace = center 
			+ vec3(1, 0, 0) * position.x * size
			+ vec3(0, 1, 0) * position.y * size;
			
	gl_Position = VP * vec4(position_worldspace, 1.0f);
	
	fragColor = color;
	uvCoords = position.xy + vec2(0.5, 0.5);
}
)";

const char* FRAGMENT_HIGHLIGHT_SHADER = R"(
#version 410 core

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 uvCoords;

layout(location = 0) out vec4 color;

uniform sampler2D mainTex;

void main()
{
	vec4 t = texture(mainTex, uvCoords);
	if(t.a < 0.1) discard;
	color = vec4(1, 0, 0, 1);
}
)";