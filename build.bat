
@echo off

start /b ap shader default_vert.vert
start /b ap shader default_frag.frag

start /b ap shader BreakerBlock_vert.vert
start /b ap shader BreakerBlock_frag.frag

start /b ap shader texture_vert.vert
start /b ap shader texture_frag.frag

start /b ap shader bitmap_font_frag.frag
start /b ap shader bitmap_font_vert.vert

start /b ap shader text_vert.vert
start /b ap shader text_frag.frag

start /b ap shader Background_vert.vert
start /b ap shader Background_frag.frag

start /b ap font Dustismo_Roman.ttf_sdf.png Dustismo_Roman.ttf_sdf.txt
start /b ap bfont Dustismo_Roman.ttf 32 2048 2048

start /b ap shader ScreenTexture_vert.vert
start /b ap shader ScreenTexture_frag.frag

start /b ap shader ApplyBlur_frag.frag
start /b ap shader ApplyBlur_vert.vert

start /b ap shader Button_vert.vert
start /b ap shader Button_frag.frag

start /b ap image Button.png

start /WAIT ap build default_frag.fragp default_vert.vertp BreakerBlock_vert.vertp BreakerBlock_frag.fragp texture_frag.fragp texture_vert.vertp bitmap_font_vert.vertp bitmap_font_frag.fragp text_vert.vertp text_frag.fragp Background_vert.vertp Background_frag.fragp ScreenTexture_vert.vertp ScreenTexture_frag.fragp ApplyBlur_vert.vertp ApplyBlur_frag.fragp Button_vert.vertp Button_frag.fragp button

cl -Zi -DWIN32_BUILD -DDEBUG -DTIMERS -FS -Wall -WX -wd4061 -wd4062 -wd4505 -wd4623 -wd4365 -wd4244 -wd4626 -wd5027 -wd4201 -wd4820 -wd4100 -wd4514 -wd4711 include.cpp -FeBranch_Debug opengl32.lib gdi32.lib user32.lib

cl -Zi -DWIN32_BUILD -DTIMERS -Ox -FS -Wall -WX -wd4061 -wd4062 -wd4505 -wd4623 -wd4365 -wd4244 -wd4626 -wd5027 -wd4201 -wd4820 -wd4100 -wd4514 -wd4711 include.cpp -FeBranch_Release opengl32.lib gdi32.lib user32.lib
