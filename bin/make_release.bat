:: Note: build the "INSTALL" target before running this script

@echo off
if exist ".\bin\" (
mkdir ".\release\docs"
mkdir ".\release\docs\file_formats"
copy "..\LICENSE" ".\release\LICENSE"
copy "..\licenses.txt" ".\release\licenses.txt"
copy "..\README.md" ".\release\README.md"
copy ".\bin\*.exe" ".\release\*.exe"
copy ".\bin\z.dll" ".\release\z.dll"
copy "..\docs\*.txt" ".\release\docs\*.txt"
copy "..\docs\file_formats\*.txt" ".\release\docs\file_formats\*.txt"
) else (
echo ---
echo Please build the INSTALL target from the MSVC project file before running this script
pause
)
