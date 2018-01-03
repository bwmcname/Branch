#include <glsl.h>
#include <vertinc.h>

layout(location=VERTEX_LOCATION) in vec4 vert;
layout(location=UV_LOCATION) in vec2 uv;
layout(location=MATRIX1_LOCATION) in mat4 mvp;
out vec2 out_uv;

void main()
{
   out_uv = uv;
   gl_Position = mvp * vert;
}
