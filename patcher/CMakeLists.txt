cmake_minimum_required(VERSION 3.12)

project(tpdppatcher CSharp)

include(CSharpUtilities)

set(patcher_sources
    "Form1.cs"
    "Form1.Designer.cs"
    "Form1.resx"
    "Program.cs"
    "Properties/AssemblyInfo.cs"
    "Properties/Resources.Designer.cs"
    "Properties/Resources.resx"
    "Properties/Settings.Designer.cs"
    "Properties/Settings.settings"
)

add_executable(patcher WIN32 ${patcher_sources})
set_target_properties(patcher PROPERTIES VS_DOTNET_REFERENCES "System;System.Data;System.Drawing;System.Windows.Forms")
set_target_properties(patcher PROPERTIES DOTNET_TARGET_FRAMEWORK_VERSION "v4.0")
csharp_set_windows_forms_properties(${patcher_sources})

install(TARGETS patcher)
