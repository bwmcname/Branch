#include <glsl.h>
#include <fraginc.h>

layout(location=DEFAULT_COLOR) out vec4 texel;
uniform sampler2D tex;
uniform vec3 color;

in vec2 out_uv;

void main()
{
   vec4 value = texture(tex, out_uv);
   texel = vec4(value.r * color, value.a);
}
