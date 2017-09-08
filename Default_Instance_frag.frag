#version 330 core
#include <fraginc.h>
#include <light.h>

in vec3 norm;
in vec3 fragPos;
in vec3 diffuseColor;
layout(location=DEFAULT_COLOR) out vec4 color;
uniform vec3 lightPos;

void main()
{
   light(norm, fragPos, diffuseColor, lightPos, color);
}
