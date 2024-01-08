@echo off

set CommonCompilerFlags= -O2 -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -DVIEW_TILEMAP_INTERNAL=1 -DVIEW_TILEMAP_SLOW=1 -DVIEW_TILEMAP_WIN32=1 -DVIEW_TILEMAP_LAPTOP=1 -DDRAW=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

REM TODO - can we just build both with one exe?

IF NOT EXIST ..\..\build\build_tilemap mkdir ..\..\build\build_tilemap
pushd ..\..\build\build_tilemap

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
del *.pdb > NUL 2> NUL
REM Optimization switches /O2
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% ..\..\Tile_Map_Editor\code\view_tilemap.cpp -Fmview_tilemap.map -LD /link -incremental:no -opt:ref -PDB:view_tilemap_%random%.pdb -EXPORT:GameUpdateAndRender
del lock.tmp
cl %CommonCompilerFlags% ..\..\Tile_Map_Editor\code\win32_view_tilemap.cpp -Fmwin32_view_tilemap.map /link %CommonLinkerFlags%
cl %CommonCompilerFlags% -D_CRT_SECURE_NO_WARNINGS ..\..\Tile_Map_Editor\code\create_tilemap.cpp -Fmcreate_tilemap.map /link %CommonLinkerFlags%
popd
