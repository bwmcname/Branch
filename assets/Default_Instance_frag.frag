#version 300 es

precision highp float;


#define DEFAULT_COLOR 0
#define BRIGHTNESS 1
#define NORMALS 2


#define light(normal, fragPos, diffuseColor, lightPos, outColor)	\
   float distance = length(lightPos - fragPos);				\
   vec3 lightDir = normalize(lightPos - fragPos);			\
   float power = max(dot(normal, lightDir), 0.0) / distance;		\
   vec3 ambient = 0.2 * diffuseColor;					\
   outColor = vec4(((power * 2.0) * diffuseColor) + ambient, 1.0)


in vec3 norm;
in vec3 fragPos;
in vec3 diffuseColor;
layout(location=DEFAULT_COLOR) out vec4 color;
// layout(location=BRIGHTNESS) out vec4 color2;
// layout(location=NORMALS) out vec4 normal;
uniform vec3 lightPos;

void main()
{
   light(norm, fragPos, diffuseColor, lightPos, color);
   // color2 = vec4(0.0f, 0.0f, 0.0f, 1.0f);
   // normal = vec4(norm, 1.0f);
}