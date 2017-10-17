
@echo off

ap shader default_vert.vert
ap shader default_frag.frag

ap shader BreakerBlock_vert.vert
ap shader BreakerBlock_frag.frag

ap shader texture_vert.vert
ap shader texture_frag.frag

ap shader bitmap_font_frag.frag
ap shader bitmap_font_vert.vert

ap shader text_vert.vert
ap shader text_frag.frag

ap shader Background_vert.vert
ap shader Background_frag.frag

ap font Dustismo_Roman.ttf_sdf.png Dustismo_Roman.ttf_sdf.txt
ap bfont Dustismo_Roman.ttf 32 2048 2048

ap shader ScreenTexture_vert.vert
ap shader ScreenTexture_frag.frag

ap shaderapplyBlur_frag.frag
ap shaderapplyBlur_vert.vert

ap shader Button_vert.vert
ap shader Button_frag.frag

ap shader Default_Instance_vert.vert
ap shader Default_Instance_frag.frag

ap shader Emissive_vert.vert
ap shader Emissive_frag.frag

ap shader outline_frag.frag

ap image Button.png

ap build default_frag.fragp default_vert.vertp BreakerBlock_vert.vertp BreakerBlock_frag.fragp texture_frag.fragp texture_vert.vertp bitmap_font_vert.vertp bitmap_font_frag.fragp text_vert.vertp text_frag.fragp Background_vert.vertp Background_frag.fragp ScreenTexture_vert.vertp ScreenTexture_frag.fragp ApplyBlur_vert.vertp ApplyBlur_frag.fragp Button_vert.vertp Button_frag.fragp button Default_Instance_vert.vertp Default_Instance_frag.fragp Emissive_vert.vertp Emissive_frag.fragp outline_frag.fragp sphere.brian

cl -Zi -DWIN32_BUILD -DDEBUG -DTIMERS -FS -Wall -WX  -wd4005 -wd4996 -wd4061 -wd4062 -wd4505 -wd4623 -wd4365 -wd4244 -wd4626 -wd5027 -wd4201 -wd4820 -wd4100 -wd4514 -wd4711 include.cpp -FeBranch_Debug opengl32.lib gdi32.lib user32.lib

cl -Zi -DWIN32_BUILD -DTIMERS -Ox -FS -Wall -WX -wd4996 -wd4005 -wd4061 -wd4062 -wd4505 -wd4623 -wd4365 -wd4244 -wd4626 -wd5027 -wd4201 -wd4820 -wd4100 -wd4514 -wd4711 include.cpp -FeBranch_Release opengl32.lib gdi32.lib user32.lib
