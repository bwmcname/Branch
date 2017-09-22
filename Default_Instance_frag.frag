#version 330 core
#include <fraginc.h>
#include <light.h>

in vec3 norm;
in vec3 fragPos;
in vec3 diffuseColor;
layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=BRIGHTNESS) out vec4 color2;
uniform vec3 lightPos;

void main()
{
   light(norm, fragPos, diffuseColor, lightPos, color);
   color2 = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
