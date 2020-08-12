This result of trying to run different .amos files, on Amos Kittens, there is stuff missing, the list is to keep track of problems with etch .amos program, so we can fix problems we have.

**Amal_Editor.AMOS**

    (c) 1990-1992 Europress Software Ltd

    (works maybe, takes lot time to start in debug mode, it does not work in AmosPRO some programing error some where.)

**Amal_Editor_Fixed.AMOS**

    (c) 1990 Mandarin / Jawx

    It does load, but does not render correct, after running the program, AmigaOS starts crashing…

**Disc_Manager.AMOS**

    Missing token 0x258C - command "@apml@"
    +Synyax error on line 1677
    if I'm not mistaken this token means part of program is compiled, so can't support that.

**Font8x8_Editor.AMOS**

    Works it starts "AMOSPro_Producitivity2:Font8x8_editor.amos"

**IFF_Compactor.AMOS**

    Works it starts "AMOSPro_Producitivity2:IFF_Compactor.AMOS"

**Menu_Editor.AMOS**

    Works it starts "AMOSPro_Producitivity1:Menu_editor.amos"

**Object_Editor.AMOS**

    ERROR: Address error at line number: 1184

    Line " A=Start(BPIC)+Deek(BPIC)+2*(N-1)) "

    BPIC is 10, bank 10 not found. 
    
    (This looks like a bug in Object_Editor.AMOS, not an Amos Kittens Bug.)

**Resource_Bank_Maker.AMOS**

    Interface: Is buggy.

    * Interface User defined instruction are not always found on actions. (pressing buttons)

    * Some buttons are rendered at the wrong position, problem with XA,XB,YA,YB.

    Pressing QUIT causes NULL pointer, programs sets error, and program terminates, safely.

    Program uses sload, (sload is for loading sample so its not supported yet.), 
    so program can’t load banks yet.

**Sample_Bank_Maker.AMOS**

    Crashes as soon as you try to load a sample, maybe something wrong on the stack.
    The Amos program does use some 680x0 routines, I have no idea if they are compatible, or not.
