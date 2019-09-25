

A small document describing how some commands works under Amos Kittens.

**Wait**

	Wait 1/50 of a second (20ms), just like normal AMOS wait, 
	but Wait command will also handle "On Menu Proc" events, 
	two reasons for this, 
	first its bad idea to process events in interpreter as slow down all AMOS programs, 
	second it forces developers to not busy loop there AMOS programs.

**Wait VBL**
	
	Will wait for vertical blanking, 
	but same as Wait command will also process Menu selections, make sure you have Wait or Wait VBL in
	your programs if your using "On Menu Gosub","On Menu Proc" or "On Menu Goto" commands

**Screen Colour**

	Will always return 256 colors, even if opened screens with just 8 colors. this not bug, 
	its becouse Amos Kittens can only use chunky mode, and it is 8bit.



