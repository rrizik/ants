@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

doskey handmade=cd C:\sh1tz\apesticks\cc++\handmade
set path=C:\sh1tz\apesticks\cc++\handmade\misc;%path%

doskey rafikmade=cd C:\sh1tz\apesticks\cc++\rafikmade
set path=C:\sh1tz\apesticks\cc++\rafikmade\misc;%path%

doskey raster=cd C:\sh1tz\apesticks\cc++\rasterizer
set path=C:\sh1tz\apesticks\cc++\rasterizer\misc;%path%

doskey gvim="C:\Program Files (x86)\Vim\vim81\gvim.exe"
doskey rem="C:\Program Files (x86)\sh1tz\programs\remedybg\remedybg.exe"

rem doskey devenv=devenv C:\sh1tz\apesticks\dx11\handmade\build\win32_handmade.exe
rem doskey gvim=start /max "" "C:\Program Files (x86)\Vim\vim81\gvim.exe"
