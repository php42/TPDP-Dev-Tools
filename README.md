# TPDP-Dev-Tools
This is a set of "Romhacking" tools for TPDP.  
While a GUI front-end is provided, these are primarily command-line tools. Basic knowledge of the windows command prompt is strongly recommended.  
An explanation of the individual tools follows below.  

## Diffgen
This is a patching utility, you don't need to use it if you'd prefer to use xdelta or whatever.  
Like most romhack patchers, it produces [differential](https://en.wikipedia.org/wiki/Data_differencing) patches so that the original game assets need not be redistributed.  
It provides TPDP specific functionality such as bypassing the encryption and compression on the archives which reduces diff sizes and allows file-level patching within the archive.  
It also provides a convenient way to extract all the files from the archives for editing.  
Invoke with --help for syntax.

Patches are applied on a **per-file** (within the archive) basis.  
This means that *multiple mods can be stacked*.  
Files that fail the CRC check are simply skipped, i.e. the first mod applied is the one that "sticks" for a given file.  
This does not apply to -m 2 (mode 2) which applies the diff to the entire archive (used to add new files to the archive).  
This means that only one mode 2 patch can be applied and it needs to be applied first. Mode 1 patches can still be applied afterward.  

Note that deleting files from the archive is presently unsupported.

## BinEdit
This converts the games various binary file formats to human-readable json and back. The produced json files are saved alongside the original (e.g. DollData.dbs -> DollData.json in the same folder).  
Note that these are also patches in the sense that the json files are used to "patch" the extracted binary files.  
The reason for this is that the entire file format is not always known, so this allows edits to be made to the known portions of existing files.  
Be prepared to bust out some python scripts or something because some of these produce large quantities of json.  
Invoke with --help for syntax.

Note that this tool *recursively scans the entire target directory tree* so be careful where you point it.

## LibTPDP
This is a c++ static library that provides facilities for manipulating the games various binary formats, including the archives themselves.  
It was made for the other tools in this project, but you can use it to make your own tools if you'd like.  
Documentation is in the form of comments in the header files.  
Its only dependency is windows.h.  

## Patcher
This is a C# GUI front-end for patching the game with a diffgen diff file.  
It literally is just a wrapper that invokes diffgen.exe with appropriate arguments for the convenience of people who don't want to use the command-line.  
This is intended to make it easy for end-users to apply a mod to their game

## Editor
This is a C# GUI front-end for diffgen and binedit.  
It also helps with some of the editing, providing cross-referencing of names and IDs, a graphical map editor, and similar utilities.  
Note that it is _not_ a complete substitute for manual editing and is provided as a convenience for certain tasks.

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

::generate a diff file using the modified directory tree
::note that if you have added new files to the directory tree and want them to be included
::you must invoke diffgen with -m 2 (mode 2) which applies the diff to the entire archive
::rather than to individual files within the archive
diffgen.exe -i "C:\games\TPDP" -o "C:\extract" --diff="C:\diff\diff.bin"

::patch the game files using the generated diff file (mode 2 is detected automatically)
diffgen.exe -i "C:\games\TPDP" -o "C:\diff\diff.bin" --patch

::Alternatively, instead of using --diff and --patch, you can use --repack
::which applies the current working directory directly to the game data.
::This is intended for the testing and development cycle where you just
::want to verify that your changes work correctly in-game.
diffgen.exe -i "C:\games\TPDP" -o "C:\extract" --repack
```


## Compiling
Prerequisites:  
[CMake](https://cmake.org/), [boost](https://www.boost.org/), and Visual Studio 2017 or newer

Cloning the repo:  
Make sure to clone with submodules: `git clone --recurse-submodules https://github.com/php42/TPDP-Dev-Tools.git`  
If you already cloned without submodules, you can get them like so: `git submodule update --init --recursive`

Installing Boost:  
Download boost from [here](https://www.boost.org/users/download/)  
You can use the prebuilt binaries if they are provided for your version of Visual Studio, but it is strongly recommended that you follow
the [instructions](https://www.boost.org/doc/libs/1_70_0/more/getting_started/windows.html) and build them from source.  
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
