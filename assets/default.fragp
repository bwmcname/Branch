#version 330 core

in vec3 norm;
out vec4 color;
in vec3 fragPos;
uniform vec3 lightPos;
uniform vec3 diffuseColor;

void main()
{
   float distance = length(lightPos - fragPos);
   vec3 lightDir = normalize(lightPos - fragPos);
   float power = max(dot(norm, lightDir), 0.05); 
   color = vec4(power * diffuseColor, 1.0);
   //color = vec4(diffuseColor, 1.0);
   //color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
