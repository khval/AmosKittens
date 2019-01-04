
# Version to build
VERSION = 1

#CC     = $(CROSS_COMPILE)gcc 
#CXX    = $(CROSS_COMPILE)c++ 
#AS     = $(CROSS_COMPILE)as 
#LD     = $(CROSS_COMPILE)ld 
#RANLIB = $(CROSS_COMPILE)ranlib 
#RM     = delete
# RM     = rm

warnings = -wall -Wuninitialized

objects_dir = objs_amoskittens/

main_objects = commands.cpp \
			commandsData.cpp \
			commandsString.cpp \
			commandsMath.cpp \
			commandsBanks.cpp \
			commandsErrors.cpp \
			commandsMachine.cpp \
			commandsGfx.cpp \
			commandsScreens.cpp \
			commandsText.cpp \
			commandsKeyboard.cpp \
			commandsSound.cpp \
			commandsDisc.cpp \
			commandsObjectControl.cpp \
			commandsHardwareSprite.cpp \
			commandsBlitterObject.cpp \
			commandsBackgroundGraphics.cpp \
			commandsAmal.cpp \
			commandsMenu.cpp \
			commandsFonts.cpp \
			commandsRead.cpp \
			commandsOn.cpp \
			AmalCompiler.cpp \
			AmalCommands.cpp \
			ext_compact.cpp \
			ext_turbo.cpp \
			debug.cpp \
			pass1.cpp \
			errors.cpp \
			stack.cpp \
			init.cpp \
			cleanup.cpp \
			readhunk.cpp \
			engine.cpp \
			bitmap_font.cpp \
			var_helper.cpp \
			os4_joystick.cpp \
			channel.cpp \
			amal_object_bob.cpp \
			amal_object_sprite.cpp \
			amal_object_screen_display.cpp \
			amal_object_screen_offset.cpp \
			screen_helper.cpp \
			spawn.cpp


objects_in_dir = ${main_objects:%.cpp=${objects_dir}%.o}
main_SRCS = AmosKittens.cpp

objects = $(main_objects:.cpp=.o)
programs= $(main_SRCS:.cpp=.exe)

all:	 $(objects_in_dir) $(programs) 

$(objects_dir)%.o:	%.cpp debug.h 
	g++ $(warnings) -c -O2 -D__USE_INLINE__ $(@:${objects_dir}%.o=%.cpp) -o $@

ccmosKittens.exe:	AmosKittens.cpp	 $(objects_in_dir)
	g++ $(warnings) -O2 -D__USE_INLINE__ $(@:.exe=.cpp) $(objects_in_dir) -o $@

%.exe:		%.cpp $(objects_in_dir)
	g++ $(warnings) -O2 -D__USE_INLINE__ $(@:.exe=.cpp) $(objects_in_dir) -o $@

clean:
	@delete $(programs) $(objects_in_dir)

.PHONY: revision 
revision:
	bumprev $(VERSION) $(programs)