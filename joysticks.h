
#ifdef amigaos4

extern struct MsgPort	*joystick_msgport;

struct joystick
{
	void				*controller ;
	AIN_DeviceHandle	*handle;
	int id;
	int num;
	int res;
};

extern int found_joysticks;
extern int used_joysticks;
extern struct joystick joysticks[4];

#else

#endif

void init_joysticks();
void close_joysticks();
void joy_stick(int joy,void *controller);

