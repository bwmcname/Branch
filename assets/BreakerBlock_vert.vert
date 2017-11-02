#include <glsl.h>
#include <vertinc.h>

layout(location=VERTEX_LOCATION) in vec4 vert;
layout(location=UV_LOCATION) in vec2 uv;
out vec2 out_uv;

uniform mat4 mvp;

void main()
{
   out_uv = uv;
   gl_Position = mvp * vert;
}
