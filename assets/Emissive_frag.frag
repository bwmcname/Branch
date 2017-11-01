#version 300 es

precision highp float;


#define DEFAULT_COLOR 0
#define BRIGHTNESS 1
#define NORMALS 2


layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=BRIGHTNESS) out vec4 brightness;
layout(location=NORMALS) out vec4 normal;

void main()
{
   color = vec4(5.0f, 5.0f, 5.0f, 1.0f);
   brightness = vec4(5.0f, 5.0f, 5.0f, 1.0f);
   normal = vec4(0.0f, 0.0f, 0.0f, 0.0f);
}