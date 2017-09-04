#version 330 core
#include <vertinc.h>

layout(location=VERTEX_LOCATION) in vec3 vert;
layout(location=UV_LOCATION) in vec2 in_uv;
out vec2 uv;
uniform mat3 transform;

void main()
{
   uv = in_uv;
   gl_Position = vec4(transform * vert, 0.0f);
}
