cmake_minimum_required(VERSION 3.12)

project(tpdpeditor CSharp)

include(CSharpUtilities)

set(editor_sources
    "Json.cs"
    "MainWindow.cs"
    "MainWindow.Designer.cs"
    "MainWindow.resx"
    "MapWindow.cs"
    "Program.cs"
    "PuppetWindow.cs"
    "Properties/AssemblyInfo.cs"
    "Properties/Resources.Designer.cs"
    "Properties/Resources.resx"
    "Properties/Settings.Designer.cs"
    "Properties/Settings.settings"
)

add_executable(editor WIN32 ${editor_sources})
set_target_properties(editor PROPERTIES VS_DOTNET_REFERENCES "System;System.Data;System.Drawing;System.Windows.Forms;System.Xml;System.Runtime.Serialization")
set_target_properties(editor PROPERTIES DOTNET_TARGET_FRAMEWORK_VERSION "v4.0")
csharp_set_windows_forms_properties(${editor_sources})

install(TARGETS editor)