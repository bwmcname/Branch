#version 300 es

precision highp float;


#define DEFAULT_COLOR 0
#define BRIGHTNESS 1
#define NORMALS 2


in vec4 gradient;
layout(location=DEFAULT_COLOR) out vec4 color;
// layout(location=BRIGHTNESS) out vec4 color2;
// layout(location=NORMALS) out vec4 normals;

void main()
{
   // apply gamma so that we can have the smooth unrealistic gradient
   color = gradient;
   // color2 = vec4(0.0, 0.0, 0.0, 1.0);
   // normals = vec4(1000.0f, 1000.0f, 1000.0f, 1.0f);
}