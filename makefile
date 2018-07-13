
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
			debug.cpp \
			pass1.cpp \
			errors.cpp \
			stack.cpp \
			init.cpp \
			cleanup.cpp \
			readhunk.cpp \
			engine.cpp \
			bitmap_font.cpp \
			var_helper.cpp 

main_SRCS = AmosKittens.cpp

objects = $(main_objects:.cpp=.o)
programs= $(main_SRCS:.cpp=.exe)

%.o:		debug.h %.cpp 
	g++ $(warnings) -c -O2 -D__USE_INLINE__ $(@:.o=.cpp) -o $@

%.exe:		%.cpp $(objects)
	g++ $(warnings) -O2 -D__USE_INLINE__ $(@:.exe=.cpp) $(objects) -o $@

all:	 $(programs) $(objects)

clean:
	delete $(programs) $(objects)

.PHONY: revision 
revision:
	bumprev $(VERSION) $(programs)