#include <glsl.h>
#include <vertinc.h>

layout(location=VERTEX_LOCATION) in vec4 vert;
layout(location=UV_LOCATION) in vec2 in_uv;

out vec2 uv;

void main()
{   
   gl_Position = vert;
   uv = in_uv;
}
