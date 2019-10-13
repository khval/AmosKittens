

A small document describing how some commands works different under Amos Kittens.

**Wait**

	Wait 1/50 of a second (20ms), just like normal AMOS wait, 
	but Wait command will also handle "On Menu Proc" events, 
	two reasons for this, 
	first its bad idea to process events in interpreter as it slows down all AMOS programs, 
	second it forces developers to not busy loop there AMOS programs.

**Wait VBL**
	
	Will wait for vertical blanking, 
	but same as Wait command will also process Menu selections, make sure you have Wait or Wait VBL in
	your programs if your using "On Menu Gosub","On Menu Proc" or "On Menu Goto" commands

**Screen Colour**

	Will always return 256 colors, even if you have opened screens with just 8 colors. this not bug, 
	it's becouse Amos Kittens can only use chunky mode, and it is 8bit.
	
**Load Iff**

	Will load .png/.jpg any file supported by datatype system on AmigaOS, 
	true color images are converted into 8bit grayscale images, or 8bit floyd dittered images.
	(in the future this command can load in images as truecolor), loading option will be in global variabel.
	
**Save Iff**

	Will save any to any image format supported by datatype system, should default to IFF, if no file extension is set.

**Unpack**

	This command will compress a screen or part of screen into a bank, unlike Amos Pro that is not limited 6bit graphics,
	this command also support 8bit graphics on Amos Kittens.
	
