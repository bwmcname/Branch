#version 330 core
#include <fraginc.h>

uniform sampler2D scene;
uniform sampler2D blur;

in vec2 out_uv;
out vec4 color;

void main()
{ 
   color = vec4(texture2D(blur, out_uv).rgb + texture2D(scene, out_uv).rgb, 1.0);
   //color = vec4(texture2D(blur, out_uv).rgb, 1.0);
}
