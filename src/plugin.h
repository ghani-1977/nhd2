/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef TPLUGIN_H
#define TPLUGIN_H

typedef struct _PluginParam
{
	const char          * id;
	char                * val;
	struct _PluginParam * next;

} PluginParam;

typedef int (*PluginExec)( PluginParam * par );
/* das dlsym kann auf PluginExec gecastet werden */

/* NOTE : alle Plugins haben uebergangs-weise neue und alte schnittstelle */
/* neues Symbol : plugin_exec */
/* es muessen nur benutzte ids gesetzt werden : nicht genannt = nicht benutzt */

/* fixed ID definitions */
#define	P_ID_FBUFFER		"fd_framebuffer"
#define	P_ID_RCINPUT		"fd_rcinput"
#define	P_ID_LCD		"fd_lcd"
#define	P_ID_NOPIG		"no_pig"		// 1: plugin dont show internal pig
#define P_ID_VTXTPID		"pid_vtxt"
#define P_ID_PROXY		"proxy"			// set proxy for save into highscore
#define P_ID_PROXY_USER		"proxy_user"		// format "user:pass"
#define P_ID_HSCORE		"hscore"		// highscore-server (as url)
#define P_ID_VFORMAT		"video_format"		// videoformat (0 = auto, 1 = 16:9, 2 = 4:3)
#define P_ID_OFF_X		"off_x"			// screen-top-offset x
#define P_ID_OFF_Y		"off_y"			// screen-top-offset y
#define P_ID_END_X		"end_x"			// screen-end-offset x
#define P_ID_END_Y		"end_y"			// screen-end-offset y
#define	P_ID_RCBLK_ANF		"rcblk_anf"		// Key-Repeatblocker Anfang
#define	P_ID_RCBLK_REP		"rcblk_rep"     	// Key-Repeatblocker Wiederholung
#define P_ID_LFBUFFER		"lfb_framebuffer"	// framebuffer pointer
#define P_ID_XRESFBUFFER	"xres_framebuffer"	// xres framebuffer
#define P_ID_YRESFBUFFER	"yres_framebuffer"	//yres framebuffer
#define P_ID_STRIDEFBUFFER	"stride_framebuffer"	// stride framebuffer
#define P_ID_MEMFBUFFER		"mem_framebuffer"	// mem available framebuffer


typedef enum plugin_type
{
	PLUGIN_TYPE_DISABLED = 0,
	PLUGIN_TYPE_GAME     = 1,
	PLUGIN_TYPE_TOOL     = 2,
	PLUGIN_TYPE_SCRIPT   = 3,
}
plugin_type_t;

#endif
