:: Note: build the "INSTALL" target before running this script

@echo off
if exist ".\bin\" (
mkdir ".\release\lib"
mkdir ".\release\include\libTPDP"
mkdir ".\release\include\common"
mkdir ".\release\docs"
copy "..\LICENSE" ".\release\LICENSE"
copy "..\licenses.txt" ".\release\licenses.txt"
copy "..\README.md" ".\release\README.md"
copy ".\lib\libtpdp.lib" ".\release\lib\libtpdp.lib"
copy ".\include\libTPDP\*.h" ".\release\include\libTPDP\*.h"
copy ".\include\common\*.h" ".\release\include\common\*.h"
copy ".\bin\*.exe" ".\release\*.exe"
copy "..\docs\*.txt" ".\release\docs\*.txt"
) else (
echo ---
echo Please build the INSTALL target from the MSVC project file before running this script
pause
)
