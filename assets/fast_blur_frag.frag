#include <glsl.h>
#include <fraginc.h>

in vec2 uv;
out vec4 color;
uniform sampler2D bright;
uniform float xOff;
uniform float yOff;

void main()
{
   vec4 result;
   
   result = texture(bright, uv + vec2(-xOff, yOff)) * 0.077847;
   result += texture(bright, uv + vec2(0.0, yOff)) * 0.123317;
   result += texture(bright, uv + vec2(xOff, yOff)) * 0.077847;

   result += texture(bright, uv + vec2(-xOff, 0.0)) * 0.123317;
   result += texture(bright, uv + vec2(0.0, 0.0)) * 0.195346;
   result += texture(bright, uv + vec2(xOff, 0.0)) * 0.123317;

   result += texture(bright, uv + vec2(-xOff, -yOff)) * 0.077847;
   result += texture(bright, uv + vec2(0.0, -yOff)) * 0.123317;
   result += texture(bright, uv + vec2(xOff, -yOff)) * 0.077847;

   color = result;
}
