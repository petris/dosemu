/* 
 * (C) Copyright 1992, ..., 2003 the "DOSEMU-Development-Team".
 *
 * for details see file COPYING in the DOSEMU distribution
 */

#include <stdlib.h>
#include <stdio.h>
#include <slang.h>

#include "config.h"
#include "emu.h"
#include "video.h"
#include "env_term.h"
#include "mouse.h"
#include "utilities.h"

/* XTERM MOUSE suport by M.Laak */
void xtermmouse_get_event (Bit8u **kbp, int *kbcount)
{
	int btn;
	static int last_btn = 0;
	int x_pos, y_pos;
    
	/* Decode Xterm mouse information to a GPM style event */

	if (*kbcount >= 3) {

		x_pos = (*kbp)[1] - 32;
		y_pos = (*kbp)[2] - 32;
		mouse_move_absolute(x_pos-1, y_pos-1, co, li);
		m_printf("XTERM MOUSE: movement (click follows) detected to pos x=%d: y=%d\n", x_pos, y_pos);

		/* Variable btn has following meaning: */
		/* 0 = btn1 dn, 1 = btn2 dn, 2 = btn3 dn, 3 = btn up */
		btn = (*kbp)[0] & 3;
    
		/* There seems to be no way of knowing which button was released */
		/* So we assume all the buttons were released */
		if (btn == 3){
			if (last_btn) {
				mouse_move_buttons(0, 0, 0);
				m_printf("XTERM MOUSE: button release detected\n");
				last_btn = 0;
			}
		} else {
			switch (btn) {
			case 0:
				mouse_move_buttons(1, 0, 0);
				m_printf("XTERM MOUSE: left button click detected\n");
				last_btn = 1;
				break;
			case 1:
				mouse_move_buttons(0, 1, 0);
				m_printf("XTERM MOUSE: middle button click detected\n");
				last_btn = 2;
				break;
			case 2:
				mouse_move_buttons(0, 0, 1);
				m_printf("XTERM MOUSE: right button click detected\n");
				last_btn = 3;
				break;
			}
		}
		*kbcount -= 3;	/* update count */
		*kbp += 3;

		pic_request(PIC_IMOUSE);
	}
}

static int has_xterm_mouse_support(void)
{
	char *term = getenv("TERM");
	char *term_entry, *xmouse_seq;
	
	if (term == NULL || config.vga || is_console(0))
		return 0;

	term_entry = SLtt_tigetent(term);
	xmouse_seq = SLtt_tigetstr ("Km", &term_entry);
	return xmouse_seq || using_xterm();
}

static int xterm_mouse_init(void)
{
	mouse_t *mice = &config.mouse;

	if (!has_xterm_mouse_support())
		return FALSE;

	mice->intdrv = TRUE;
	mice->type = MOUSE_XTERM;
	mice->use_absolute = 1;

	/* save old highlight mouse tracking */
	printf("\033[?1001s");
	/* enable mouse tracking */
	printf("\033[?9h\033[?1000h\033[?1002h\033[?1003h");	
	fflush (stdout);
	m_printf("XTERM MOUSE: Remote terminal mouse tracking enabled\n");
	return TRUE;
}

static void xterm_mouse_close(void)
{
	/* disable mouse tracking */
	printf("\033[?1003l\033[?1002l\033[?1000l\033[9l");
	/* restore old highlight mouse tracking */
	printf("\033[?1001r");

	m_printf("XTERM MOUSE: Mouse tracking deinitialized\n");
}

static void xterm_mouse_set_cursor(int action, int mx, int my, int x_range, int y_range)
{
	/* do nothing: we cannot affect the mouse cursor in an xterm */
}


struct mouse_client Mouse_xterm =  {
	"xterm",		/* name */
	xterm_mouse_init,	/* init */
	xterm_mouse_close,	/* close */
	NULL,			/* run */
	xterm_mouse_set_cursor	/* set_cursor */
};
