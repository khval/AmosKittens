
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#ifdef __linux__
#include <stdint.h>
#include "os/linux/stuff.h"
#endif

#include <vector>
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commandsDevice.h"
#include "commands.h"
#include "debug.h"
#include "errors.h"
#include "amosString.h"

extern int last_var;

extern std::vector<struct kittyDevice> deviceList;

struct kittyDevice *kFindDevice( int id )
{
	unsigned int n;

	for (n=0;n<deviceList.size();n++)
	{
		if (deviceList[n].id == id ) return &deviceList[n];
	}

	return NULL;
}

int kFindDeviceIndex( int id )
{
	unsigned int n;

	for (n=0;n<deviceList.size();n++)
	{
		if (deviceList[n].id == id ) return n;
	}

	return -1;
}

void kCloseDevice( struct kittyDevice *dev )
{
	if ((dev->error == 0)&&(dev -> io))
	{
		if (dev -> sendt)
		{
			printf("try abort\n");
			AbortIO( (struct IORequest *) dev -> io);
			printf("try wait\n");
			WaitIO( (struct IORequest *) dev -> io);
		}

		printf("try to close\n");
		CloseDevice( dev -> io );
	}

   	if (dev->io)
	{
		printf("remove io request\n");
		if (dev->io) FreeSysObject ( ASOT_IOREQUEST, (struct IORequest *) dev->io);
		dev->io = NULL;
	}

	if (dev->port) 
	{
		printf("free msg port\n");
		FreeSysObject( ASOT_PORT, dev->port);
		dev->port = NULL;
	}
}

void kFreeDevice( int id )
{
	int index;
	struct kittyDevice *dev = NULL;

	index = kFindDeviceIndex( id );

	printf("id %d, index %d\n",id,index);

	if (index>-1)
	{
		dev = &deviceList[index];
		if (dev) kCloseDevice( dev );
		deviceList.erase(deviceList.begin()+index);
	}
}

char *_deviceDevOpen( struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 5:
				{
					int id = getStackNum( stack-4 );
					struct stringData *name = getStackString( stack-3  );
					int ioLength = getStackNum( stack-2 );
					int unitNumber = getStackNum( stack-1 );
					int flags = getStackNum( stack );
					struct kittyDevice dev;

					kFreeDevice( id );

					printf("Dev Open %d,%s,%d,%d,%d\n",id,&(name->ptr),ioLength,unitNumber,flags);

					dev.id = id;
					dev.sendt = false;
					dev.error = -1;
					dev.io = NULL;

					dev.port = (struct MsgPort *) AllocSysObjectTags( ASOT_PORT, TAG_END) ;
					if (dev.port)
					{
						printf("have port\n");

						dev.io = (struct IORequest *) AllocSysObjectTags ( ASOT_IOREQUEST, 
								ASOIOR_ReplyPort, dev.port, ASOIOR_Size, ioLength, TAG_END );

						if (dev.io)
						{
							printf("have io\n");

							dev.error = OpenDevice( &(name->ptr), unitNumber, dev.io, flags );
							deviceList.push_back(dev);
						}
					}

					popStack( stack - data -> stack );

					if (dev.error != 0) 
					{
						kCloseDevice( &dev );
						setError(142,data->tokenBuffer);
					}

					setStackNum( 0 );
					return NULL;
				}
				break;
		default:
				popStack( stack - data -> stack );
				setError(22,data-> tokenBuffer);
				break;
	}

	setStackNum( 0 );	
	return NULL;
}

char *deviceDevOpen(struct nativeCommand *device, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _deviceDevOpen, tokenBuffer );
	return tokenBuffer;
}

char *_deviceDevClose( struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack +1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			kFreeDevice( getStackNum( stack) );
			break;
		default:
			popStack( stack - data -> stack );
			setError(23,data-> tokenBuffer);
			break;
	}

	return NULL;
}

char *deviceDevClose(struct nativeCommand *device, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _deviceDevClose, tokenBuffer );
	return tokenBuffer;
}

char *_deviceDevBase( struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack +1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			{
				int id = getStackNum( stack );
				struct kittyDevice *dev;

				dev = kFindDevice( id );
				if (dev)
				{
					setStackNum( (int) dev -> io );
					return NULL;
				}
				else
				{
					setError( 141 , data -> tokenBuffer );	// Device not opened
					return NULL;
				}
			}
		default:
			popStack( stack - data -> stack );
			setError(23,data-> tokenBuffer);
			break;
	}

	return NULL;
}

char *deviceDevBase(struct nativeCommand *device, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _deviceDevBase, tokenBuffer );
	return tokenBuffer;
}

char *_deviceDevCheck( struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack +1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			{
				int id = getStackNum( stack );
				struct kittyDevice *dev;

				dev = kFindDevice( id );
				if (dev)
				{
					
					setStackNum( (int) CheckIO( dev -> io ));
					return NULL;
				}
				else
				{
					setError( 141 , data -> tokenBuffer );	// Device not opened
					return NULL;
				}
			}
		default:
			popStack( stack - data -> stack );
			setError(23,data-> tokenBuffer);
			break;
	}

	return NULL;
}

char *deviceDevCheck(struct nativeCommand *device, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _deviceDevCheck, tokenBuffer );
	return tokenBuffer;
}


char *_deviceDevDo( struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack +1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			{
				int id = getStackNum( stack-1 );
				int ioCmd = getStackNum( stack );
				struct kittyDevice *dev;

				dev = kFindDevice( id );
				if (dev)
				{
					if (dev ->io)
					{
						dev -> io->io_Command = ioCmd;
						DoIO( dev -> io );
					}
				}
			}

			break;
		default:
			popStack( stack - data -> stack );
			setError(23,data-> tokenBuffer);
			break;
	}

	return NULL;
}
char *deviceDevDo(struct nativeCommand *device, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _deviceDevDo, tokenBuffer );
	return tokenBuffer;
}

char *_deviceDevSend( struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack +1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			{
				int id = getStackNum( stack-1 );
				int ioCmd = getStackNum( stack );
				struct kittyDevice *dev;

				dev = kFindDevice( id );
				if (dev)
				{
					if (dev ->io)
					{
						dev -> io->io_Command = ioCmd;
						SendIO( dev -> io );
						dev -> sendt = true;
					}
				}
			}
			break;
		default:

			setError(23,data-> tokenBuffer);
			break;
	}

	popStack( stack - data -> stack );
	return NULL;
}

char *deviceDevSend(struct nativeCommand *device, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _deviceDevSend, tokenBuffer );
	return tokenBuffer;
}

char *_deviceDevAbort( struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack +1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			{
				int id = getStackNum( stack );
				struct kittyDevice *dev;

				dev = kFindDevice( id );
				if (dev)
				{
					if (dev -> sendt)
					{
						dev -> sendt = false;
						if (dev ->io)
						{
							AbortIO( dev -> io );
							WaitIO( dev -> io );
						}
						return NULL;
					}
				}

				setError( 141 , data -> tokenBuffer );	// Device not opened
				return NULL;
			}

		default:
			popStack( stack - data -> stack );
			setError(23,data-> tokenBuffer);
			break;
	}

	return NULL;
}

char *deviceDevAbort(struct nativeCommand *device, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _deviceDevAbort, tokenBuffer );
	return tokenBuffer;
}
