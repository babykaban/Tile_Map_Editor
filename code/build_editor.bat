@echo off

set CommonCompilerFlags= -Od -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -DEDITOR_INTERNAL=1 -DEDITOR_SLOW=1 -DEDITOR_WIN32=1 -DEDITOR_LAPTOP=1 -DDRAW=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib

REM TODO - can we just build both with one exe?

IF NOT EXIST ..\..\build\build_editor mkdir ..\..\build\build_editor
pushd ..\..\build\build_editor

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
del *.pdb > NUL 2> NUL
REM Optimization switches /O2
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% ..\..\Tile_Map_Editor\code\editor.cpp -Fmeditor.map -LD /link -incremental:no -opt:ref -PDB:editor_%random%.pdb -EXPORT:GameUpdateAndRender
del lock.tmp
cl %CommonCompilerFlags% ..\..\Tile_Map_Editor\code\win32_editor.cpp -Fmwin32_editor.map /link %CommonLinkerFlags%
popd
