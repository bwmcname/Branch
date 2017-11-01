#include <glsl.h>
#include <vertinc.h>

layout(location=MATRIX1_LOCATION) in mat4 amvp;
layout(location=COLOR_INPUT_LOCATION) in vec3 aColor;
layout(location=VERTEX_LOCATION) in vec2 vertex;

out vec3 color;

void main()
{
   gl_Position = amvp * vec4(vertex, 0.0, 1.0);
   color = aColor;
}
