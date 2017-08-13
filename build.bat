
@echo off

start /b ap shader default.vert
start /b ap shader default.frag

start /b ap shader BreakerBlock.vert
start /b ap shader BreakerBlock.frag

start /b ap shader texture.vert
start /b ap shader texture.frag

start /b ap shader bitmap_font.frag
start /b ap shader bitmap_font.vert

start /b ap shader text.vert
start /b ap shader text.frag

start /b ap shader Background.vert
start /b ap shader Background.frag

start /b ap font Dustismo_Roman.ttf_sdf.png Dustismo_Roman.ttf_sdf.txt
start /b ap bfont Dustismo_Roman.ttf 32 2048 2048

start /b ap shader ScreenTexture.vert
start /b ap shader ScreenTexture.frag

start /b ap shader ApplyBlur.frag
start /b ap shader ApplyBlur.vert

cl -Zi -DWIN32_BUILD -DDEBUG -DTIMERS -FS -Wall -WX -wd4061 -wd4062 -wd4505 -wd4623 -wd4365 -wd4244 -wd4626 -wd5027 -wd4201 -wd4820 -wd4100 -wd4514 -wd4711 include.cpp -FeBranch_Debug opengl32.lib gdi32.lib user32.lib

cl -Zi -DWIN32_BUILD -DTIMERS -Ox -FS -Wall -WX -wd4061 -wd4062 -wd4505 -wd4623 -wd4365 -wd4244 -wd4626 -wd5027 -wd4201 -wd4820 -wd4100 -wd4514 -wd4711 include.cpp -FeBranch_Release opengl32.lib gdi32.lib user32.lib
