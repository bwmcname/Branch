#version 330 core
#include <vertinc.h>

layout(location=VERTEX_LOCATION) in vec2 vert;
layout(location=UV_LOCATION) in vec2 uv;

out vec2 tex_uv;

void main()
{
   tex_uv = uv;
   gl_Position = vec4(vert, 0.0f, 1.0f);
}
