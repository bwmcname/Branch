#include <glsl.h>
#include <fraginc.h>

in vec4 gradient;
in vec2 frag_uv;
layout(location=DEFAULT_COLOR) out vec4 color;

void main()
{
   color = gradient;
   // color = vec4(texture(tex, frag_uv).xyz, 1.0);
}
