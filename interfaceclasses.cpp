
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>

#include "debug.h"

#if defined(__amigaos4__) || defined(__amigaos)
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/retroMode.h>
#include <proto/kittyCompact.h>
extern struct RastPort font_render_rp;
#endif

#include "AmosKittens.h"
#include "interfacelanguage.h"
#include "amosstring.h"

#define I_FUNC_UPDATE (void (*)(zone_base*, cmdcontext*, int, int, int, int))
#define I_FUNC_RENDER_API (void (*)(zone_base*))
#define I_FUNC_MOUSE_EVENT (void (*)(zone_base *base,struct cmdcontext *context, int mx, int my, int zid))

extern void edit_render(zone_edit *base);
extern void button_render(zone_button *base);
extern void hslider_render(zone_hslider *base);
extern void vslider_render(zone_vslider *base);
extern void hypertext_render(zone_hypertext *base);
extern void activelist_render(zone_activelist *base);

extern void hslider_mouse_event(zone_hslider *base,struct cmdcontext *context, int mx, int my, int zid);
extern void vslider_mouse_event(zone_vslider *base,struct cmdcontext *context, int mx, int my, int zid);
extern void button_mouse_event(zone_button *base,struct cmdcontext *context, int mx, int my, int zid);
extern void edit_mouse_event(zone_edit *base,struct cmdcontext *context, int mx, int my, int zid);
extern void hypertext_mouse_event(zone_hypertext *base,struct cmdcontext *context, int mx, int my, int zid);
extern void activelist_mouse_event(zone_activelist *base,struct cmdcontext *context, int mx, int my, int zid);


zone_base::zone_base()
{
//	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
//	getchar();

	bzero( &pos, sizeof( struct ivar));
	bzero( &value, sizeof( struct ivar));

	script_action = NULL;
	update = NULL;
	mouse_event = NULL;
	render = NULL;
}

void il_slider_update (struct zone_vslider *base, struct cmdcontext *context, int args, int arg1,int arg2,int arg3)
{
	if (args>0) base -> value.num = arg1;
	if (args>1) base -> total = arg2;
	base -> render( (struct zone_base *) base);
}

void il_button_update (struct zone_button *base, struct cmdcontext *context, int args, int arg1,int arg2,int arg3)
{
	if (args>0) base -> value.num = arg1;
	base -> render( (struct zone_base *) base);
}

void il_hypertext_update (struct zone_hypertext *base, struct cmdcontext *context, int args, int arg1,int arg2,int arg3)
{
	printf("args: %d arg1: %d, arg2: %d, arg3 %d\n",args , arg1,arg2,arg3);
	getchar();

	if (args>0) base -> pos.num = arg1;
	base -> render( (struct zone_base *) base);
}

void il_edit_update (struct zone_hypertext *base, struct cmdcontext *context, int args, int arg1,int arg2,int arg3)
{
	if (args>0) base -> value.num = arg1;
	base -> render( (struct zone_base *) base);
}

void il_activelist_update (struct zone_hypertext *base, struct cmdcontext *context, int args, int arg1,int arg2,int arg3)
{
	printf("args: %d arg1: %d, arg2: %d, arg3 %d\n",args , arg1,arg2,arg3);
	getchar();

	if (args>=0) base -> pos.num = arg1;
	base -> render( (struct zone_base *) base);
}

zone_hslider::zone_hslider()
{
	update = I_FUNC_UPDATE	il_slider_update;
	render = I_FUNC_RENDER	hslider_render;
	mouse_event = I_FUNC_MOUSE_EVENT hslider_mouse_event;
}

zone_vslider::zone_vslider()
{
	update = I_FUNC_UPDATE	il_slider_update;
	render = I_FUNC_RENDER	vslider_render;
	mouse_event = I_FUNC_MOUSE_EVENT vslider_mouse_event;
}

zone_button::zone_button()
{
	update = I_FUNC_UPDATE	il_button_update;
	render = I_FUNC_RENDER	button_render;
	mouse_event = I_FUNC_MOUSE_EVENT button_mouse_event;
}

zone_hypertext::zone_hypertext()
{
	update = I_FUNC_UPDATE	il_hypertext_update;
	render = I_FUNC_RENDER	hypertext_render;
	mouse_event = I_FUNC_MOUSE_EVENT hypertext_mouse_event;
}

zone_edit::zone_edit()
{
	update =I_FUNC_UPDATE		il_edit_update;
	render = I_FUNC_RENDER	edit_render;
	mouse_event = I_FUNC_MOUSE_EVENT edit_mouse_event;
}

zone_activelist::zone_activelist()
{
	update =I_FUNC_UPDATE		il_activelist_update;
	render = I_FUNC_RENDER	activelist_render;
	mouse_event = I_FUNC_MOUSE_EVENT activelist_mouse_event;
}

void iblock::set(bool (*_start_fn)(cmdcontext*, cmdinterface*), void (*_end_fn)(cmdcontext*))
{
	start_fn = _start_fn;
	end_fn = _end_fn;
}

