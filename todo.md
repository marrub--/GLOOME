top priority shit
===
```
add automap to texture rendering (SetAutomapToTexture? heh)
add full button input getting, maybe even an event system to acs
fix cameratextures, in software you can see the player's weapon sprite (but you can't in ogl)
add a parm to SetCameraToTexture that disables/enables weapon sprite viewing
fix the menus with variadic ticrates
add if statements to menudefs (for cvars and shit)
screenspace shaders
change weapon sprite offsets via ACS (APROP_?) viewtopic.php?f=34&t=29910
rectangular collision boxes (will require a "force" parm for SetActorAngle)
add sound offsets ( see https://stackoverflow.com/questions/434599/openal-how-does-one-jump-to-a-particular-offset-more-than-once )
```

second priority stuff
===
```
ask david about adding an internal acs compiler
orthogonal cameras
A_Warp in ACS
frame tics as expression in decorate
add flags to offset
- OFF_NEUTRAL = 1 // default offsets to 0,0
- OFF_RELATIVE = 2 // obvious
- OFF_FORCEADD = 4 // will force adding to the offset if bobbing
add offsetx and offsety expressions to decorate
add menu backgrounds to menudef
make modeldef support more than 4 pieces per actor
merge ryan's checkweapon changes
probably merge these:
viewtopic.php?f=34&t=31497
viewtopic.php?f=34&t=31516
viewtopic.php?f=34&t=46380
```

"eventually"
===
```
redo vid_renderer to switch opengl versions (between 1.8.10 and 2.x renderers)
probably angelscript to replace decorate and acs
ttf fonts
```

when cybernetic koalas have taken over the earth
===
```
zandronum MP. hell yeah client/server
```

