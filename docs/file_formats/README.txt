These docs detail both the json files produced for a given binary as well as the binaries themselves.
Brief descriptions of the binaries are given for most formats, but for specific implementation details you may need to check the source code.
For details on most of the binary formats, check gamedata.h and gamedata.cpp in libtpdp.
For details on the encrypted puppet format, check puppet.h and puppet.cpp in libtpdp.
For details on the encrypted save file, check savefile.h and savefile.cpp in libtpdp (note that these facilities are presently unfinished).
Beyond that, use a hex editor and figure it out. Anything that isn't documented here or in the source code is unknown.

note that ID 0 is typically used for None/Nothing where applicable
for binary formats, the ID of an element is indicated by its position in the file (this also applies to some text formats). therefore most will have a NULL element at the beginning.

Note that the game uses Shift-JIS encoded text EVERYWHERE. All text is shift-jis encoded, whether it's in a text file, binary file, or the executable itself.
The json files, however, are UTF-8 encoded with unix line endings. Please use a proper text editor (i.e. *NOT* MS Notepad).

.csv files are text configuration files, you can edit them with any text editor so long as you make sure to save as shift-jis encoding with windows (CRLF) line endings or the game will not be happy.

important files:
Efile.bin (used for encryption of puppets and the save file)
DollData.dbs (puppet definitions)
DollCaption.csv (puppet names and descriptions)
SkillData.sbs (skill definitions)
SkillData.csv (skill names and descriptions)
effect.csv (information about the effect of skills)
AbilityData.csv (ability names and descriptions)
Compatibility.csv (effectiveness of elements vs other elements http://en.tpdpwiki.net/wiki/Type_Chart )
DollHeight.csv (sprite metrics)
DollMainGLoadFlag.bin (used to specify which sprites are valid and should be loaded)
ItemData.csv
ObjGLoadFlag.bin (used to specify which overworld sprites are valid and should be loaded)
ObjFirstActivity.bin (event flags)
*.dod (trainer battle)
*.mad (wild puppets and other map attributes)
*.fmf (map geometry)
*.obs (map event table)
MapName.csv
StandName.csv

probably more stuff, look around
