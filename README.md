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
unlike AMOS Pro, Amos Kittens probably have something on stack after the function returns, so you most likely can use it as it was a "function".. but that is just side effect. 

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
