top priority shit
===
```
add if statements to menudefs (for cvars and shit)
change weapon sprite offsets via ACS (APROP_?) viewtopic.php?f=34&t=29910
add sound offsets ( see https://stackoverflow.com/questions/434599/openal-how-does-one-jump-to-a-particular-offset-more-than-once )
decals on floors/ceilings, ask fgsfds for help maybe
redo hudmessages entirely, add a new function that allows you to print sprites as well
dynamically overlayable sprites. hell yeah.
merge ryan's getweapon changes (~/Downloads/GetWeapon changes.diff)
merge these:
viewtopic.php?f=34&t=31497
viewtopic.php?f=34&t=46380
add DEFLTDEF lump to specify defaults for keys/cvars/etc (only for engine.pk3)
[23:46] <nax> marrub:  add "todo:  deez nuts"
[23:49] <TehFakeSalt> I've noticed from messing around with a build I compiled that GLOOME still generates a place in its .ini file for ZDoom's supported games. Not suggesting removing compatibility, but could it only, say, generate controls for Doom if Doom is run once, instead of as soon as the .ini is created? It just kinda creates clutter for games that are likely not even going to ran at all.
A_WeaponAltReady? allowing the user to use the custom psprite layers for TRUE dual weapons
make the hexen armors not hardcoded
hitbox maps
fix cameratextures, in software you can see the player's weapon sprite (but you can't in ogl)
opus fuck yeah http://stackoverflow.com/questions/16496288/decoding-opus-audio-data
```

second priority stuff
===
```
cl_bobstyle cvar (0 defaults to the weapon's, 1-6 will set the bob style) -- also cl_bobspeed/bobx/boby
add a parm to SetCameraToTexture that disables/enables weapon sprite viewing
add automap to texture rendering (SetAutomapToTexture? heh)
ttf fonts
rectangular collision boxes (will require a "force" parm for SetActorAngle)
flag for TEXTURES so that XScale and YScale aren't inverted
model transparency. might look odd in some cases but that's the fault of the person who made the model
ask david about adding an internal acs compiler
orthogonal cameras
voxel/model lighting maybe
prebaked lights? maybe using strive VE's lightmap system? mite b cool
frame tics as expression in decorate

add flags to offset
- OFF_NEUTRAL  = 1 // default offsets to 0,0
- OFF_RELATIVE = 2 // obvious
- OFF_FORCEBOB = 4 // will force adding to the offset if bobbing

add offsetx and offsety expressions to decorate
add menu backgrounds to menudef
make modeldef support more than 4 pieces per actor
```

hard to do, definitely worth it
===
```
screenspace shaders -- have to detangle the clusterfuck that is FShader and friends
redo vid_renderer to switch opengl versions (between 1.8.10 and 2.x renderers)
angelscript to "replace" decorate and acs
zandronum MP. hell yeah client/server
```

