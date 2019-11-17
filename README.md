# AmosKittens

About
-----
Amos Kittens tries to be 100% compatible AMOS interpreter, so you download Amos programs from Aminet or cover disks or run AMOS code from ADF disk images. This is the goal of this project.

**AmigaOS4.x**
This platform this thing primary developed on, however code is some times tested on other operating systems.

**MorphOS / AROS / AmigaOS3.x**
The API's are whery similar to AmigaOS4.x there for it should be relatively easy to make the changes need to make it compile, however due to having to focus on adding new features, I have no time to keep thing up to date on many different operating systems.

**Linux**
I decided to port some of it to Linux, now interpreter did works under Linux x86 32bit at one time, (it did however not have a graphical display on Linux. Unlike the AmigaOS version). The Linux version is used to find hard to find bugs, and help find and remove stupid mistakes in the code, due to the superior memory protection in Linux. (Currently some files were removed due to being outdated compare to Amiga version of this files.)

**Windows**
Part of the code is sometimes tested in Virtual Studio, some headers does support visual studio compiler.
However, no attempt at compiling a full version has been tried.

**ATARI**
I have been investigating, if that be possible due link between STOS and AMOS, from what found doing some research on ATARI ST graphics, is that it has lot less colors, and can't display as many colors in higher resolutions. I have not looked at memory limitations but AMOS Kittens is written in C++ code and bit fatter then 680x0 optimized assembler code, I been investigating Falcon if it might be able to do it, and it be cool to support it, but there is lot less users on that platform.

Developer's contributors to Amos kittens
----------------------------------------
If you’re a MorphOS or AROS developer don't let that stop you, but please make directories like OS/MorphOS or OS/AROS, where keep the modified versions of files, plase clone this porject on git so I can merge in changes I like), I like try to avoid "#IFDEF" all over the source code.

Amos developers:
------------------------

Amos kittens is becoming more and more feature complete, there are few commands that not working, and most extensions have not support. So it bit limited what it can be used for, before posting bug reports please read Notes, some features are not yet implemented, or maybe a bit broken or even incompatible. 

Please also read the document describing how Amos Kitten commands works.
https://github.com/khval/AmosKittens/blob/master/commands.md

To start a game you type:
AmosKittens [dir]/[filename.amos]
  
If window flashes on the screen, you might be runing a simple exsample, without "Wait Key", unlike Amos Pro, Amos kittens don't wait for key press if the, program is done.

If AmosKittens return with a token number, it is likely that Amos program your trying is too advanced for Amos kittens.
See "Issues" and "Current Status:" on GitHub so see what Amos Kittens support and what not.

Writing Amos Kittens compatible code, I suggest using “Amos Professional X” as this most advanced version of Amos Pro right now,
there are other versions of Open Source Amos Pro out there, I have not checked this out, don’t know what is fixed,
Amos Kittens was tested whit amos programs whitens in AMOS PRO 2.0.

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
* Chapter 6. Graphics.
* Chapter 7. Control structures.
* Chapter 8. Maths commands
* Chapter 10. Screens
* Chapter 15. Background graphics.
* Chapter 18. The Keyboard
* Chapter 20. Disk access.
* Chapter 22. Machine level instructions

Partially implemented:

* Chapter 8. Text & Windows
* Chapter 19. Other commands
* Chapter 11. Hardware sprites
* Chapter 12. Blitter objects
* Chapter 13. Object control
* Chapter 14. Amal
* Chapter 16. Menus
* Chapter 21. Screen compaction (only unpack suported).
* Chapter 17. Sound

Not Implemented:

* Chapter 17. music

AmosPro support:
 * Resource banks
 * The Interface Language (mostly done)
 * Devices and Libraries

Note: 
-----
Unlike AMOS Pro, Amos Kittens probably have something on stack after the procedure returns, so you most likely can use it as it was a "function".. but that is just side effect. (I like to keep this because it the way modern programing languages works.)

Note about machine Code (or Amos lowlevel commands)
------------------------------------
Be careful with these commands.

String in Amos kittens are \0 terminated.
VarPtr(a#) will return a address to double not a float.
Call don't support arguments (yet)

On X86 Linux you might run into endieness issues if your using VarPtr(var), to peek into local or global variable.

Not yet supported:
------------------
Recursive procedure calls: due way the local variables are implmented for now, locals was implmented quick and dirty.. 
(in the global list), This part of the code will need to be rewritten to support stack frames, (like normal programing languages does). 

Some kown commands that was skiped or is currently NOP (no operation)

https://github.com/khval/AmosKittens/issues/26

Most of the new stuff in Amos Pro.

Orginal source code can be found here:
--------------------------------------
https://github.com/khval/AmosKittens
