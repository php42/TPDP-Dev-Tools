:: Note: build the "INSTALL" target before running this script

@echo off
if exist ".\bin\" (
mkdir ".\release\lib"
mkdir ".\release\include\libTPDP"
mkdir ".\release\include\common"
mkdir ".\release\docs"
mkdir ".\release\docs\file_formats"
copy "..\LICENSE" ".\release\LICENSE"
copy "..\licenses.txt" ".\release\licenses.txt"
copy "..\README.md" ".\release\README.md"
copy ".\lib\libtpdp.lib" ".\release\lib\libtpdp.lib"
copy ".\include\libTPDP\*.h" ".\release\include\libTPDP\*.h"
copy ".\include\common\*.h" ".\release\include\common\*.h"
copy ".\bin\*.exe" ".\release\*.exe"
copy ".\bin\zlib.dll" ".\release\zlib.dll"
copy "..\docs\*.txt" ".\release\docs\*.txt"
copy "..\docs\file_formats\*.txt" ".\release\docs\file_formats\*.txt"
) else (
echo ---
echo Please build the INSTALL target from the MSVC project file before running this script
pause
)
