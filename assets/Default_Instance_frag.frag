#include <glsl.h>
#include <fraginc.h>
#include <light.h>

in vec3 norm;
in vec3 fragPos;
in vec3 diffuseColor;
layout(location=DEFAULT_COLOR) out vec4 color;
uniform vec3 lightPos;

void main()
{
   color = vec4(diffuseColor, 1.0);
   // light(norm, fragPos, diffuseColor, lightPos, color);
}
