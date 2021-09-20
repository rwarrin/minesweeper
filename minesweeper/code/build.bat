@echo off

IF NOT EXIST build mkdir build

pushd build

SET CompilerFlags=-nologo -Z7 -Od -Oi -fp:fast -FC -diagnostics:classic -wd5045
SET LinkerFlags=-incremental:no

del *.pdb > NUL 2> NUL

cl.exe %CompilerFlags% ..\minesweeper\code\asset_pipeline.cpp /link %LinkerFlags%

echo WAITING FOR PDB > lock.tmp
cl.exe %CompilerFlags% -LD ..\minesweeper\code\minesweeper.cpp /link /PDB:minsweeper_%random%.pdb %LinkerFlags% /EXPORT:UpdateAndRender
del lock.tmp

cl.exe %CompilerFlags% ..\minesweeper\code\win32_minesweeper.cpp /link %LinkerFlags% user32.lib gdi32.lib winmm.lib advapi32.lib

popd
