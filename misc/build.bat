@echo off

rem warnings wd4459 wd4456
rem set cl_flags=-nologo -MTd -Gm- -GR- -EHa- -Od -Oi -FC -Z7 -Fm -WX -W4 -wd4505 -wd4456 -wd4459 -wd4201 -wd4100 -wd4189 -DDEBUG=1

rem Optimization switches /O2 /Oi /fp:fast
rem
rem UNUTILIAZED LOCAL VARIABLES 
rem set cl_flags=-nologo -MTd -Gm- -GR- -EHa- -Od -Oi -FC -Z7 -Fm -WX -W4 -wd4101 -wd4204 -wd4505 -wd4456 -wd4459 -wd4201 -wd4100 -wd4189 -DDEBUG=1
set cl_flags=-nologo -MTd -Gm- -GR- -EHa- -Od -Oi -FC -Z7 -Fm -WX -W4 -wd4700 -wd4101 -wd4204 -wd4505 -wd4456 -wd4459 -wd4201 -wd4100 -wd4189 -DDEBUG=1
rem set clangcl_flags=-std=c99 -pedantic -MTd -GR- -EHa- -Od -Oi -fdiagnostics-absolute-paths -Z7 -WX -W4 -Wno-unused-parameter -Wno-unused-function -DDEBUG=1 -ftime-trace
set clangcl_flags=-MTd -GR- -EHa- -Od -Oi -fdiagnostics-absolute-paths -Z7 -WX -W4 -Wno-unused-variable -Wno-missing-braces -Wno-unused-parameter -Wno-unused-function -Wno-switch -DDEBUG=1 -ftime-trace
set linker_flags=-incremental:no -opt:ref
set linker_libs=user32.lib gdi32.lib winmm.lib -MAP

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
rem 32-bit build
rem cl %cl_flags% ..\code\win_platform.c /link -subsystem:windows,5.1 %linker_flags% %linker_libs%

rem 64-bit build
del *.pdb > NUL 2> NUL
rem cl %cl_flags% ..\code\game.c         -LD -link %linker_flags% -PDB:game_%random%.pdb -EXPORT:main_game_loop
rem cl %cl_flags% ..\code\win_platform.c -link  %linker_flags% %linker_libs%
clang-cl %clangcl_flags% ..\code\game.c         -LD -link %linker_flags% -PDB:game_%random%.pdb -EXPORT:main_game_loop
clang-cl %clangcl_flags% ..\code\win_platform.c     -link %linker_flags% %linker_libs%
rem -link %linker_flags% %linker_libs%
popd

rem ------ cl ------
rem -nologo - Turns off c/c++ info at the top
rem -Gm- - Turn off minimal rebuild
rem -Fm - Produces a .map file with the build that maps all functions and methods used in your program to addresses and allows you to see them (can provide name to specify name eg-> -Fmname.map)
rem -FC - Output full path of source code files passed to the compiler in diagnostics
rem -WX - Warnings cause compilation failure
rem -W4 - Show warnings of level 4 severity
rem -wd<warning_number> - ignore that warning on compilation
rem -Zi - Generate debug files information
rem -Z7 - Generate debug files information without vc120.pdb
rem -Oi - Do any intrinsic cpu operations that you can, rather than accessing the c library
rem -Od - No optimization at all
rem -O2 - Optimized (will conflict with Od)
rem -GR- - Turn off runtime type information. c++ feature
rem -EHa- - Turn off exception handling
rem -MT - Link/package c run time library into the exe. dont look for dll (allows running the program onto 32bit machines, eg- windows xp)
rem -MTd - (USE DEBUG VERSION OF THE RUN TIME LIBRARY) Link/package c run time library into the exe. dont look for dll (allows running the program onto 32bit machines, eg- windows xp)
rem -opt:ref - Dont put anything into the exe if nobody is using it
rem -incremental:no - TODO(rafik): look at what this does
rem -LD - creates a dll file when calling cl, otherwice /DLL
rem -link - links windows .lib files required for certain MSDN functions to ba ran
rem -EXPORT - TODO(rafik): not sure exactly what this does, look into it
rem -D<anything> - pass values into file

rem ------ cl ------
rem -std= - specifies the standard for the language
rem -pedantic - disables extensions (compiler specific features)
rem ------ clang-cl UNSUPPORTED EQUIVALENTS ------
rem -FC = -fdiagnostics-absolute-paths

rem TERMINAL STUFF
rem dumpbin /disasm *.[exe, obj, ...] - prints out human readable translation unit
