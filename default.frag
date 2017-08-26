#version 330 core
#include <fraginc.h>

in vec3 norm;
layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=NORMAL) out vec4 color3;
in vec3 fragPos;
uniform vec3 lightPos;
uniform vec3 diffuseColor;

void main()
{
   float distance = length(lightPos - fragPos);
   vec3 lightDir = normalize(lightPos - fragPos);
   float power = max(dot(norm, lightDir), 0.1) / distance;
   color = vec4((power * 2.0) * diffuseColor, 1.0);
   color3 = vec4(norm, 1.0);
}
