#version 330 core
#include <vertinc.h>

uniform mat4 v;
layout(location=VERTEX_LOCATION) in vec4 vertex;
layout(location=NORMAL_LOCATION) in vec3 normal;
layout(location=COLOR_INPUT_LOCATION) in vec3 aColor;
layout(location=MATRIX1_LOCATION) in mat4 amvp;
layout(location=MATRIX2_LOCATION) in mat4 am;
out vec3 fragPos;
out vec3 norm;
out vec3 diffuseColor;

void main()
{
   norm = (transpose(inverse(am)) * vec4(normal, 0.0)).xyz;
   fragPos = (am * vertex).xyz;
   gl_Position = amvp * vertex;
   diffuseColor = aColor;
}
