# TPDP-Dev-Tools
This is a set of "Romhacking" tools for TPDP.  
The toolchain consists of a GUI front-end editor and some command-line back-end tools.  
Basic knowledge of the windows command prompt is recommended, but not required.  
An explanation of the individual tools follows below.

## Editor
The GUI editing interface. It supports editing puppets, trainers, skills, and maps.  
It also handles invoking the command-line tools for most tasks, including creating patch files.  
However not everything can be handled by the GUI. As part of the setup, all of the games files will be extracted to a "working directory".  
Changes to any of the files in the working directory (as well as files added to the directory) will be reflected in the game data when repacking or creating a patch.  
Sprites and other data files must be edited manually in this way.

## Patcher
GUI front-end for patching the game.  
It literally is just a wrapper that invokes diffgen.exe with appropriate arguments.  

## Diffgen
This is a command-line utility for manipulating the games encrypted archives.  
It supports extracting, repacking, and patching the game data.  
Invoke with --help for syntax.

Patches are applied on a **per-file** (within the archive) basis.  
This means that *multiple mods can be stacked*.  
Any files shared between patches are overwritten by the last patch applied.  
The patch format is actually just a zip file that mirrors the layout of the game archives, so these can be made by hand if you like.

Note that deleting files from the archive is presently unsupported.

## BinEdit
This is a command-line utility for converting the games various binary file formats to human-readable json and back.  
The produced json files are saved alongside the original (e.g. DollData.dbs -> DollData.json in the same folder).  
Note that these are also patches in the sense that the json files are used to "patch" the extracted binary files.  
The reason for this is that the entire file format is not always known, so this allows edits to be made to the known portions of existing files.  
Be prepared to bust out some python scripts or something because some of these produce large quantities of json.  
Invoke with --help for syntax.

## LibTPDP
This is a C++ static library that provides facilities for manipulating the games various binary formats, including the archives themselves.  
It was made for the other tools in this project, but you can use it to make your own tools if you'd like.  
Documentation is in the form of comments in the header files.  
Its only dependency is windows.h.

## Example Session
```batch
::extract the files
diffgen.exe -i "C:\games\TPDP" -o "C:\extract" --extract

::convert extracted binary files to json
binedit.exe -i "C:\extract" --convert

::edit files in C:\extract as you please
::you can also add new files, the directory tree mirrors the layout of the archives

::use the json files to patch the binaries (convert json back to binary)
binedit.exe -i "C:\extract" --patch

::generate a patch file using the modified directory tree
diffgen.exe -i "C:\games\TPDP" -o "C:\extract" --diff="C:\patch\patch.bin"

::patch the game files using the generated patch file
diffgen.exe -i "C:\games\TPDP" -o "C:\patch\patch.bin" --patch

::Alternatively, instead of using --diff and --patch, you can use --repack
::which applies the current working directory directly to the game data.
::This is intended for the testing and development cycle where you just
::want to verify that your changes work correctly in-game.
diffgen.exe -i "C:\games\TPDP" -o "C:\extract" --repack
```


## Compiling
Prerequisites:  
[CMake](https://cmake.org/), [boost](https://www.boost.org/) (1.72.0 preferred), and Visual Studio 2017 or newer

Cloning the repo:  
Make sure to clone with submodules: `git clone --recurse-submodules https://github.com/php42/TPDP-Dev-Tools.git`  
If you already cloned without submodules, you can get them like so: `git submodule update --init --recursive`

Installing Boost:  
Download boost from [here]https://www.boost.org/users/history/version_1_72_0.html)  
You can use the prebuilt binaries if they are provided for your version of Visual Studio, but it is strongly recommended that you follow
the [instructions](https://www.boost.org/doc/libs/1_72_0/more/getting_started/windows.html) and build them from source.  
In short:  
1. Unzip the boost sources somewhere
2. In the source folder, run bootstrap.bat, this will create b2.exe
3. Run b2.exe

Using CMake:  
Before doing anything, make a subfolder in the TPDP-Dev-Tools folder called "build".  
Open up CMake, set the source code folder to wherever you cloned TPDP-Dev-Tools, and set the binary folder to the build folder you just made.  
Click configure, change the BOOST_ROOT variable to wherever you extracted Boost, click configure again, and finally click generate.  
All other settings should be fine to leave at default.

Visual Studio project files will be generated in the build folder. From here you can simply open TPDP-Dev-Tools.sln and click build.
