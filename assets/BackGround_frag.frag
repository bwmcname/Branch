#include <glsl.h>
#include <fraginc.h>

in vec4 gradient;
layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=BRIGHTNESS) out vec4 color2;

void main()
{
   // apply gamma so that we can have the smooth unrealistic gradient
   // color = vec4(pow(gradient.xyz, vec3(2.2)), 1.0);
   color = gradient;
   // color2 = vec4(0.0, 0.0, 0.0, 0.0);
}
