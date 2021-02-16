.evs scripts each contain 32 10-field entries to be executed sequentially.
The first field of each entry determines the type of operation for that entry.
A list of known values (in hexadecimal) is given below, courtesy of OmegaBloodmoon.

1: Action done by the player (Move, jump, emotion bubbles, etc...)
2: Same except for objects on the map
3: Camera
4: Warps the player
5: Loads a txt file
6: Enables/disables a flag
7: Stops the current event and loads another one
8: Enables/Disables an object on the map
9: Flag test (If flag is enabled, keeps reading the next chunk normally, otherwise skips to chunk x, first chunk being chunk 0)
a: Works with b to make shadings, pictures, etc... Not sure how it works since I only need to make a black screen
b: ^
c: Battle puppet
d: Battle trainer
e: Battle puppet? Prob has specifics that diferentiate it from c, but didn't look into it much
f: Enables trainer. After a trainer is beaten, it can't be fought again. Even if an event should load his battle, it won't. So you need f to reenable it. Not sure if it also reenables trainers on the map, only tested with event trainers.
10/11/12: The only notes I have for those is "TEST" so either it looks like a test of some kind, or I put a note to make tests about it. Not sure since I don't remember where I saw those.
14: Gives item
15: Removes item
16: Item tests (Functions about the same way as 9)
Starting from here I'm unsure about the effects since I never tested them, but it's what it's probably doing
17: Plays music
19: Puppet box
1d: Team heal
22: Pvp hub
25: Save?
26/27: I labeled both as "Hina?" so either of them is probably the seal break
28: Credits?
2f: Calls a list
45: Special battle. Not sure what exactly is different from a normal puppet battle, but this one is used for lv99 NLily battle so...
Considering the highest one I found is 45, and if we say that all values below that are used, that makes a LOT of unknown events

(Editors note: the lvl 99 lily is uncatchable, so 45 might be for uncatchable puppets)
