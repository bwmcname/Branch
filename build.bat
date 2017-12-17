
@echo off
pushd assets

rem calls to asset packer to build assets
..\ap shader default_vert.vert
..\ap shader default_frag.frag

..\ap shader BreakerBlock_vert.vert
..\ap shader BreakerBlock_frag.frag

..\ap shader texture_vert.vert
..\ap shader texture_frag.frag

..\ap font c:/Windows/Fonts/arial.ttf 1024 1024 16

..\ap shader text_vert.vert
..\ap shader text_frag.frag

..\ap shader Background_vert.vert
..\ap shader Background_frag.frag

..\ap shader ScreenTexture_vert.vert
..\ap shader ScreenTexture_frag.frag

..\ap shader applyBlur_frag.frag
..\ap shader applyBlur_vert.vert

..\ap shader Button_vert.vert
..\ap shader Button_frag.frag

..\ap shader Default_Instance_vert.vert
..\ap shader Default_Instance_frag.frag

..\ap shader Emissive_vert.vert
..\ap shader Emissive_frag.frag

..\ap shader bitmap_font_vert.vert
..\ap shader bitmap_font_frag.frag

..\ap shader outline_frag.frag

..\ap shader fast_blur_frag.frag

..\ap image Button.png

..\ap image Block.png

..\ap map 2048 2048 Button.png Block.png

rem pack asset file
..\ap build default_frag.fragp default_vert.vertp BreakerBlock_vert.vertp BreakerBlock_frag.fragp texture_frag.fragp texture_vert.vertp bitmap_font_vert.vertp bitmap_font_frag.fragp text_vert.vertp text_frag.fragp Background_vert.vertp Background_frag.fragp ScreenTexture_vert.vertp ScreenTexture_frag.fragp ApplyBlur_vert.vertp ApplyBlur_frag.fragp Button_vert.vertp Button_frag.fragp button Block Default_Instance_vert.vertp Default_Instance_frag.fragp Emissive_vert.vertp Emissive_frag.fragp outline_frag.fragp fast_blur_frag.fragp sphere.brian wow.font
popd
cl -Zi -DWIN32_BUILD -DDEBUG -DTIMERS -FS -Wall -WX  -wd4005 -wd4996 -wd4061 -wd4062 -wd4505 -wd4623 -wd4365 -wd4244 -wd4626 -wd5027 -wd4201 -wd4820 -wd4100 -wd4514 -wd4711 Include.cpp -FeBranch_Debug opengl32.lib gdi32.lib user32.lib

cl -Zi -DWIN32_BUILD -DTIMERS -Ox -FS -Wall -WX -wd4996 -wd4005 -wd4061 -wd4062 -wd4505 -wd4623 -wd4365 -wd4244 -wd4626 -wd5027 -wd4201 -wd4820 -wd4100 -wd4514 -wd4711 Include.cpp -FeBranch_Release opengl32.lib gdi32.lib user32.lib
