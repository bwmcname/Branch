#include <glsl.h>
#include <vertinc.h>

layout(location=VERTEX_LOCATION) in vec4 vert;
uniform mat4 mvp;

void main()
{
   gl_Position = mvp * vert;
}
