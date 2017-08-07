#version 330 core
#include <fraginc.h>

in vec3 norm;
layout(location=DEFAULT_COLOR) out vec4 color;
layout(location=BRIGHTNESS) out vec4 color2;
in vec3 fragPos;
uniform vec3 lightPos;
uniform vec3 diffuseColor;

void main()
{
   float distance = length(lightPos - fragPos);
   vec3 lightDir = normalize(lightPos - fragPos);
   float power = max(dot(norm, lightDir), 0.05); 
   color = vec4((power * 2.0) * diffuseColor, 1.0);

   
   if(power > 0.8)
   {
      color2 = color;
   }
   else
   {
      color2 = vec4(0.0, 0.0, 0.0, 0.0);
   }
   

   //color = vec4(diffuseColor, 1.0);
   //color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
