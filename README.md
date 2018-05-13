# AmosKittens

About
-----
Amos Kittens tries to be 100% compatible AMOS interpreter, so you download Amos programs from Aminet or cover disks or run AMOS code from ADF disk images. This is goal of this project. Now its a simple AMOS interpreter for AmigaOS, (more precisely being programmed on AmigaOS4.x.

Developer's contributors to Amos kittens
----------------------------------------
If you’re a MorphOS or AROS developer don't let that stop you, but please make directories like OS/MorphOS or OS/AROS, where keep the modified versions of files, plase clone this porject on git so I can merge in changes I like), I like try to avoid "#IFDEF" all over the source code.

Amos developers:
------------------------
You can start using this thing for small text adventures, quiz games, or something like that. Right now .AMOS filename is hard coded into AmosKittens.cpp file, so some changes will be needed to made to allow it load from .AMOS files as CLI arguments, or WBStartup message.

Before posting bug reports please read Notes, some features are not yet implemented, or maybe a bit broken or even incompatible. 

Debuging Amos Kittens:
----------------------
To enable debugging edit debug.h file here you find some switches , you can enable or disable, once the file is changed all files will be rebuilt automatic no need to type "make clean"

Amos kittens might stop at any time.. it is possible that getchar(), command is halting the program. This most likely due to a command your using has not yet fully implemented, and program is being paused, so I can see what is going on.

Current status:
---------------

Amos The Creator Manual:

Implemented:

* Chapter 4. Basic Principle .
* Chapter 5. String functions.
* Chapter 7. Control structures.
* Chapter 8. Maths commands
* Chapter 20. Disk access.
* Chapter 22. Machine level instructions

Partially implemented:

* Chapter 6. Graphics.
* Chapter 10. Screens
* Chapter 18. The Keyboard
* Chapter 19. Other commands

Not Implemented:

* Chapter 8. Text & Windows
* Chapter 11. Hardware sprites
* Chapter 12. Blitter objects
* Chapter 13. Object control
* Chapter 14. Amal
* Chapter 15. Background graphics.
* Chapter 16. Menus
* Chapter 17. Sound and music
* Chapter 21. Screen compaction.

Note: 
-----
Recursive procedures: is not supported, due way the local variables are implmented for now, locals was implmented quick and dirty..

Unlike AMOS Pro, Amos Kittens probably have something on stack after the procedure returns, so you most likely can use it as it was a "function".. but that is just side effect. (I like to keep this because it the way modern programing languages works.)

Commands "Save" and"Load" is not supported atm, will return to this when I start working on graphics.

Page 47 in AMOS The Creator Manual, local data statements is not working in Amos Pro, restore, data and read is covered many places in the manual, I return to this later time, personally I think its redundant old way of doing things.

Note Machine Code 
-----------------
Ba careful with these commands.

String in Amos kittens are \0 terminated.
Amos kittens don’t have length in front of strings, VarPtr(a$)-2 won't work, use Len(a$)
VarPtr(a$), will return a address to double not a float.
Call don't support arguments (yet, arguments has to be converted from Amos Kitty to Amos Pro, and freed on command exit)

Not yet supported:
------------------
logical "xor"
All the new stuff in Amos Pro.

Orginal source code can be found here:
--------------------------------------
https://github.com/khval/AmosKittens
