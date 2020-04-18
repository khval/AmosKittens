This result of trying to run different .amos files, on Amos Kittens, there is stuff missing, the list is to keep track of problems with etch .amos program, so we can fix problems we have.

Error: undefined procedure --> Normaly there is a missing token (mssing command), need to check again, too see..


**Amal_Editor.AMOS**

(works maybe, takes lot time to start in debug mode, it does not work in AmosPRO some programing error some where.)

**Amal_Editor_Fixed.AMOS**

Not working, Somehow Amos kittens failed to load the Amos program code fully, under investigation.

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

Stops at "Screen Base" command, that is not implemented.

**Resource_Bank_Maker.AMOS**

Failes to open a dialog window, I tried to debug this one before.

**Sample_Bank_Maker.AMOS**

Syntax error on line 1102
