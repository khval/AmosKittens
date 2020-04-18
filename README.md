# AmosKittens

About
-----
Amos Kittens tries to be 100% compatible AMOS interpreter, so you can download Amos programs from Aminet or cover disks or run AMOS code from ADF disk images. This is the goal of this project. On the Amos command this project try’s to stay 99% compatible, a few command wont work as this project use a bit more modern chunky image format instead of the planar graphic format.

**AmigaOS4.x**
This platform this thing primary developed on, however code is some times tested on other operating systems.

**MorphOS / AROS / AmigaOS3.x**
The API's are whery similar to AmigaOS4.x there for it should be relatively easy to make the changes need to make it compile, however due to having to focus on adding new features, I have no time to keep thing up to date on many different operating systems.

https://github.com/khval/AmosKittens/issues/35

**Linux**
I decided to port some of it to Linux, now interpreter did works under Linux x86 32bit at one time, (it did however not have a graphical display on Linux. Unlike the AmigaOS version). The Linux version is used to find hard to find bugs, and help find and remove stupid mistakes in the code, due to the superior memory protection in Linux. (Currently some files were removed due to being outdated compare to Amiga version of this files.), now after implementing the “include” command, Linux little endian support got officially broken.

https://github.com/khval/AmosKittens/issues/34

**Windows**
Part of the code is sometimes tested in Virtual Studio, some headers does support visual studio compiler.
However, no attempt at compiling a full version has been tried. Now after implementing the “include” command, Windows little endian support got officially broken.

**ATARI**
I have been investigating, if that be possible due link between STOS and AMOS, from what found doing some research on ATARI ST graphics, is that it has lot less colors, and can't display as many colors in higher resolutions. I have not looked at memory limitations but AMOS Kittens is written in C++ code and bit fatter then 680x0 optimized assembler code, I been investigating ATARI Falcon and ATARI TT, this might be able to do it, and it be cool to support it, but there is lot less users on this platforms.

Developer's contributors to Amos kittens
----------------------------------------
If you’re a MorphOS or AROS developer don't let that stop you, but please make directories like OS/MorphOS or OS/AROS, where keep the modified versions of files, plase clone this porject on git so I can merge in changes I like), I like try to avoid "#IFDEF" all over the source code.

Amos developers:
------------------------

Kittens is becoming more and more feature complete, there are few commands that is not working,

Please also read the document describing how Kitten commands works.
Some of commands have enhancements over the original AmosPro commands, but they should be backwards compatible.

https://github.com/khval/AmosKittens/blob/master/commands.md

Kittens does not support the old extension format, but I have recreated some of old extensions in order for your AMOS programs to work, please note that some of the extensions are not fully implemented, download and check the status page on this projects here.

https://github.com/khval/kittyCraft.library

https://github.com/khval/kittyMusic.library

https://github.com/khval/kittyTurbo.library

https://github.com/khval/kittycompact.library


To start a game or program you type:
AmosKittens [dir]/[filename.amos]
  
If window flashes on the screen, you might be runing a simple exsample, without "Wait Key", unlike Amos Pro, Amos kittens don't wait for a key press if the program is done.

If AmosKittens return with a token number, It might be using a command this not supported, or not implemented, check the source code of Amos program your running. Then check “Not yet supported” as bottom of this page. And check "Issues" and "current Status:" on GitHub so see what Amos Kittens support and what not.

Writing Amos Kittens compatible code, I suggest using “Amos Professional X” as this is the most advanced version of Amos Pro right now, there are other versions of Open Source Amos Pro out there, I have not checked this out, don’t know what is fixed,
Amos Kittens was tested whit amos programs written in AMOS PRO 2.0, so no garanties.

Debuging Amos Kittens:
----------------------
To enable debugging edit debug.h file here you find some switches , you can enable or disable, once the file is changed all files will be rebuilt automatic no need to type "make clean"

Amos kittens might stop at any time.. it is possible that getchar(), command is halting the program. This most likely due to a command your using is not yet fully implemented, and program is being paused, so I can see what is going on.

Current status:
---------------

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
* Chapter 21. Screen compaction.
* Chapter 22. Machine level instructions

Partially (or mostly) implemented:

* Chapter 8. Text & Windows
* Chapter 19. Other commands
* Chapter 11. Hardware sprites
* Chapter 12. Blitter objects
* Chapter 13. Object control
* Chapter 14. Amal
* Chapter 16. Menus
* Chapter 17. Sound

Not Implemented:

* Chapter 17. music

AmosPro support:
 * Resource banks
 * The Interface Language (mostly done)
 * Devices and Libraries

Note: 
-----
Unlike AMOS Pro, Amos Kittens probably have something on the stack after the procedure returns, so you most likely can use it as it was a "function".. but that is just a side effect. (I like to keep this because it the way modern programing languages works.)

Note about machine Code (or Amos lowlevel commands)
------------------------------------
Be careful with these commands.

String in Amos kittens are \0 terminated.
VarPtr(a#) will return a address to double not a float.
Call don't support arguments (yet)

On X86 Linux you might run into endieness issues if your using VarPtr(var), to peek into local or global variable.

Not yet supported:
------------------
Some kown commands that was skiped or is currently NOP (no operation)

https://github.com/khval/AmosKittens/issues/26

Orginal Amos Kittens source code can be found here:
--------------------------------------
https://github.com/khval/AmosKittens

Legal:
------
Amos Pro should not be bundled with this package, I do not own it, I have no copyrights to the originals Amos Pro binary’s or AmosPro source code and the software license of Amos Pro is disputed, plus who owns it also not clear to me. So if you do so its on your own risk.

What we know is that Amos Pro was published by Europress they had the copyright, but the source code was released as BSD style license by clickteam, I believe it was also under MIT license for a while, François Lionet later wonted to change License to GPL style license, on Facebook groups, we do not need the source, and we should not use it, for this project. 

There is no such thing as abandonware in copyright laws, just because its not on sale does not mean you have the right to distribute commercial software. what suggest is linking to official sites that have right download software from, or a provide links to sites where you can buy it. 

(But there some thing called public domain, only if someone forgot to include copyright notice, or copyright is over 70 years old, can it end up in public domain. Or if the license is public domain license. Even public domain has restrictions, (US law.))

And finally, I do not have any right to orginal trademarks, like the graphics / logos / designs, I do not own the “Amos The Creator”, "Amos 3D“, "Easy Amos”, “Amos Professional”, or any of these names, so please do not bundle trademarked graphics / designs with this package. I’m not responsible for breach of copyright or trademark. If trademarks are being protected or not is also unclear as this point.

Amos Kittens is only legal because the patents if any have expired, they are limited to 20 years.
"Amos" is a common name like Stave or Fredrik, so should not be a issue to use.

Amos kittens was started after XAMOS author Stephen Harvey-Brooks (Mequa) of JAMOS/XAMOS passed away in 2015, he was responsible for different AMOS like interpreters. XAMOS was another attempt to revive the AMOS Basic language. As no one was carry the flag at time.

Amos Kittens is a direct decedent of the Amos2Ascii project started in 2017, it also depends on earlier work of retroMode.library started in October 2017.

Assets / resources included:
------------------------------------

* A new redesigned mouse pointer it also contains new patterns, in 256 colors.

* A new default resource for Amos Kittens.

Redesigned all GUI elements, it removed all design elements from Amos Pro, 
it does not include any of the interface normally required by Amos Pro.
and all text normally in the resource is removed, should not be needed in Amos Kittens.

(Some of programs written in Amos Basic Language, can have colors hard coded, 
and therefor can ignore the colors in the default resource.)
