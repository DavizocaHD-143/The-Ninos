o off
title Voxlap .BAT
cls

set MSVC6_PATH=C:\MSVC6\VC98
set SDK_PATH=C:\Program Files\Microsoft Platform SDK for Windows XP SP2
set GAME_FILES=main.c
set EXE_NAME=TheNinos
set INCLUDE=%SDK_PATH%\include;%MSVC6_PATH%\include;%INCLUDE%
set LIB=%SDK_PATH%\lib;%MSVC6_PATH%\lib;%LIB%

::compile

del %EXE_NAME%.EXE 2>nul

cl /GX /ML /J /TP %GAME_FILES% VOXLAP5.C KPLIB.C WINMAIN.CPP /link V5.OBJ ddraw.lib dinput.lib dxguid.lib user32.lib gdi32.lib ole32.lib /OUT:%EXE_NAME%.EXE
