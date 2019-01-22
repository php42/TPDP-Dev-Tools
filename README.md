# TPDP-Dev-Tools
This is a set of "Romhacking" tools for TPDP.  
These are command-line tools, basic knowledge of the windows command prompt is required.  
An explanation of the individual tools follows below.  

## Diffgen
This is a patching utility, you don't need to use it if you'd prefer to use xdelta or whatever.  
Like most romhack patchers, it produces differential patches so that game assets need not be redistributed.  
It just provides TPDP specific functionality such as bypassing the encryption and compression on the archives which reduces diff sizes and allows file-level patching within the archive.  
It also provides a convenient way to extract all the files from the archives for editing.  
Invoke with --help for syntax.

Patches are applied on a **per-file** (within the archive) basis.  
This means that *multiple mods can be stacked*.  
Files that fail the CRC check are simply skipped, i.e. the first mod applied is the one that "sticks" for a given file.  
This does not apply to -m 2 (mode 2) which applies the diff to the entire archive (used to add new files to the archive).  
This means that only one mode 2 patch can be applied and it needs to be applied first. Mode 1 patches can still be applied afterward.  

Note that you *cannot create new directories* even in mode 2 as there is no legitimate need to do so.

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

## Example Session
```batch
::extract the files
diffgen.exe -i "C:\games\TPDP" -o "C:\extract" --extract

::convert extracted binary files to json
binedit.exe -i "C:\extract" --convert

::edit files in C:\extract as you please
::you can also add new files, the directory tree mirrors the layout of the archives

::use the json files to patch the binaries
binedit.exe -i "C:\extract" --patch

::generate a diff file using the modified directory tree
::note that if you have added new files to the directory tree and want them to be included
::you must invoke diffgen with -m 2 (mode 2) which applies the diff to the entire archive
::rather than to individual files within the archive
diffgen.exe -i "C:\games\TPDP" -o "C:\extract" --diff="C:\diff\diff.bin"

::patch the game files using the generated diff file (mode 2 is detected automatically)
diffgen.exe -i "C:\games\TPDP" -o "C:\diff\diff.bin" --patch
```


## Compiling
Dependencies: [open-vcdiff](https://github.com/google/open-vcdiff) and [boost](https://www.boost.org/)  
A C++ 17 compatible compiler is required.

Installing dependencies:  
You may retarget the msvc project file include directories, or you may install them to the 'deps' folder such that the following paths are valid:

Include:  
TPDP-Dev-Tools/deps/boost/  
TPDP-Dev-Tools/deps/open-vcdiff/src/  

Binaries:  
TPDP-Dev-Tools/deps/boost/stage/lib/  
TPDP-Dev-Tools/deps/open-vcdiff/build/Release/  
TPDP-Dev-Tools/deps/open-vcdiff/build/Debug/

You may also run `git submodule update --init --recursive` to obtain open-vcdiff.
