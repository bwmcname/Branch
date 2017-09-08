#version 330 core
#include <fraginc.h>
#include <light.h>

in vec3 norm;
layout(location=DEFAULT_COLOR) out vec4 color;
in vec3 fragPos;
uniform vec3 lightPos;
uniform vec3 diffuseColor;

void main()
{
   light(norm, fragPos, diffuseColor, lightPos, color);
}
