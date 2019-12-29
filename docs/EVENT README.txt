To place an event/object/warp on a map, select one of the object layers (normally the first).
Set the brush value to the event ID you want, and place it anywhere.
The type of event is determined by the value of the ID, more on this later.
Event IDs are unique to each map, so an ID may be reused on different maps without conflicting.

To modify the parameters of an event (e.g. the one you just placed), go to the Event tab and set the "Event Index" to the event ID you want to modify.
"Object ID" selects the sprite for the event.
"Movement Mode" is mostly used for trainers and NPCs. It controls if and how the NPC moves around, which direction it faces, etc.
"Movement Speed" controls the delay between "steps" when the NPC moves around. Lower is faster.
"Event Arg" contains the ID of the script/trainer/etc file. Its exact purpose depends on the type of event (see below).
"Index Arg" is used for warps. It contains the event ID of the destination on the other map.
"Flags" contains a series of 12 mostly unknown 1-byte values (see below). It can normally be left at default, or you can copy the value from a similar existing event.
"Enabled by Default" determines if the event is active the first time the map is loaded up. If unchecked, the event will need to be enabled by another event.

Event type by ID value:
1-255: Scripted events and static objects.
256+: Signs and dialogue.
512+: Trainers.
896+: Warps.

The purpose of "Event Arg" varies depending on the value of "Event Index".
"Event Arg" function by "Event Index" value:
1-255: Points to script files in gn_dat5/script/event.
256+: Points to text files in gn_dat5/script/talk.
512+: Points to trainer dod files in gn_dat5/script/dollOperator.
896+: The map ID to warp to. For warps within the same map, use the current map ID.

The "Flags" values are mostly unknown, but the 3rd value adjusts the maximum distance that trainers will go to intercept the player when entering line of sight.
When set to 0, the player will need to explicitly interact with the trainer to begin the battle.

"Movement Mode" values:
0: static, 1: face down, 2: face left, 3: face right, 4: face up
5: look around, 6: wander, 7: run around, 8: face player
9: intercept player, 11: look away from player, 12: avoid player
13: run from player, 14: spin counter-clockwise, 15: spin clockwise
