#include <glsl.h>

out vec4 color;
in vec2 uv;
uniform sampler2D image1;
uniform sampler2D image2;

void main()
{
   color = texture(image1, uv) + texture(image2, uv);
   
   // color = texture(image2, uv);
}
