@echo off

set CommonCompilerFlags=-MTd -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4  -wd4201 -wd4456 -wd4100 -wd4189 -wd4505 -DCREATE_TILEMAP_INTERNAL=1 -DCREATE_TILEMAP_SLOW=1 -DCREATE_TILEMAP_WIN32=1 -DLAPTOP=1 -DDRAW=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\..\build_tilemap mkdir ..\..\build_tilemap
pushd ..\..\build_tilemap

REM 64-bit build
del *.pdb > NUL 2> NUL
REM Optimization switches /O2
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% ..\code\Cpp\create_tilemap.cpp -Fmcreate_tilemap_.map -LD /link -incremental:no -opt:ref -PDB:create_tilemap_%random%.pdb -EXPORT:GameUpdateAndRender
del lock.tmp
cl %CommonCompilerFlags% ..\code\Cpp\win32_create_tilemap.cpp -Fmwin32_create_tilemap.map /link %CommonLinkerFlags%
popd
