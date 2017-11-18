#include <glsl.h>
#include <vertinc.h>

uniform mat4 mvp;
uniform mat4 m;
uniform mat4 v;
layout(location=VERTEX_LOCATION) in vec4 vertex;
layout(location=NORMAL_LOCATION) in vec3 normal;

void main()
{   
   gl_Position = mvp * vertex;
}
