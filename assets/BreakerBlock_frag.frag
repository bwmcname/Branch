#include <glsl.h>
#include <fraginc.h>

layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=BRIGHTNESS) out vec4 color2;
uniform sampler2D tex;

in vec2 out_uv;

void main()
{
   vec4 value = texture(tex, out_uv);
   color = value;
   color2 = vec4(value.r * 1.0, 0.25 * value.r, 0.0, 1.0);
}
