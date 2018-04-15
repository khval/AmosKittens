# AmosKittens

About
-----
Amos Kittens tries to be 100% compatible AMOS interpreter, so you download Amos programs from Aminet or cover disks or run AMOS code from ADF disk images. This is goal of this project.

Now its a simple AMOS interpreter for AmigaOS, 
(more precisely being programmed on AmigaOS4.x, If you’re a MorphOS or AROS developer don't let that stop you, but please make directlry like OS/MorphOS or OS/AROS, where keep the modified versions of files, plase clone this porject on git so I can merge in changes I like)

This program is only uploaded to GITHUB, so developers who is interested in helping out can have look at it.
This program is of little use to AMOS developers at the moment, but hopefully it will be in X months time.

Currently this interpreter dumps out a lot debug information, it will holt the program at critical places waiting for "ENTER" key to continue.

Current status:
---------------

Amos The Creator Manual:

Chapter 4. Basic Principles is implemented.
Chapter 5. String functions is implemented.
Chapter 7. Control structures is implemented.
Chapter 20. Disk access is implemented.

Note: 
-----
Recursive procedures: is not supported, due way the local variables are implmented for now, locals was implmented quick and dirty..

Unlike AMOS Pro, Amos Kittens probably have something on stack after the procedure returns, so you most likely can use it as it was a "function".. but that is just side effect. (I like to keep this because it the way modern programing languages works.)

Commands "Save" and"Load" is not supported atm, will return to this when I start working on graphics.

Not setting values in parameters, some going skip for now, will return to this at later time.
(some modern basic or JavaScript use command nothing or null for this, this is not supported, 
atm, but should not be to hard to implement, it be just different type on the stack, and preset type before data is assigned)

Page 47 in AMOS The Creator Manual, local data statements is not working in Amos Pro, restore, data and read is covered many places in the manual, I return to this later time, personally I think its redundant old way of doing things.

Machine Code 
------------

Command supported:

* Hex$, Bin$

Known bugs:
----------
* Amos Kittens does not clean up after it self..

Not yet supported:
------------------
lots of stuff.
logical "xor"
graphics stuff.
Machine code commands (only two).
All the new stuff in Amos Pro.

Orginal source code can be found here:
--------------------------------------
https://github.com/khval/AmosKittens
