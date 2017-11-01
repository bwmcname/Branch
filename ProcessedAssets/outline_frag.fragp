#version 300 es

precision highp float;


#define DEFAULT_COLOR 0
#define BRIGHTNESS 1
#define NORMALS 2


#define F_EPSILON 0.1

uniform sampler2D normals;
uniform vec3 forward;

in vec2 uv;
out vec4 color;

void main()
{  
   vec3 normal = texture(normals, uv).rgb;
   if(abs(dot(forward, normal)) < F_EPSILON) color = vec4(0.1, 0.1, 0.1, 1.0);
   else discard;
}
