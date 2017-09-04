#version 330 core
#include <vertinc.h>

layout(location=VERTEX_LOCATION) in vec2 vertex;
layout(location=UV_LOCATION) in vec2 uv;

uniform mat3 transform;

out vec2 frag_uv; 

void main()
{
   frag_uv = uv;
   gl_Position = vec4((transform * vec3(vertex, 1.0)).xy, 0.0, 1.0);
}
