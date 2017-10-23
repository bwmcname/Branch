#include <glsl.h>
#include <vertinc.h>

layout(location=VERTEX_LOCATION) in vec4 vert;
out vec4 gradient;

void main()
{
   if(vert.y > 0.0)
   {
      gradient = vec4(0.0, 0.0, 0.0, 1.0);
   }
   else
   {
      gradient = vec4(25.0 / 255.0, 81.0 / 255.0, 98.0 / 255.0, 1.0);
   }
   
   gl_Position = vert;
}
