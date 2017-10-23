#include <glsl.h>

in vec2 tex_uv;
uniform sampler2D tex;

out vec4 color;

#define smoothing (4.0f / 16.0f)

void main()
{
   float distance = texture2D(tex, tex_uv).a;
   float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
   color = vec4(1, 1, 1, alpha);   
}
