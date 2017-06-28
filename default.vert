#version 330 core
#include <vertinc.h>

uniform mat4 mvp;
uniform mat4 m;
uniform mat4 v;
layout(location=VERTEX_LOCATION) in vec4 vertex;
layout(location=NORMAL_LOCATION) in vec3 normal;
out vec3 norm;
out vec3 fragPos;
out vec3 light;

void main()
{
   norm = normalize(transpose(inverse(mat3(m))) * normal);
   fragPos = (m * vertex).xyz;
   gl_Position = mvp * vertex;
}
