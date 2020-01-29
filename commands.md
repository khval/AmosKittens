

A small document describing how some commands works different under Amos Kittens.
If you like to write programs for Amos Kittens, get the Amos Pro manual, 
most of the commands are the same and should work the same, 
only the commands that are different are listed here.
not implmented commands not listed, dummy commands not listed, 
see other docs like issue for more info.

**AMOS to Back**

	Insted of just hiding Amos Kittens, it will iconify Amos kittens on AmigaOS4.x, 
	in the future you can expect same  behavior on AmigsOS3.x and AROS, MorphOS, 
	(In the future if Amos Kittens is ported to Windows, 
	this might result in minimized window to start bar.)

	You should not use this command to hide opening screen use command "view off", 
	use command "View On" when your done.

**AMOS to Front**

	Uniconify Amos Kittens, and open the window.

**Colour**

	To set value

	Colour <num>,<value>
	
	To get value
	
	value = Colour(num)
	
	num is colour number.
	value is RGB value.

	can be used in standard AMOS ECS/OCS style.
	to set color to white.

	colour 0,$FFF

	or can be used in ARGB format.

	color 0,$FFFFFFFF
	
	You can only get value in OCS/ECS format.
	
**Colour back**

	Sets the border color, the color outside of the screen.
	works in OCS/ECS format or ARGB format.

	Colour back $FFF

	or

	Colour back $FFFFFFFF

**Palette**

	Command used to set more then once colour
	can be used two ways old OCS format, or ARGB format.

	Palette $000,,$FFF
	Palette $FF000000,,$FFFFFFFF

	Note this two lines does the same thing.
	sets colour 0 and colour 2, colour 1 is ignored as no value given.

**Disc Info$**

	This command return string "Volume name:size free",
	unlike Amos Pro this command does support large hard drives over 2Gbytes.

**Dfree**

	This command returns free bytes on current drive/partition. Amos Kitten follow AMOS pro 
	standards when value is under 2Gbytes, if the value is above 2Gbytes then this command will 
	return size above 2Gbytes as double, (Extremely large numbers can get truncated).

	Use command "Disk Info$" command to get correct number of bytes free (if drive is extremly large).

**Screen Colour**

	Will always return 256 colors, even if you have opened screens with just 8 colors. this not bug, 
	it's becouse Amos Kittens can only use chunky mode, and it is 8bit.

**Screen Open**

	Screen open <num>,<width>,<height>,<colors>,<mode>
	
	<num> is screen number 0 to 7
	<width> screen width
	<height> screen height
	<colors> 
		value 1-256 is 256 colors.
		value 4096 is HAM
		value 16777216 reserved for 24bit ARGB...		
		(playing with idea of using -6 for HAM6 and -8 for HAM8)

	<mode> can be Lowres,hires,laced
	(laced is always promoted to dbpal, dbntsc like modes.)

	ham8 can be set by setting mode to 64, but this can change.

**Sprites**

	Just like AmosPro sprites, but unlike AMOS Pro that uses hardware sprites and where there are 
	color restrictions, this sprites are 256 colors, and so you do not need to worry 
	about sprites not being displayed.

**Load Iff**

	Will load .png/.jpg any file supported by datatype system on AmigaOS, 
	true color images are converted into 8bit grayscale images, or 8bit floyd dittered images.
	(in the future this command can load in images as truecolor), loading option will be in global variabel.
	IFF images with HAM6 format also supported.
	HAM8 images also supported.
	
**Save Iff**

	Will save to any image format supported by amigaos datatype system, it should default to IFF, 
	if no file extension is set. (on other operating system this might be limited to only few file formats.)

**Spack**

	This command will compress a screen or part of screen into a bank, 
	unlike Amos Pro that is not limited 6bit graphics,
	this command also support 8bit graphics on Amos Kittens.
	this command will count the number of colors on the screen before compressing the image,
	if its 64 colors or less, it will use AMOS packed format, if more then 64 colors 
	it will exstend the color platte table. 

**Unpack**

	This command support 1bit to 8bit compressed banks, including HAM6.
	
**Joy**

	Almost the same as Amos Pro, but supports more buttons.
	
	bit 0 - value 1 - joystick up
	bit 1 - value 2 - joystick down
	bit 2 - value 4 - joystick left
	bit 4 - value 8 - joystick right
	bit 5 - value 16 - joystick button 1
	bit 6 - value 32 - joystick button 2
	bit 7 - value 64 - joystick button 3
	bit 8 - value 128 - joystick button 4
	bit 9 - value 256 - joystick button 5
	...
	
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
		
**Include**

	include "path/file.amos"
	include "volume:path/file.amos"

	This command is called before .amos program is started, does not take variabels, only text string.	
	
	AMOSPRO can have problems finding files, if no direct path is used.
	AMOS Kittens will however try hard to find the files, if for example
	Amos kittens is started from command like this:
	"AmosKittens.exe path/file.amos" then Amos Kittens will try to look for includes in "path/"
	if however a direct path is set in the include, then Amos kittens should use a direct path.

**Limit bob**

	Limit Bob 
	
	remove limit from all bobs
	
	Limit bob n
	
	remove limit from bob n, (this one is not supported by Amos Pro)
	
	limit bob x,y to x,y
	
	set Limit on all bobs, bob should exist before using this command.
	(On "Amos Kittens" it already does, on "Amos Pro" it does not)
	
	Limit bob n,x,y to x,y
	
	set Limit on bob "N", bob should exist before using this command.
	(On "Amos Kittens" it already does, on "Amos Pro" it does not)

**Amalerr**

	Just like Amalerr in Amos Basic.
	
	S$="F R0=0 T 20; P ; N R0; bad code"
	trap Amal 0,S$
	print mid$(s$,Amalerr)

	More or less return error in the string, because Amos kittens strips all lower chars, 
	before trying to execute the scripts, this one can return the wrong position.
