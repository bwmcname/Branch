
@echo off

cl -Zi -Wall -WX -wd4623 -wd4365 -wd4244 -wd4626 -wd5027 -wd4201 -wd4820 -wd4100 -wd4514 -wd4711 include.cpp -Fewin32 opengl32.lib gdi32.lib user32.lib

ap shader default.vert
ap shader default.frag

ap shader texture.vert
ap shader texture.frag

ap font FINALNEW.TTF_sdf.png FINALNEW.TTF_sdf.txt
