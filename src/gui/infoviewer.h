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


#ifndef __infoview__
#define __infoview__

#include <sectionsdclient/sectionsdclient.h>

#include <driver/rcinput.h>
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <system/settings.h>
#include "widget/menue.h"
#include <gui/widget/progressbar.h>
#include <string>


class CInfoViewer
{
	private:
		void Init(void);
		CFrameBuffer * frameBuffer;
		
		bool           gotTime;
		bool           recordModeActive;
#ifndef SKIP_CA_STATUS
		bool           CA_Status;
#endif
	
		int            InfoHeightY;
		int            InfoHeightY_Info;
		
		bool         showButtonBar;
		
		int            BoxEndX;
		int            BoxEndY;
		int            BoxStartX;
		int            BoxStartY;
		int            ButtonWidth;

		int            ChanWidth;
		int            ChanHeight;
		int            ChanInfoX;
		
		int BoxHeight;
		int BoxWidth;
		
		int satNameWidth;
		
		// dimensions of radiotext window
#if ENABLE_RADIOTEXT		
		int             rt_dx;
		int             rt_dy;
		int             rt_x;
		int             rt_y;
		int             rt_h;
		int             rt_w;
#endif		

		int		asize;

		int ChanLogoX;
		int ChanLogoY;
		int PIC_X;
		int PIC_Y;

		CSectionsdClient::CurrentNextInfo info_CurrentNext;
		t_channel_id   channel_id;

		char           aspectRatio;

		uint32_t           sec_timer_id;
		uint32_t           fadeTimer;
		bool           virtual_zap_mode;
		CChannelEventList               evtlist;
		CChannelEventList::iterator     eli;

		void show_Data( bool calledFromEvent = false );
		void paintTime( bool show_dot, bool firstPaint );
		
		void showButton_Audio();
		void showButton_SubServices();
		
		void showIcon_16_9();
#if ENABLE_RADIOTEXT		
		void showIcon_RadioText(bool rt_available) const;
#endif		
	
#ifndef SKIP_CA_STATUS
		void showIcon_CA_Status(int);
		void paint_ca_icons(int, char*);
#endif
		void showIcon_VTXT()      const;
		void showRecordIcon(const bool show);
		void showIcon_SubT() const;

		void showIcon_Resolution() const;
		void showIcon_Audio(const int ac3state) const;
		
		void showFailure();
		void showMotorMoving(int duration);
		void showLcdPercentOver();
		void showSNR();
		
#if ENABLE_RADIOTEXT		
		void showRadiotext();
		void killRadiotext();
#endif		

		CProgressBar *snrscale, *sigscale, *timescale;
		char MoviePercent;
		std::string eventname;

 public:
		bool chanready;
		bool	is_visible;

		uint32_t    lcdUpdateTimer;

		CInfoViewer();

		void	start();

		void	showTitle(const int ChanNum, const std::string & Channel, const t_satellite_position satellitePosition, const t_channel_id new_channel_id = 0, const bool calledFromNumZap = false, int epgpos = 0); // Channel must be UTF-8 encoded

		enum
		{
			NO_AC3,
			AC3_AVAILABLE,
			AC3_ACTIVE
		};

		void lookAheadEPG(const int ChanNum, const std::string & Channel, const t_channel_id new_channel_id = 0, const bool calledFromNumZap = false); //alpha: fix for nvod subchannel update
		void	killTitle();
		CSectionsdClient::CurrentNextInfo getEPG(const t_channel_id for_channel_id, CSectionsdClient::CurrentNextInfo &info);
		CSectionsdClient::CurrentNextInfo getCurrentNextInfo() { return info_CurrentNext; }
	
		void	showSubchan();
#ifndef SKIP_CA_STATUS
		void	Set_CA_Status(int Status);
#endif
	
		int     handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data);
		void    clearVirtualZapMode() {virtual_zap_mode = false;}
		
		void	showEpgInfo();
};

class CInfoViewerHandler : public CMenuTarget
{
	public:
		int  exec( CMenuTarget* parent,  const std::string &actionkey);
		int  doMenu();

};
#endif
