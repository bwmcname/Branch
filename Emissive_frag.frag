#version 330 core
#include <fraginc.h>

layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=BRIGHTNESS) out vec4 brightness;

void main()
{
   color = vec4(5.0f, 5.0f, 5.0f, 1.0f);
   brightness = vec4(5.0f, 5.0f, 5.0f, 1.0f);
}
