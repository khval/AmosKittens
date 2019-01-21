
extern struct MsgPort	*joystick_msgport;

struct joystick
{
	void				*controller ;

#ifdef __amigaos4__	
	AIN_DeviceHandle	*handle;
#endif

#ifdef __linux__	
	void	*handle;
#endif

	int id;
	int num;
	int res;
};

extern int found_joysticks;
extern int used_joysticks;
extern struct joystick joysticks[4];

void init_joysticks();
void close_joysticks();
void joy_stick(int joy,void *controller);

