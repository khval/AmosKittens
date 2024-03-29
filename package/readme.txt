Amos Kittens
------------

Amos Kittens tries to be 100% compatible AMOS interpreter, so you can download Amos programs from 
Aminet or cover disks or run AMOS code from ADF disk images. This is the goal of this project. 
The Amos Kittens commands tries to stay 99% compatible, a few commands won't work as this 
project use a bit more modern chunky image format instead of the planar graphic format used on Amiga 500 to 4000.

AmigaOS4.x is the platform this thing primary developed on, 
however code is sometimes tested on other operating systems.

There are tree ways you use Amos Kittens.

* By double click on Amos Kittens icon.

You can also double click icon and select an Amos game or program.

* As workbench tool for a workbench project icon.

Change the "Default tool:" on your Amos project icon to AmosKittens.
set Execution Mode/Run as: Workbench.

* From the command line interface "CLI/AmigaDOS/Shell/KingCon"

AmosKittens [dir]/[filename.amos]
For more information see readme on github:

(if you start Amos Kittens in bad, why Amos game or program might struggle to find assets.)

https://github.com/khval/AmosKittens/edit/master/README.md

User interface

F12 to switch between Fullscreen and window mode
F11 to switch between NTSC and PAL
Joystick is default to keypad, can be changed in config

Graphic modes supported:

lowres / hires / interlaced / HAM6, HAM8, 8bit graphics, 
double buffer, Dual playfields, rainbow effects, color cycling, etc supported.

It has extension support, but this are not original extensions.

The main difference between the 
new extensions and the old extensions are:

* Proper garbage collection system.
* Written in C code.
* Can be shared between instances of the interpreter.

Stuff not supported:

Planar / lowlevel peeking & poking into screens, sprites, bobs.

Changes:

27.09.2022 version 0.9 (optimzing & bug fixes)

 * Speed up “char *skip_next_cmd(…)”
 * Some changes were not committed before.
 * Optimized Amos Kittens stack by removing state from the stack.
 * Improvement I debugging.
 * Token relocation for operator optimization.
 * Math order operator optimization.
 * Command Dir$ fixes.
 * Fixed bug in Eof command
 * Command “Resume Next” implemented.
 * Command Input$(fd,length) fixed. (And some minor fixes.)

18.09.2022 version 0.8 (Bug fixes.)

 * A complicated math bug found and fixed.
 * fixed a bug in the "Screen Mode" command.

07.09.2022 version 0.8

17 commits to kittyMusiCraft.library (st load, st play, st stop, st pause, st resume)
7 commits to ptreplay.library (PTGetSample, PTPatternData)
4 commits to kittyMusic.library (track load, track play, track stop)
1 commit to RetroMode.library (fixed polygon)

 * Remove the required version 53, for extensions.
 * Fixed sticky key problem, due to missed key up event.
 * Don't allow BLoad to blow up if, bank is not found.
 * Reserve AS - commands fixes
 * Fixed DirFirst$(filename$) problem.
 * Make sure zones are freed, on exit.
 * Mouse Limit, X Mouse, Y Mouse problems fixed.
 * Command Set Font bug fixed.
 * Fixed broken iconify support.
 * Added error requestors.
 * Open requester on errors.
 * Added version string to exe file.
 * Minor improvements to Polygon
 * Improvements to packaging.
 * Fullscreen now has back border.
 * Fixed joystick / joypad, offsets of buttons / axis 

30.06.2020 version 0.7, initial release.

This was first version. It only supported some extensions where supported.

Music (only sound, not actual music), Compact, Trubo plus, Craft.

At this time AMAL, ANIM, Basic where pretty mature support.
While the "Interface language" commands where as proof of concept.

Best Regards
Kjetil Hvalstrand / LiveForIt

