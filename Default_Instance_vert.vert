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
   // norm = normalize(transpose(inverse(mat3(v) * mat3(am))) * normal);
   norm = transpose(inverse(mat3(am))) * normal;
   fragPos = (am * vertex).xyz;
   gl_Position = amvp * vertex;
   diffuseColor = aColor;
}
