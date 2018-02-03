# Amos2AscII
Translate Token AMOS Source code into ASCII Source code

Amos2AscII works as Amiga Icon tool, can started just clicking on it, or from shell.

About this program:
-------------------
If you feel like it study the files fork this project and make the changes, that's what Open Source is all about is not.
Anyway this project kind a started due lack of sources and information about AMOS was bad, but was not excellent, there were not any examples. Therefore, 
I guess that is my main motivation for this project, besides being able to view Amos source code on AmigaOS4.1 without aid of emulation.

My hope here that this project my fork into other projects like AMOS token editors, transcoders AMOS to C++, 
AMOS interpreters and so on. So project might start or seed in documenting AMOS, to maybe give it new life.

Amos was good beginner's language; it feels like half of my life is in Amos. This is way AMOS has this emotional value to me, and way I think maybe It's worth spending some time on it.

Dependences
-----------
This program uses AmosExtension.library to obtain Command names from Amos extensions.

program will try to read config file from this paths, in this coder.

<path to .amos file>/AMOSPro_Interpreter_Config
amospro:s/AMOSPro_Interpreter_Config
s:AMOSPro_Interpreter_Config
progdir:AMOSPro_Interpreter_Config

program will try the next until it find it.

Unlike AmosPro this program look for AMOSPro_Interpreter_Config in <path to .amos file>,
this is because, *.AMOS token files might not use the same extensions, and mismatch between 
extensions is possible between source codes. amospro:s is next logical place if your read of ADF image or floppy disk.
s: if its installed to HD, and progrdir if AMOSPRO is not found, in this case it fail to load extensions ;-), 
because it wont be able to find any.

Orginal source code can be found here:
--------------------------------------
https://github.com/khval/Amos2AscII
