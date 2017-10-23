#include <glsl.h>
#include <fraginc.h>

in vec4 gradient;
// layout(location=DEFAULT_COLOR) out vec4 color;
// layout(location=BRIGHTNESS) out vec4 color2;
// layout(location=NORMALS) out vec4 normals;
out vec4 color;

void main()
{
   // apply gamma so that we can have the smooth unrealistic gradient
   color = vec4(pow(gradient.xyz, vec3(2.2)), 1.0);
   // color2 = vec4(0.0, 0.0, 0.0, 1.0);
   // normals = vec4(1000.0f, 1000.0f, 1000.0f, 1.0f);
}
