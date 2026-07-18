o off
title Server .BAT
cls

set MSVC6_PATH=C:\MSVC6\VC98
set SDK_PATH=C:\Program Files\Microsoft Platform SDK for Windows XP SP2
set GAME_FILES=main.c
set EXE_NAME=TheNinosServer
set INCLUDE=%SDK_PATH%\include;%MSVC6_PATH%\include;include;%INCLUDE%
set LIB=%SDK_PATH%\lib;%MSVC6_PATH%\lib;%LIB%

::compile

del %EXE_NAME%.EXE 2>nul

cl /GX /ML /J /TP %GAME_FILES% /link ENET.lib ws2_32.lib winmm.lib /OUT:%EXE_NAME%.EXE
