.evs scripts each contain 32 10-field entries to be executed sequentially.
The first field of each entry determines the type of operation for that entry.
A list of known values (in hexadecimal) is given below; original list courtesy of OmegaBloodmoon, expanded upon by Patri.

1: Action done by the player (Move, jump, emotion bubbles, etc...)
2: Same except for objects on the map
3: Camera
4: Warps the player
5: Loads a txt file
6: Enables/disables a flag
7: Stops the current event and loads another one
8: Enables/Disables an object on the map
9: Flag test (If flag is false, keeps reading the next chunk normally, otherwise skips to chunk x, first chunk being chunk 0)
A: Screen fade
B: Display images
C: Battle Puppet
D: Battle trainer
E: Battle starter
F: Toggles trainer. After a trainer is beaten, it can't be fought again, even via events.
10: Name Change
11: Gender Change
12: Choose Puppet
13: Give Puppet
14: Gives item
15: Removes item
16: Item tests (Functions about the same way as 9)
17: Plays music
18: Plays sound effect
19: Puppet box
1A: Change object behavior
1B: Direction Test
1C: Shop
1D: Team heal
1E: Alpha Color
1F: Check Puppet Book completion
20: Teleport object
21: Fullscreen text
22: PvP hub
23: Reincarnation
24: Toggle Gap Map Spot
25: Save
26: Choose favorite
27: Randomize favorite
28: Credits
29: Release Puppet
2A: Costume menu
2B: Toggle money display
2C: Check money (Works about the same as 9)
2D: Subtract money
2E: Random branch
2F: Calls a list
30: Follower lock
31: Rental team
32: Limit steps
33: Cancel steps limit
34: Set HVT Trainer
35: Reset Winstreak
36: Increment Winstreak
37: Check Winstreak
38: Fight HVT Trainer
39: Conditional branch
3A: Import base game save
3B: Change map weather
3C: Increment/decrement accumulator variable
3D: Comparison against accumulator variable
3E: Reset accumulator variable
3F: Update Winstreak
40: HVT Validity Check
41: Forced Save
42: HVT Rewards
43: Previous favorite check
44: Load starter
45: Uncatchable Battle
46: Post-game Credits
47: No-op
48: Use Item
49: Sell Items, max vanilla event opcode

=============================================================================================
In-depth documentation:
(Big Special Thanks to OmegaBloodmoon, and AtaeKurri by extension, for a lot of information on multiple of these!)

100 + event ID: Runs event without waiting for them to finish. Allows for running multiple event lines at the same time.
A simultaneous event will run itself and any other simultaneous events below it, until it runs into an event that does wait.
	
Event syntax:
The events are written as they would be in the Event Scripts editor. For the sake of viewing the event files in a hex editor, each field is two bytes; field 1 is the first two bytes, field 2 is the next two bytes... so on, so forth.
Note for the sake of hex editing: the fields are in little endian; byte pairs are stored OPPOSITE of how they'd be read.
e.g.: 0x1234 would be stored as 0x34, followed by 0x12.

1: Player Actions
	1, [Action], [Facing Direction], [Count], 0, [Extra Parameter]
	
	Action List (Shared with Object Actions):
	0 = Nothing
	2 = Turn Around
	3 = Wait
	4 = Nothing?/Also wait?
	5 = Emote [Uses Extra Parameter; see below]
	6 = Run in place
	7 = Jump in place
	8 = Counter-clockwise spin around
	9 = Clockwise spin around
	A = Toggle visibility (Hitbox stays)
	B = Toggle bicycle (Player-Only)
	C = Animate and vanish (Breakable Rocks and Cuttable Trees. Hitbox disappears)
	D = Walk
	E = Run
	F = Fast Walk?
	10 = Walk backwards
	11 = 2-Tile Jump
	12 = Enter Boat (Player-Only)
	13 = 1-tile Jump (Player-Only)
	14 = Running Bump 
	15 = Face Player / Face Left, if Player
	16 = Slow walk in place
	17 = Gap Map animation + Toggle Player's visibility (Player-Only)
	18 = FAST Run (Object-Only, ignores collision)
	19+ = Crashes the game
	
	Facing Direction List:
	0 = Don't Update
	1 = Down
	2 = Left
	3 = Right
	4 = Up
	5 = Away from Current
	6+ = Away from Current - just use 5

	Extra Parameters:
		Emote:
		0 = ??
		1 = Exclamation
		2 = Question
		3 = Smile
		4 = Sweatdrop
		5 = Lightbulb
		6 = Note
		7 = Heart
		8 = Blush
		9 = Sleep
		A = ...
		B = Annoyed
		C = Anger Vein
		D = Exclamation (No Sound)
		E = Sparkles
		F = MG (Chewing)
		10 = Neutral Face
		11 = Wink
		12 = Stare
		13 = Very Happy
		14 = NONE
	
2: Object Actions
	2, [Action], [Facing Direction], [Count], [Object ID], [Extra Parameter]
	
	Object ID Specials (Can be used as Target Object in other Event Types):
	0 = Self
	892 (0x37C) = Player's Puppet
	1024 (0x400) = Player (Applies to other objects' targets, but can't be used to make the player do things directly. Use event type 1 for player actions)
	
3: Camera Control
	3, [Camera Mode], [Scroll Speed - Higher is faster], [Tile Count], [Target Object]
	
	Camera Modes:
		1 = Down
		2 = Left
		3 = Right
		4 = Up
		5 = Does nothing
		6 = Center on Player (Default)
		7 = Center on Object
	
	If using a target object, set Tile Count to a non-zero value.
		
4: Warps
	4, [Map ID. 0 = Self], [Target Object], [Facing Direction], [Transition Type], [Silent Warp flag]
	
		Transition Types:
		0 = Full-screen Fade to and from Black
		1 = Directional Fade + Walk Forward
		2 = Flip Screen
		
	Note: What Collision Layer the player is on after a warp depends on what layer the Target Object's on.
	
5: Dialogue
	5, [Text ID], [1 = Yes/No Prompt], [Line to skip to if No is picked]

6: Flag Toggle
	6, [Flag ID], [0 = False / 1 = True]

7: Event Trigger
	7, [Built-in Flag Check. 1 = Always Load - Otherwise, load if Flag is True], [New Event ID]
	
8: Object Toggle
	8, [Map ID. 0 = Self], [Object ID. 0 = Self], [0 = Disabled / 1 = Enabled]
	
9: Flag Check
	9, [Flag ID], [Line to Skip to if Flag is True]

A: Screen Fade
	A, [0 = Black / 1 = White], [Opacity: 00-FF], [Fade Speed. Higher is faster]

B: Display Image
	B, [Image ID], [Fade Speed. Higher is faster], [Opacity]
	
	Images are loaded from gn_dat5.arc/graph.
	
C: Puppet Battle
	C, [Puppet ID], [Line to Skip to on Win - 0x21 defaults to next], [Line to Skip to on Loss - 0x21 defaults to blackout], [Style], [Level]

D: Trainer Battle
	D, [Trainer ID], [Line to Skip to on Win - 0x21 defaults to next], [Line to Skip to on Loss - 0x21 defaults to blackout]

E: Starter Battle
	E
	
		Utilizes event 44 to load its data. Loads nothing, not even NULL/ID 0, on its own.
		Starter is locked to Level 5, Normal style.

F: Trainer Toggle
	F, [Trainer ID], [0 = Not Fought, 1 = Fought]
	
10: Name Change
	10, [What to Name]
		What to name:
		0: Player
		1: Puppet (Defined through Event 12)
		2: Box (Not Defined)

11: Player Gender Change
	11
	
	The change in the Player's sprite is instantaneous, so hiding it or using a screen fade is recommended.
	Prompt's text is baked into gn_dat1.arc/common/message/SexSelectMessage.txt

12: Puppet Choice
	12, [Line to Skip to on back out]

	Used for selecting a Puppet in your party to be the target of other events. Required by:
	ID 10 (Name Change), ID 29 (Release Puppet), ID 2A (Costume Change)
	
13: Give Puppet
	13, [Puppet ID], [Style], [Level], [Unknown], [Line to Skip to after execution - 0x21 continues to next line]
	
		If Event type 44 is executed before, and Puppet ID is set to 0, gives starter instead.
		Style and level arguments are completely ignored; giving a starter always gives the favorite at level 5, Normal Style, all ranks at S, Heart Mark and Dream Shard/Boundary Trance equipped.

14: Give Item
	14, [Item ID], [Count], [Line to Skip to if max exceeded - 0x22 shows default message and ends event], [Hide Message Box Flag]
	
15: Take Item
	15, [Item ID], [Count]
	
16: Item Check
	16, [Item ID], [Count], [Line to Skip to on Fail]
	
17: Switch Background Music
	17, [Music ID], 0
	Setting the 3rd field to 1 causes a softlock. The game probably tries to wait, much like with Sound Effects.
	Set Music ID to 10000 (0x2710) to reset it to the map's default.
	
18: Play Sound Effect
	18, [Sound ID], [0 = Proceed to next line immediately / 1 = Wait for SFX to end first]

19: Puppet Box
	19

1A: Change Object Behavior
	1A, [Object ID. 0 = Self], [Behavior], [Movement Speed. 0 = Unchanged; 0x100 = 0. Lower is faster]

	The object's behavior will only remain changed as long as the map isn't changed and the player doesn't save and reload.
	Behavior and movement speed values are exactly the same as in the Map Editor.

1B: Direction Check
	1B, [Object ID. 0 = Self], [Jump if Facing Down], [Jump if Facing Left], [Jump if Facing Right], [Jump if Facing Up]
	Setting any of the 3rd, 4th, 5th or 6th field to 0x21 advances execution to the next line.
	
1C: Open Shop
	1C, [Shop ID]

	Shops are configurable from gn_dat5.arc/script/shop.
	
1D: Team Heal
	1D, [Heal Type]
		0: Heal SFX
		1: Silent
		2: Fade to Black + Jingle
		3+: Softlock
	
1E: Alpha Color
	1E, [Color]
	
	Alpha gets composited with this color.
		0: Black
		1: White
	
	Strange blending operation that causes semi-transparent pixels to get blown out if you set it to 1.
	Not very useful.

1F: Puppet Book Check
	1F, [Puppet Count], [Line to Skip to if not enough Puppet types caught]

20: Teleport object
	20, [Object ID], [Target X Pos], [Target Y Pos], [Always 2?]
	
21: Fullscreen Text
	21, [Text ID], [Print Speed Modifier. Lower is faster; 0 = Instant]

22: Online Play Menu
	22

23: Reincarnation Menu
	23

24: Enable/Disable Gap Map Locations
	24, [Map ID], [0 = Disable, 1 = Enable]

25: Forced Save
	25

26: Starter Choice
	26, [Line to Skip to on Cancel]
	
27: Randomize Starter
	27

28: Credits
	28

29: Release Puppet [Uses 12 to select Puppet]
	29, [Line to Skip to if trying to release last Puppet in party]

2A: Costume Change [Uses 12 to select Puppet]
	2A, [Line to Skip to on Cancel]

2B: Toggle Money Display
	2B

2C: Money Check
	2C, [Count], [Line to Skip to if money's too low]

2D: Take Money
	2D, [Count]

2E: Random Branch, 2 outcomes
	2E, [Probability of Success. 1 = 50/50, higher seems to decrease it], [Line to Skip to if chance succeeds]

2F: List Trigger
	2F, [Text ID], [List ID]
	
	Lists handle any prompt more complex than a simple Yes/No.
	Lists are configurable from gn_dat5.arc/script/list.
	Backing out of a list always picks the bottommost option.
	
30: Lock/Unlock Followers
	30, [1 = Lock, 0 = Unlock]

	Affects both the following Puppet and any object with behavior type 17 (Follow Player).
	
31: Preset Teams
	31, [Team ID]

	Do note, this REPLACES the current team. Any Puppets the Player currently has in their team will be lost.
	Preset Team Puppets work and behave just like any other Puppet.
	
32: Start Step Count
	32, [Step Count], [Event ID to trigger once said steps are taken]

33: Stop Step Count
	33
	
34-38: HVT Events. See Omega's HVT Doc [Or here, if they add on the info].

39: Conditional Branch
	39, [Line]
	
	Jumps to [Line] depending on some flag set when reading a base game save file.
	Presumably meant to be used with 3A.

3A: Import base game save
	Appears to read puppets from a base TPDP save file and put them... somewhere.

3B: Switch Map Effect
	3B, [Map ID], [Effect ID]

	Weather/Effect List:
	0: None
	1: Darkness
	2: Fog
	3: Red Fog
	4: Snow
	5: Cherry Blossoms
	6: Heat
	7: Spirits?
	8: Light Orbs
	9: Rain
	
	Replaces the effect for the map in question PERMANENTLY, until changed again via this event. Carries over between map transitions and save & reloads.

3C: Increment/Decrement Accumulator
	3C, [Index], [Value], [Subtract]
	
	Adds [Value] to one of the 128 accumulator variables in the save file.
	[Index] must be less than 0x80 (128).
	If [Subtract] is nonzero, [Value] is negated (subtracted).
	The result is clamped to the range [0, 9999].

3D: Compare Accumulator
	3D, [Index], [Value], [Operator], [Line]
	
	[Operator] selects one of the following operations:
		0: [Value] <= accumulator
		nonzero: accumulator < [Value]
	
	Jumps to [Line] if [Operator] evaluates true.

3E: Reset Accumulator
	3E, [Index]
	
	Sets specified accumulator to 0.

3F-40: HVT Events. See Omega's HVT Doc [Or here, if they add on the info].

43: Check Starter Match (New Game+)
	43: 43, [Line to Skip to if Current starter = Last run's starter]
	
	Omega: "Seems to check if your chosen favorite is the same as your previous playthrough"

44: Load Starter
	44
		
		Required by Event type E, and can be used by Event type 13 to give a starter. Probably necessary due to the Human Village Tournament, and the starter having either a Dream Shard or a Boundary Trance depending on it.

45: Uncatchable, Unfleeable Puppet Battle
	45, [Puppet ID], [Line to Skip to on Win - 0x21 defaults to next], [Line to Skip to on Loss - 0x21 defaults to blackout], [Style], [Level]
		
		Exactly the same as Event Type C, but the Puppet is uncatchable and you can't run away from the fight. You can, however, use a Plain Doll to flee. This is counted as a win.
	
46: Post-Game Credits

47: No-op
	Advances to next line.
	This is also the default behaviour for invalid event opcodes.

48: Use Item
	48, [Item ID]
	
	Invokes the effect of [Item ID] as if it were used by the player.
	Some items have hardcoded behaviours when used this way:
		0x00A: Bead necklace.
		0x00B: Talisman.
		0x00C: Magatama.
		0x243: Gap map, behaves like event 4 with a target object of 894. Target map is set elsewhere but defaults to 0 (same map).
		0x245: Bike.
		0x248: Boat.

49: Sell Items
	Open shop screen for selling items in your bag.
	This is the highest event opcode in vanilla SoD.