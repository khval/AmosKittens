# AmosKittens

About
-----
Simple AMOS interpreter.

This program is only uploaded to GITHUB, so developers who is interested in helping out can have look at it.
This program is of no use to AMOS developers at the moment, but hopefully it will be in X months time.

Current status:
---------------
This commands are supported:

Dim,Print,Input,Goto,If,Then,Else,End If,Do,Loop,repeat,until,False,True,While,Wend

And some math operations works: 

pulss, minus, muls, div.

Logical opertations supported:

equal, not equal, less, more, 

String commands supported:
--------------------------
Left$(), Mid$(), Right$(), Instr(), Flip$, Space$, Upper$, Lower$

Known bugs:
----------
* Amos Kittens does not clean up after it self..
* Horrible stack handling, should clean up stack, after reading args, not just decrement stack.
* Should double check for, double use of pointer numbers is stacks, string functions. (not freed so not problem yet.)
* logcial operations don't compare strings.

Not yet supported:
------------------
lots of stuff.
and, or, strings commands.
procedures
gosub
disk stuff.
graphiCS stuff.

Orginal source code can be found here:
--------------------------------------
https://github.com/khval/AmosKittens
