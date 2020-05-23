
#define joy_up 1
#define joy_down 2
#define joy_left 4
#define joy_right 8

#ifdef __amigaos4__
#include <proto/Amigainput.h>
#endif

extern struct MsgPort	*joystick_msgport;

enum
{
	joy_none,
	joy_usb,
	joy_keyb,
	joy_serial
};

struct joystick
{
	void				*controller ;

	int type;			// keyboard / usb / serial / ...
	int type_count;

#ifdef __amigaos4__	
	AIN_DeviceHandle	*handle;
#endif

#ifdef __linux__	
	void	*handle;
#endif

	int device_id;
	int port;
	int res;
	int connected;
};

extern int joy_keyboard_index;

extern int found_joysticks;
extern int used_joysticks;
extern struct joystick joysticks[4];

void init_usb_joysticks();
void close_joysticks();
void joy_stick(int joy,void *controller);

