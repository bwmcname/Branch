#include <glsl.h>
#include <fraginc.h>

uniform sampler2D tex;
in vec2 out_uv;

layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=BRIGHTNESS) out vec4 color2;

void main()
{
   color = texture(tex, out_uv);
   color2 = vec4(4.0f, 0.0, 1.0, 1.0);
}
