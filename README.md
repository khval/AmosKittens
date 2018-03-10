# AmosKittens

About
-----
Simple AMOS interpreter.

This program is only uploaded to GITHUB, so developers who is interested in helping out can have look at it.
This program is of no use to AMOS developers at the moment, but hopefully it will be in X months time.

Current status:
---------------

This commands are supported:

* Dim,Print,Input,Goto,If,Then,Else,End If,Do,Loop,repeat,until,False,True,While,Wend


And some math operations works: 

* Pulss, Minus, Muls, Div, power, Inc, Dec, Add

Logical opertations supported:

* equal, not equal, less, more, 

String
------

commands supported:

* Left$(), Mid$(), Right$(), Instr(), Flip$, Space$, Upper$, Lower$, String$, Chr$, Asc, Len, Val, Str$

Basic Principles
----------------

Command supported:

* Procedure, End Proc, Shared, Global, Pop Proc, Param, Param#, Param$, Reserve As Work, Reserve As Chip Work, Reserve As Data, Reserve As Chip Data, List Bank, Erase, Start, Length, BSave, BLoad

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
logical "and", "or", "xor"
disk stuff.
graphics stuff.
Machine code commands (only two).

Orginal source code can be found here:
--------------------------------------
https://github.com/khval/AmosKittens
