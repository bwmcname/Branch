#version 300 es

precision highp float;


#define VERTEX_LOCATION 1
#define NORMAL_LOCATION 2
#define COLOR_INPUT_LOCATION 3
#define UV_LOCATION 4
#define MATRIX1_LOCATION 5 // AND 6 7 8
#define MATRIX2_LOCATION 9 // AND 10 11 12


layout(location=VERTEX_LOCATION) in vec4 vert;
layout(location=UV_LOCATION) in vec2 in_uv;

out vec2 uv;

void main()
{   
   gl_Position = vert;
   uv = in_uv;
}
