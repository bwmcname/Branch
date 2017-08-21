#version 330 core
#include <fraginc.h>

layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=BRIGHTNESS) out vec4 color2;

void main()
{
   color = vec4(3.0, 3.0, 3.0, 1.0);
   color2 = vec4(3.0f, 3.0, 3.0, 1.0);
}
