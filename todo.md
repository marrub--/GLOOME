top priority shit
===
```
fix cameratextures, in software you can see the player's weapon sprite (but you can't in ogl)
add a parm to SetCameraToTexture that disables/enables weapon sprite viewing
fix the menus with variadic ticrates
add if statements to menudefs (for cvars and shit)
screenspace shaders
pixelratio in mapinfo/gameinfo
add zcommon.acs and friends to the project for the custom definitions
change weapon sprite offsets via ACS (APROP_?) viewtopic.php?f=34&t=29910
rectangular collision boxes (will require a "force" parm for SetActorAngle)
add sound offsets ( see https://stackoverflow.com/questions/434599/openal-how-does-one-jump-to-a-particular-offset-more-than-once )
```

second priority stuff
===
```
A_Warp in ACS
pufftid parm for LineAttack
frame tics as expression in decorate
add flags to offset
- OFF_NEUTRAL = 1 // default offsets to 0,0
- OFF_RELATIVE = 2 // obvious
- OFF_FORCEADD = 4 // will force adding to the offset if bobbing
add offsetx and offsety expressions to decorate
add menu backgrounds to menudef
make modeldef support more than 4 pieces per actor
merge ryan's checkweapon changes
```

"eventually"
===
```
lua to either replace decorate/acs or just add it as an extra language
ttf fonts
```

