#include <glsl.h>

in vec2 frag_uv;
uniform sampler2D tex;
uniform vec3 color;

out vec4 pixel;

void main()
{
   pixel = vec4(color, texture(tex, frag_uv).a);
}
