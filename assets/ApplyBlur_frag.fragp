#version 300 es

precision highp float;


#define DEFAULT_COLOR 0
#define BRIGHTNESS 1
#define NORMALS 2


uniform sampler2D scene;
uniform sampler2D blur;

in vec2 out_uv;
out vec4 color;

void main()
{ 
   color = vec4(texture(blur, out_uv).rgb + texture(scene, out_uv).rgb, 1.0);
}
