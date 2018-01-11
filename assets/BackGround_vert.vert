#include <glsl.h>
#include <vertinc.h>

layout(location=VERTEX_LOCATION) in vec4 vert;
layout(location=UV_LOCATION) in vec2 uvs;

uniform vec3 color1;
uniform vec3 color2;
out vec4 gradient;
out vec2 frag_uvs;

void main()
{
   if(vert.y > 0.0)
   {
      gradient = vec4(color2, 1.0);
   }
   else
   {
      //rgb(24, 84, 39)
      gradient = vec4(color1, 1.0);
   }

   frag_uvs = uvs;
   gl_Position = vert;
}
