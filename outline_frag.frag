#version 330 core
#include <fraginc.h>

#define F_EPSILON 0.1

uniform sampler2D normals;
uniform vec3 forward;

in vec2 uv;
out vec4 color;

void main()
{  
   vec3 normal = texture2D(normals, uv).rgb;
   if(abs(dot(forward, normal)) < F_EPSILON) color = vec4(0.1, 0.1, 0.1, 1.0);
   else discard;
}
