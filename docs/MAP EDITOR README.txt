Controls:
Left click: Paint.
Right click: Erase.
Ctrl + left click: Select brush from active layer.
Shift + drag left click: Copy region (all layers).
Alt + left click: Paste

Some important notes about map editing:

1. Event IDs can be used only once per map. you will need to make copies if you want more of them.
2. Mapping multiple events onto the same trainer/script file will not work properly for events that save state (e.g. trainers and items).
3. The type of event (e.g. trainer, item, etc) is determined by the value of it's ID.

The meaning of the "Event Arg" field depends on the type of event.
For events 1-255 it points to script files in gn_dat5/script/event.
For events 256+ it points to text files in gn_dat5/script/talk.
For events 512+ it points to trainer dod files in gn_dat5/script/dollOperator.
For events 896+ it's the map ID to warp to.

"Index Arg" applies only to warps, and is the event ID of it's counterpart on the other map.
For any other events it should be set to the events own ID.

The "Flags" field should be copied from a similar event (i haven't figured out what these do yet).

"Zone" values for the zone mask:
3: Collision (solid wall)
8: Water (allows boat riding)
11: Doors
12: Disable bike riding
Probably more, look around.

The "Background Pattern" layer contains a pattern of tiles in the top left corner used as the background (beyond the edge of the map or transparent tiles).
Leaving it blank produces a reflection of the sky.

Event scripts (gn_dat5/script/event) are mostly opaque for the moment.
You'll just have to identify them by finding where they're used in the game.

To make a new map, copy one of the existing map folders and rename everything.
You can then open it in the editor as ususal.
However warps seem to be broken in new maps as they will all be "locked".
You can warp in but you will be trapped there unless you gap map out.
In fact, dialogue events seem to be the only ones that work properly.
I've yet to determine the exact cause of this, but it seems to be external to the map files themselves.
If you really need entirely new maps you can repurpose the unused ones as events do work correctly on those.

Expect weirdness as the event system is not fully understood yet.

See mad.txt, fmf.txt, and obs.txt for more information on the map files.
