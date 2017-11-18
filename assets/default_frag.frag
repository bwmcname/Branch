#include <glsl.h>
#include <fraginc.h>
#include <light.h>

in vec3 norm;
layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=BRIGHTNESS) out vec4 color2;
layout(location=2) out vec4 color3;
in vec3 fragPos;
uniform vec3 lightPos;
uniform vec3 diffuseColor;

void main()
{
   light(norm, fragPos, diffuseColor, lightPos, color);
   color2 = vec4(0.0, 0.0, 0.0, 1.0);
   color3 = vec4(norm, 1.0);
}
