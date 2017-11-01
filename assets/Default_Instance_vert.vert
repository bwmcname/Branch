#version 300 es

precision highp float;


#define VERTEX_LOCATION 1
#define NORMAL_LOCATION 2
#define COLOR_INPUT_LOCATION 3
#define UV_LOCATION 4
#define MATRIX1_LOCATION 5 // AND 6 7 8
#define MATRIX2_LOCATION 9 // AND 10 11 12


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