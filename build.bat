
@echo off

cl -Zi -Wall -WX -wd4623 -wd4626 -wd5027 -wd4201 -wd4820 -wd4100 -wd4514 -wd4711 include.cpp -Fewin32 opengl32.lib gdi32.lib user32.lib

ap shader default.vert
ap shader default.frag
