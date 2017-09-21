#version 330 core

in vec2 frag_uv;
uniform sampler2D tex;

out vec4 color;

void main()
{
   color = vec4(0, 1, 0, texture2D(tex, frag_uv).r);
}