#include <glsl.h>

in vec2 frag_uv;
uniform sampler2D tex;

out vec4 color;

void main()
{
   color = vec4(0, 1, 0, texture(tex, frag_uv).a);
}
