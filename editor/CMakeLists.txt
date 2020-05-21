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
    "TrainerWindow.cs"
    "SkillWindow.cs"
    "DesignWindow.cs"
    "MapDisplay.cs"
    "TilesetDisplay.cs"
    "ResizeDialog.cs"
    "ResizeDialog.Designer.cs"
    "ResizeDialog.resx"
    "ShiftDialog.cs"
    "ShiftDialog.Designer.cs"
    "ShiftDialog.resx"
    "NewMapDialog.cs"
    "NewMapDialog.Designer.cs"
    "NewMapDialog.resx"
    "NewIDDialog.cs"
    "NewIDDialog.Designer.cs"
    "NewIDDialog.resx"
    "PatchDialog.cs"
    "PatchDialog.Designer.cs"
    "PatchDialog.resx"
    "EventWindow.cs"
    "Bitmap.cs"
    "MapPanel.cs"
    "Properties/AssemblyInfo.cs"
    "Properties/Resources.Designer.cs"
    "Properties/Resources.resx"
    "Properties/Settings.Designer.cs"
    "Properties/Settings.settings"
)

add_executable(editor WIN32 ${editor_sources})
set_target_properties(editor PROPERTIES VS_DOTNET_REFERENCES "System;System.Data;System.Drawing;System.Windows.Forms;System.Xml;System.Runtime.Serialization")
set_target_properties(editor PROPERTIES DOTNET_TARGET_FRAMEWORK_VERSION "v4.0")
set_target_properties(editor PROPERTIES VS_GLOBAL_ApplicationIcon "${CMAKE_CURRENT_SOURCE_DIR}/../assets/dev-tools_icon.ico")
csharp_set_windows_forms_properties(${editor_sources})
target_compile_options(editor PRIVATE "/warn:4")

install(TARGETS editor)
