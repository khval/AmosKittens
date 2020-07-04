
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/retroMode.h>

#include "amoskittens.h"
#include "engine.h"

extern struct Window *My_Window;
extern struct retroEngine *engine ;

typedef struct CompositeHookData_s {
	struct BitMap *srcBitMap; // The source bitmap
	int32 srcWidth, srcHeight; // The source dimensions
	int32 offsetX, offsetY; // The offsets to the destination area relative to the window's origin
	int32 scaleX, scaleY; // The scale factors
	uint32 retCode; // The return code from CompositeTags()
} CompositeHookData;

static struct Rectangle rect;
static struct Hook hook;
static CompositeHookData hookData;


void BackFill_Func(struct RastPort *ArgRP, struct BackFillArgs *MyArgs);
void set_target_hookData( void );

struct Hook BackFill_Hook =
{
	{NULL, NULL},
	(HOOKFUNC) &BackFill_Func,
	NULL,
	NULL
};

static ULONG compositeHookFunc(
			struct Hook *hook, 
			struct RastPort *rastPort, 
			struct BackFillMessage *msg)
 {

	CompositeHookData *hookData = (CompositeHookData*)hook->h_Data;

	hookData->retCode = CompositeTags(
		COMPOSITE_Src, 
			hookData->srcBitMap, 
			rastPort->BitMap,
		COMPTAG_SrcWidth,   hookData->srcWidth,
		COMPTAG_SrcHeight,  hookData->srcHeight,
		COMPTAG_ScaleX, 	hookData->scaleX,
		COMPTAG_ScaleY, 	hookData->scaleY,
		COMPTAG_OffsetX,    msg->Bounds.MinX - (msg->OffsetX - hookData->offsetX),
		COMPTAG_OffsetY,    msg->Bounds.MinY - (msg->OffsetY - hookData->offsetY),
		COMPTAG_DestX,      msg->Bounds.MinX,
		COMPTAG_DestY,      msg->Bounds.MinY,
		COMPTAG_DestWidth,  msg->Bounds.MaxX - msg->Bounds.MinX + 1,
		COMPTAG_DestHeight, msg->Bounds.MaxY - msg->Bounds.MinY + 1,
		COMPTAG_Flags,      COMPFLAG_SrcFilter | COMPFLAG_IgnoreDestAlpha | COMPFLAG_HardwareOnly,
		TAG_END);

	return 0;
}

void BackFill_Func(struct RastPort *ArgRP, struct BackFillArgs *MyArgs)
{
	set_target_hookData();

	if (My_Window)
	{
		register struct RastPort *RPort = My_Window->RPort;

		LockLayer(0, RPort->Layer);
		DoHookClipRects(&hook, RPort, &rect);
		UnlockLayer( RPort->Layer);
	}
}

void set_target_hookData( void )
{
	int image_height = (instance.engine_pal_mode ? instance.video -> height : instance.video -> height * 5 / 6);

 	rect.MinX = My_Window->BorderLeft;
 	rect.MinY = My_Window->BorderTop;
 	rect.MaxX = My_Window->Width - My_Window->BorderRight - 1;
 	rect.MaxY = My_Window->Height - My_Window->BorderBottom - 1;

 	float destWidth = rect.MaxX - rect.MinX + 1;
 	float destHeight = rect.MaxY - rect.MinY + 1;
 	float scaleX = (destWidth + 0.5f) / instance.video -> width;
 	float scaleY = (destHeight + 0.5f) / image_height;

	hookData.srcBitMap = engine->rp.BitMap;
	hookData.srcWidth = instance.video -> width;
	hookData.srcHeight = image_height;
	hookData.offsetX = My_Window->BorderLeft;
	hookData.offsetY = My_Window->BorderTop;
	hookData.scaleX = COMP_FLOAT_TO_FIX(scaleX);
	hookData.scaleY = COMP_FLOAT_TO_FIX(scaleY);
	hookData.retCode = COMPERR_Success;

	hook.h_Entry = (HOOKFUNC) compositeHookFunc;
	hook.h_Data = &hookData;
}



