#include <glsl.h>

out vec4 color;
in vec2 uv;
uniform sampler2D image2;

uniform float xstep;
uniform float ystep;

float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
   vec4 result = texture(image2, uv) * weight[0];
   
   for(int i = 1; i < 5; ++i)
   {
      float deltaX = (float(i) * xstep);
      float deltaY = (float(i) * ystep);
      result += texture(image2, vec2(min(uv.x + deltaX, 1.0), min(uv.y + deltaY, 1.0))) * weight[i];
      result += texture(image2, vec2(max(uv.x - deltaX, 0.0), max(uv.y - deltaY, 0.0))) * weight[i];
   }
      
   color = result;

   result = texture(image2, uv);
}
