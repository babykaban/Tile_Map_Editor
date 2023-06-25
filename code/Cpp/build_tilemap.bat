@echo off

set CommonCompilerFlags= -MT -nologo -GR- -Gm- -EHa- -Od -Oi -WX -W4 -wd4456 -wd4201 -wd4100 -wd4189 -wd4996 -FC -Z7 -DCHECK_TIME=1
set CommonLinkerFlags= -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\..\build_tilemap mkdir ..\..\build_tilemap
pushd ..\..\build_tilemap

cl %CommonCompilerFlags% ..\code\Cpp\win32_create_tilemap.cpp /link %CommonLinkerFlags%
popd
