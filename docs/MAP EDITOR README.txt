Controls:
Left click: Paint.
Right click: Erase.
Middle Click: Edit event.
Ctrl + left click: Select brush from active layer.
Ctrl + right click: Erase (all  layers).
Shift + drag left click: Copy region (all layers).
Alt + left click: Paste.
Alt + right click: Flood fill.

Hotkeys (when the map is focused):
Ctrl + Z: Undo.
Ctrl + Shift + Z: Redo.
1-8: Select active layer.
Ctrl + 1-8: Toggle layer visibility.
G: Toggle grid.
L: Toggle labels.

Some important notes about map editing:

1. Event IDs can be used only once per map. you will need to make copies if you want more of them on the same map.
2. Mapping multiple events onto the same trainer/script file will not work properly for events that save state (e.g. trainers and items).
3. The type of event (e.g. trainer, item, etc) is determined by the value of its ID.

See "EVENT README.txt" for details on how to place and edit events/warps/objects.

"Zone" values for the zone mask:
1: Elevated terrain
2: Ramp (transition up/down from elevated terrain)
3: Collision (solid wall)
4: One-way ledge from up to down
5: One-way ledge from right to left
6: One-way ledge from left to right
7: One-way ledge from down to up
8: Water (allows boat riding)
10: Waterfall
11: Disable running + bike
12: Disable bike riding
13: Ramp down from collision layer 2
14: Ramp up to collision layer 2
15: Slide on ice
Possibly more, look around.

The "Background Pattern" layer contains a pattern of tiles in the top left corner used as the background (beyond the edge of the map).
Leaving it blank produces a reflection of the sky.

Event scripts (gn_dat5/script/event) are mostly opaque for the moment.
You'll just have to identify them by finding where they're used in the game.

Expect weirdness as the event system is not fully understood yet.

See mad.txt, fmf.txt, and obs.txt in the file_format folder for a discription of the actual map files.
