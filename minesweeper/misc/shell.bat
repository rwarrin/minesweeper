@echo off

subst Z: /D
subst Z: E:\Dev\Projects\minesweeper

pushd Z:

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
start D:\Development\Vim\vim80\gvim.exe

cls
