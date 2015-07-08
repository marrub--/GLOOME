**********************
 _____  _     _____  ________  ___ _____ 
|  __ \| |   |  _  ||  _  |  \/  ||  ___|
| |  \/| |   | | | || | | | .  . || |__  
| | __ | |   | | | || | | | |\/| ||  __| 
| |_\ \| |___\ \_/ /\ \_/ / |  | || |___ 
 \____/\_____/\___/  \___/\_|  |_/\____/ 

- OpenGL-Oriented Original Media Engine -

**********************

WHAT IS GLOOME:

GLOOME is a commercial-friendly GPL-compliant rebrand of the GZDoom engine,
which is based on the ZDoom engine, which in turn is based on id Software's
Doom engine. It's a little bit incestuous.

All of the code from Doom has been relicensed from the Doom Software License to
the GPL license, and all legally troubled code (such as the code taken from
Ken Silverman's BUILD engine) have been either removed or rewritten and
licensed to be GPL compatible.

**********************

GLOOME GITHUB:

https://github.com/marrub--/GLOOME/

All of the GLOOME source code is stored here, free for anyone to look at,
persue, fork, or modify for their own purposes, as per GPL requirements.

**********************

WHY GLOOME:

There's been a lot of discussion as of time of this writing about the viability
of using Doom source ports to create full-fledged indie games. A lot of people
have come out with some astounding projects; massive mapsets, complete gameplay
overhauls, redone graphics, etc.
But every now and then, there's a project that's completely unlike Doom at all,
to the point where people say it would be better as its own thing, rather than
a "mod". And sometimes these discussions just kind of ... end with the usual
question marks.

Is it possible to do this? Can mods really become standalone games? Would it
even be feasible to undertake all the work for a small community? What draws
the line between an extremely in-depth "mod" and a fully standalone "game"?
Wouldn't we still need to have a doom2.wad to run? If we still have ties to
Doom, won't that cause legal issues? What would Carmack do?

This fork hopes to address these issues. As mentioned before, all of the
licensed code has been rewritten. All of the legally-questionable code has been
plucked out and replaced with either more legally-compliant code or outright
rewritten. The engine itself is completely open-source and free for anyone to
modify, adjust, distribute, or whatever as they please.

With this, we now have an engine where someone can make a completely new
not-Doom game with and distribute among other sites, like RPG Maker was to
Yume Nikki or Cherry Tree High Comedy Club.

Someone completely unfamiliar with Doom, the Doom modding scene, or anything at
all can just download a game, fire up the .exe, and play it without needing any
know-how or "drag this .pk3 onto this .exe" or "load up multiple files" or
"DON'T PUT IT IN YOUR SKINS FOLDER FOR THE LOVE OF GOD". If somebody wanted to
make a full-fledged indie first-person-shooter, they can use this engine to
create a slew of new maps, new enemies, new levels, new items, new weapons, and
more, and then throw it up on Steam without worrying about Doom copyrights.

**********************

FAQ:

Q: So with this I could develop a whole game in maps, sprites, sounds, etc, and
   then release it commercially for money?
A: Did you not read the above blurb? Yes!

Q: So how exactly would I make something?
A: GLOOME reads, recognizes, and utilizes the exact same code that Doom mods
   have been using for years now--DECORATE, SBARINFO, and ACS. They're easy to
   pick up, easy to use, and provide a lot of power. Likewise, it recognizes
   the same map format that Doom mods have been using, and people can just as
   easily draw a few linedefs, create a room, and get going.

Q: Can I create something other than an FPS with this?
A: Yes. While an FPS is obviously the native genre and one the engine can
   handle the best, people have long been stretching the boundaries to create a
   wide variety of different things. People have made arcade shmups, 2D
   platformers, puzzle games, action RPGs, sneaking games, and visual novels.
   
Q: Is there netplay support?
A: GZDoom's peer-to-peer netplay structure has not been touched and should work
   the exact same way, all frustrations intact. Do note that support for this
	 is limited, and may break in the future due to neglect.
	 Poor little netcode.

Q: Can this run Doom or Doom mods?
A: Yes, you'll have to supply your own copy of Doom or Freedoom to run it with
   It CAN play Doom and Doom mods, but that...isn't the point.

Q: What does this offer me over the GZDoom engine?
A: Mechanically? Nothing. Playing on GLOOME is no different from playing on
   GZDoom. Internally, however, there are a few changes to help game makers,
	 such as adding customizable cheat codes and new functions in
	 ACS and DECORATE.

Q: In terms of exact numbers, what featureset is this compatible with?
A: GZDoom 1.8.10 doesn't line up with an exact ZDoom version number, but if
   generally if you set your goal to be compatible with 2.7.1 then you should
   be fine. We have a whole ton of extra dev features, and we've already
   backported some newer features, so it should be good.

Q: Why GZDoom 1.8.10? Why not go with the most recent hardware renderer?
A: 1.8.10 is more compatible with a wider variety of hardware with less issues,
   with the downside of being a bag of dicks to wade through on the programming
   end. There are many aspiring developers and modders that simply can't run
   the OpenGL upgrade that came with post-1.8.10 versions, and more importantly
   there are plenty of players as well. Plus, if we purposely don't try and
   stay 1:1 with GZDoom, less chance of getting into any potential upsets with
   Graf or Randy, since they'll still have the feature-superior engine. We
   could also talk it out like reasonable adults, sure, but Just In Case.

Q: But won't the GPL license cause some issues? Needing to distribute the
   source code along with your project might be a problem.
A: It won't cause any problems. In fact, it's practically a non-issue. Either
   download and .zip up the source code on the github or provide a link to the
   github, and you're good to go. The latter is easier, but the former will
   provide more tech-savvy people the ability to see exactly what the engine
   is, how it works, and what a bundle of hacks it is.

Q: Does this run with Brutal Doom?
A: No, stop that.

**********************

CREDITS:

John Carmack
- The forefather of everything. Not just Doom, but for his incredible
generosity and astounding contributions to the video game industry. Without
his work, we would not be where we are now. The entire scene would be a wildly
different place.
Thank you, Carmack.

Randy Heit and Christoph Oelckers
- Creating ZDoom and GZDoom, of which, of course, this is based on (and where
at least 99% of the code comes from). Being amazing programmers and keeping
up with a source port for 20 or so years. Without these people, this port would
have not even been a thought.

JP LeBreton
- Project lead of DECK, one of the big motivators/inspirations behind GLOOME.

marrub
- Did pretty much everything else.

TerminusEst13
- Project director.
