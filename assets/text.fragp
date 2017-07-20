#version 330 core

in vec2 frag_uv;
uniform sampler2D tex;

out vec4 color;

#define smoothing (2.0/8.0)

void main()
{
   float distance = texture2D(tex, frag_uv).a;
   float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
   color = vec4(0, 1, 0, alpha);
}
