#include <glsl.h>
#include <fraginc.h>

layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=BRIGHTNESS) out vec4 color2;

void main()
{
   color = vec4(3.0, 3.0, 3.0, 1.0);
   color2 = vec4(4.0f, 0.0, 1.0, 1.0);
}
