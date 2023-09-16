@echo off

set CommonCompilerFlags=-MTd -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4996 -wd4201 -wd4456 -wd4100 -wd4189 -wd4505 -DVIEW_TILEMAP_INTERNAL=1 -DVIEW_TILEMAP_SLOW=1 -DVIEW_TILEMAP_WIN32=1 -DLAPTOP=1 -DDRAW=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\..\build_tilemap mkdir ..\..\build_tilemap
pushd ..\..\build_tilemap

REM 64-bit build
del *.pdb > NUL 2> NUL
REM Optimization switches /O2
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% ..\code\Cpp\view_tilemap.cpp -Fmview_tilemap.map -LD /link -incremental:no -opt:ref -PDB:view_tilemap_%random%.pdb -EXPORT:GameUpdateAndRender
del lock.tmp
cl %CommonCompilerFlags% ..\code\Cpp\win32_view_tilemap.cpp -Fmwin32_view_tilemap.map /link %CommonLinkerFlags%

cl %CommonCompilerFlags% ..\code\Cpp\create_tilemap.cpp -Fmcreate_tilemap.map /link %CommonLinkerFlags%

popd
