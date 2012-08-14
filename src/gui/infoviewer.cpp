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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/vfs.h>

#include <map>

#include <fcntl.h>

#include <gui/scale.h>
#include <gui/infoviewer.h>

#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>

#include <daemonc/remotecontrol.h>

#include <global.h>
#include <neutrino.h>
#include <gui/pictureviewer.h>

#include <gui/movieplayer.h>


#include <sys/timeb.h>
#include <time.h>
#include <sys/param.h>
#include <zapit/satconfig.h>
#include <zapit/frontend_c.h>
#include <video_cs.h>

#include <system/debug.h>


void sectionsd_getEventsServiceKey(t_channel_id serviceUniqueKey, CChannelEventList &eList, char search = 0, std::string search_text = "");
void sectionsd_getCurrentNextServiceKey(t_channel_id uniqueServiceKey, CSectionsdClient::responseGetCurrentNextInfoChannelID& current_next );

extern CRemoteControl * g_RemoteControl;		/* neutrino.cpp */
extern CPictureViewer * g_PicViewer;

extern cVideo * videoDecoder;
extern CFrontend * live_fe;


#define COL_INFOBAR_BUTTONS            (COL_INFOBAR_SHADOW + 1)
#define COL_INFOBAR_BUTTONS_BACKGROUND (COL_INFOBAR_SHADOW_PLUS_1)

#define ICON_LARGE_WIDTH 26
#define ICON_SMALL_WIDTH 16
#define ICON_LARGE 30
#define ICON_SMALL 18
#define ICON_Y_1 18

#define ICON_OFFSET (2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2)

#define BOTTOM_BAR_OFFSET 0
#define SHADOW_OFFSET 6
#define borderwidth 4
#define LEFT_OFFSET 5
#define ASIZE 100

// in us
#define FADE_TIME 40000

#define LCD_UPDATE_TIME_TV_MODE (60 * 1000 * 1000)


int time_left_width;
int time_dot_width;
int time_width;
int time_height;
bool newfreq = true;
char old_timestr[10];
static event_id_t last_curr_id = 0, last_next_id = 0;

extern int FrontendCount;

extern CZapitClient::SatelliteList satList;
static bool sortByDateTime (const CChannelEvent& a, const CChannelEvent& b)
{
        return a.startTime < b.startTime;
}

extern int timeshift;
extern bool autoshift;
extern uint32_t shift_timer;


#define RED_BAR 40
#define YELLOW_BAR 70
#define GREEN_BAR 100
#define BAR_BORDER 1
#define BAR_WIDTH 72 //(68+BAR_BORDER*2)
#define BAR_HEIGHT 12 //(13 + BAR_BORDER*2)
#define TIME_BAR_HEIGHT 12
// InfoViewer: H 63 W 27
#define NUMBER_H 63
#define NUMBER_W 27


extern std::string ext_channel_name;
int m_CA_Status;
extern bool timeset;


CInfoViewer::CInfoViewer ()
{
  	Init();
}

void CInfoViewer::Init()
{
	frameBuffer = CFrameBuffer::getInstance ();
	
	BoxStartX = BoxStartY = BoxEndX = BoxEndY = 0;
	recordModeActive = false;
	is_visible = false;

	showButtonBar = false;

	gotTime = timeset;
	CA_Status = false;
	virtual_zap_mode = false;
	chanready = 1;

	fileplay = 0;
	
	sigscale = new CScale(BAR_WIDTH, 8, RED_BAR, GREEN_BAR, YELLOW_BAR);
	
	snrscale = new CScale(BAR_WIDTH, 8, RED_BAR, GREEN_BAR, YELLOW_BAR);
	
	timescale = new CScale(108, TIME_BAR_HEIGHT + 5, 30, GREEN_BAR, 70, true);	//5? see in code
}

void CInfoViewer::start()
{
	ChanWidth = 122;
	ChanHeight = 70;
	
	lcdUpdateTimer = g_RCInput->addTimer(LCD_UPDATE_TIME_TV_MODE, false, true);
}

void CInfoViewer::paintTime (bool show_dot, bool firstPaint)
{
	if (gotTime) 
	{
		int ChanNameY = BoxStartY + (ChanHeight >> 1) + 5;	//oberkante schatten?
	
		char timestr[10];
		struct timeb tm;
	
		ftime (&tm);
		strftime ((char *) &timestr, 20, "%H:%M", localtime (&tm.time));
	
		if ((!firstPaint) && (strcmp (timestr, old_timestr) == 0)) 
		{
			if (show_dot)
				frameBuffer->paintBoxRel (BoxEndX - time_width + time_left_width - LEFT_OFFSET, ChanNameY, time_dot_width, time_height / 2 + 2, COL_INFOBAR_PLUS_0);
			else
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString (BoxEndX - time_width + time_left_width - LEFT_OFFSET, ChanNameY + time_height, time_dot_width, ":", COL_INFOBAR);

			strcpy (old_timestr, timestr);
		} 
		else 
		{
			strcpy (old_timestr, timestr);
	
			if (!firstPaint) 
			{
				frameBuffer->paintBoxRel (BoxEndX - time_width - LEFT_OFFSET, ChanNameY, time_width + LEFT_OFFSET, time_height, COL_INFOBAR_PLUS_0, RADIUS_MID, CORNER_TOP);
			}
	
			timestr[2] = 0;
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString (BoxEndX - time_width - LEFT_OFFSET, ChanNameY + time_height, time_left_width, timestr, COL_INFOBAR);

			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString (BoxEndX - time_left_width - LEFT_OFFSET, ChanNameY + time_height, time_left_width, &timestr[3], COL_INFOBAR);

			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString (BoxEndX - time_width + time_left_width - LEFT_OFFSET, ChanNameY + time_height, time_dot_width, ":", COL_INFOBAR);

			if (show_dot)
				frameBuffer->paintBoxRel (BoxEndX - time_left_width - time_dot_width - LEFT_OFFSET, ChanNameY, time_dot_width, time_height / 2 + 2, COL_INFOBAR_PLUS_0);
		}
	}
}

void CInfoViewer::showRecordIcon (const bool show)
{
	recordModeActive = CNeutrinoApp::getInstance ()->recordingstatus || shift_timer;

	if (recordModeActive) 
	{
		int ChanNameX = BoxStartX + ChanWidth + 20;

		if (show) 
		{
			frameBuffer->paintIcon (autoshift ? NEUTRINO_ICON_AUTO_SHIFT : NEUTRINO_ICON_REC, ChanNameX, BoxStartY + 12);

			if(!autoshift && !shift_timer) 
			{
				int chanH = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight ();
				frameBuffer->paintBoxRel (ChanNameX + 28 + SHADOW_OFFSET, BoxStartY + 12 + SHADOW_OFFSET, 300, 20, COL_INFOBAR_SHADOW_PLUS_0);
				frameBuffer->paintBoxRel (ChanNameX + 28, BoxStartY + 12, 300, 20, COL_INFOBAR_PLUS_0);
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString (ChanNameX + 30, BoxStartY + 12 + chanH, 300, ext_channel_name.c_str (), COL_INFOBAR, 0, true);
			} 
			else
				frameBuffer->paintBackgroundBoxRel (ChanNameX + 28, BoxStartY + 12, 300 + SHADOW_OFFSET, 20 + SHADOW_OFFSET);
		} 
		else 
		{
			frameBuffer->paintBackgroundBoxRel (ChanNameX, BoxStartY + 10, 20, 20);
		}
	}
}

void CInfoViewer::showTitle (const int ChanNum, const std::string & Channel, const t_satellite_position satellitePosition, const t_channel_id new_channel_id, const bool calledFromNumZap, int epgpos)
{
	last_curr_id = last_next_id = 0;
	
	std::string ChannelName = Channel;

	bool show_dot = true;
	bool fadeOut = false;
	int fadeValue;
	bool new_chan = false;

	//dprintf(DEBUG_NORMAL, "CInfoViewer::showTitle: chan num %d name %s (%llx)\n", ChanNum, Channel.c_str(), (new_channel_id& 0xFFFFFFFFFFFFULL));

	showButtonBar = !calledFromNumZap;
	bool fadeIn = false /*g_settings.widget_fade*/ && (!is_visible) && showButtonBar;

	is_visible = true;
	
	if (!calledFromNumZap && fadeIn)
		fadeTimer = g_RCInput->addTimer (FADE_TIME, false);

	fileplay = (ChanNum == 0);
	newfreq = true;
	
	sigscale->reset(); 
	snrscale->reset(); 
	timescale->reset();

	InfoHeightY = NUMBER_H * 9 / 8 + 2 * g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight () + 25;
	InfoHeightY_Info = 40;

	time_height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getHeight () + 5;
	time_left_width = 2 * g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getRenderWidth (widest_number);
	time_dot_width = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getRenderWidth (":");
	time_width = time_left_width * 2 + time_dot_width;

	BoxStartX = g_settings.screen_StartX + 10;
	BoxEndX = g_settings.screen_EndX - 10;
	BoxEndY = g_settings.screen_EndY - 10;

	int BoxEndInfoY = showButtonBar ? (BoxEndY - InfoHeightY_Info) : (BoxEndY);
	BoxStartY = BoxEndInfoY - InfoHeightY;

	if (!gotTime)
		gotTime = timeset;

	if (fadeIn) 
	{
		fadeValue = 0x10;
		frameBuffer->setBlendLevel(fadeValue);
	}
	else
	{
		fadeValue = g_settings.gtx_alpha;
	}

	/* kill linke seite */
	frameBuffer->paintBackgroundBox (BoxStartX, BoxStartY + ChanHeight, BoxStartX + (ChanWidth / 3), BoxStartY + ChanHeight + InfoHeightY_Info + 10);

	/* kill progressbar */
	frameBuffer->paintBackgroundBox (BoxEndX - 120, BoxStartY, BoxEndX, BoxStartY + ChanHeight);

	int col_NumBoxText;
	int col_NumBox;

	if (virtual_zap_mode) 
	{
		col_NumBoxText = COL_MENUHEAD;
		col_NumBox = COL_MENUHEAD_PLUS_0;

		if ((channel_id != new_channel_id) || (evtlist.empty())) 
		{
			evtlist.clear();
			//evtlist = g_Sectionsd->getEventsServiceKey(new_channel_id & 0xFFFFFFFFFFFFULL);
			sectionsd_getEventsServiceKey(new_channel_id & 0xFFFFFFFFFFFFULL, evtlist);
			
			if (!evtlist.empty())
				sort(evtlist.begin(),evtlist.end(), sortByDateTime);
			
			new_chan = true;
		}
	} 
	else 
	{
		col_NumBoxText = COL_INFOBAR;
		col_NumBox = COL_INFOBAR_PLUS_0;
	}

	if (! calledFromNumZap && !(g_RemoteControl->subChannels.empty()) && (g_RemoteControl->selected_subchannel > 0))
	{
		channel_id = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].getChannelID();
		ChannelName = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].subservice_name;
	} 
	else 
	{
		channel_id = new_channel_id;
	}

	int ChanNameX = BoxStartX + ChanWidth + SHADOW_OFFSET;
	int ChanNameY = BoxStartY + (ChanHeight / 2) + 5;	//oberkante schatten?
	ChanInfoX = BoxStartX + (ChanWidth / 3);

	asize = (BoxEndX - (2*ICON_LARGE_WIDTH + 2*ICON_SMALL_WIDTH + 4*2) - 102) - ChanInfoX;
	asize = asize - (NEUTRINO_ICON_BUTTON_RED_WIDTH+6)*4;
	asize = asize / 4;

	//Shadow linke seite
	frameBuffer->paintBox (BoxEndX-20, ChanNameY + SHADOW_OFFSET, BoxEndX + SHADOW_OFFSET, BoxEndY, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID, CORNER_TOP);

	//shadow unter seite
	frameBuffer->paintBox (ChanInfoX + SHADOW_OFFSET, BoxEndY -20, BoxEndX + SHADOW_OFFSET, BoxEndY + SHADOW_OFFSET, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID, CORNER_BOTTOM); //round

	//infobox
	frameBuffer->paintBoxRel (ChanNameX-10, ChanNameY, BoxEndX-ChanNameX+10, BoxEndInfoY-ChanNameY, COL_INFOBAR_PLUS_0, RADIUS_MID, CORNER_TOP); // round

	//number box Shadow
	frameBuffer->paintBoxRel (BoxStartX + SHADOW_OFFSET, BoxStartY + SHADOW_OFFSET, ChanWidth, ChanHeight + 4, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID, CORNER_BOTH); // round

	//time
	paintTime (show_dot, true);

	//record-icon
	showRecordIcon (show_dot);
	show_dot = !show_dot;

	//numberbox
	frameBuffer->paintBoxRel (BoxStartX, BoxStartY, ChanWidth, ChanHeight + 4, COL_INFOBAR_PLUS_0, RADIUS_MID, CORNER_BOTH); // round

	//sat name
	int ChanNumYPos = BoxStartY + ChanHeight;

	char strChanNum[10];
	sprintf(strChanNum, "%d", ChanNum);

	if (satellitePositions.size()) 
	{
		sat_iterator_t sit = satellitePositions.find(satellitePosition);

		if(sit != satellitePositions.end()) 
		{
			int satNameWidth = g_SignalFont->getRenderWidth(sit->second.name);
			
			if (satNameWidth > (ChanWidth - 4))
				satNameWidth = ChanWidth - 4;

			int chanH = g_SignalFont->getHeight();
				
			if ( g_settings.infobar_sat_display )
				g_SignalFont->RenderString (3 + BoxStartX + ((ChanWidth - satNameWidth) / 2), BoxStartY + chanH, satNameWidth, sit->second.name, COL_INFOBAR);
		}

		ChanNumYPos += 5;
	}

	/* paint logo */
	bool logo_ok = false;


#define PIC_W 52
#define PIC_H 39

	if (g_settings.infobar_sat_display && satellitePosition != 0 && satellitePositions.size() ) 
	{
		PIC_X = (ChanNameX + 10);
		PIC_Y = (ChanNameY + time_height - PIC_H);

		logo_ok = g_PicViewer->DisplayLogo(channel_id, PIC_X, PIC_Y, PIC_W, PIC_H);

		if(logo_ok)
		{
			/* ChannelNumber */
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->RenderString(ChanNameX + PIC_W + 15, ChanNameY + time_height, ChanWidth, strChanNum, col_NumBoxText);
	
			/* ChannelName */
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString (ChanNameX + PIC_W + 20 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getRenderWidth(strChanNum), ChanNameY + time_height, BoxEndX - (ChanNameX + 20) - time_width - LEFT_OFFSET - 5 - ChanWidth, ChannelName, COL_INFOBAR, 0, true);	// UTF-8
		}
		else	
		{
			/* ChannelNumber */
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->RenderString(ChanNameX + 5, ChanNameY + time_height, ChanWidth, strChanNum, col_NumBoxText);
	
			/* ChannelName */
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString (ChanNameX + 15 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getRenderWidth(strChanNum), ChanNameY + time_height, BoxEndX - (ChanNameX + 20) - time_width - LEFT_OFFSET - 5 - ChanWidth, ChannelName, COL_INFOBAR, 0, true);	// UTF-8
		}
	}
	else //show logo in infobox center and chanNum
	{
		/* ChannelLogo */
		PIC_X = (BoxStartX + ChanWidth / 2 - PIC_W / 2);
		PIC_Y = (BoxStartY + ChanHeight / 2 - PIC_H / 2) + 5;

		logo_ok = g_PicViewer->DisplayLogo(channel_id, PIC_X, PIC_Y, PIC_W, PIC_H);

		/* ChannelNumber */
		if(logo_ok)
		{
			/* ChannelNumber */
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->RenderString(ChanNameX + 5, ChanNameY + time_height, ChanWidth, strChanNum, col_NumBoxText);
	
			/* ChannelName */
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString (ChanNameX + 15 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getRenderWidth(strChanNum), ChanNameY + time_height, BoxEndX - (ChanNameX + 20) - time_width - LEFT_OFFSET - 5 - ChanWidth, ChannelName, COL_INFOBAR, 0, true);	// UTF-8
		}
		else	
		{
			/* ChannelNumber(numberBox) */
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->RenderString(BoxStartX + ((ChanWidth - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getRenderWidth(strChanNum))>>1), ChanNumYPos - 15, ChanWidth, strChanNum, col_NumBoxText);

			/* ChannelName */
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString (ChanNameX + 15, ChanNameY + time_height, BoxEndX - (ChanNameX + 20) - time_width - LEFT_OFFSET - 5 - ChanWidth, ChannelName, COL_INFOBAR, 0, true);	// UTF-8
		}
	}

	int ChanInfoY = BoxStartY + ChanHeight + 10;
	ButtonWidth = (BoxEndX - ChanInfoX - ICON_OFFSET) >> 2;

	frameBuffer->paintBox (ChanInfoX, ChanInfoY, ChanNameX, BoxEndInfoY, COL_INFOBAR_PLUS_0);

	//buttons bar
	if (showButtonBar) 
	{
		sec_timer_id = g_RCInput->addTimer (1*1000*1000, false);
		
		if (BOTTOM_BAR_OFFSET > 0)
			frameBuffer->paintBackgroundBox (ChanInfoX, BoxEndInfoY, BoxEndX, BoxEndInfoY + BOTTOM_BAR_OFFSET);

		// crypt bar
		frameBuffer->paintBox(ChanInfoX, BoxEndInfoY-2, BoxEndX, BoxEndY-20, COL_INFOBAR_BUTTONS_BACKGROUND);

		//crypt icons bar
		frameBuffer->paintBox (ChanInfoX, BoxEndInfoY-2, BoxEndX, BoxEndY-20, COL_INFOBAR_PLUS_1);
		
		// show date
		char datestr[11];
			
		time_t wakeup_time;
		struct tm *now;
			
		time(&wakeup_time);
		now = localtime(&wakeup_time);
		
		strftime( datestr, sizeof(datestr), "%d.%m.%Y", now);
			
		int widthr = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(datestr, true); //UTF-8
			
		//g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString( (BoxEndX - ChanInfoX + widthr)/2, BoxEndY - 16, widthr, datestr, COL_INFOBAR);
		int stringstartposX = ChanInfoX + (BoxEndX >> 1) - (widthr >> 1) - 40;
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(stringstartposX, BoxEndY - 16, (BoxEndX - ChanInfoX) - (stringstartposX - ChanInfoX), datestr, /*COL_INFOBAR*/COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
		
		// botton bar
		frameBuffer->paintBox(ChanInfoX, BoxEndY - 20, BoxEndX, BoxEndY, COL_INFOBAR_BUTTONS_BACKGROUND, RADIUS_MID, CORNER_BOTTOM); //round

		//signal
		showSNR();

		// blue button
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, ChanInfoX + 16*3 + asize * 3 + 2*7, BoxEndY - ICON_Y_1);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX + 16*4 + asize * 3 + 2*8, BoxEndY+2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2 + 2), g_Locale->getText(LOCALE_INFOVIEWER_STREAMINFO), COL_INFOBAR_BUTTONS, 0, true); // UTF-8

		showButton_Audio ();
		showButton_SubServices ();
		showIcon_CA_Status(0);
		showIcon_16_9();
		showIcon_VTXT();
		showIcon_SubT();
		showIcon_Resolution();
	}

	// show current_next epg data
	sectionsd_getCurrentNextServiceKey(channel_id & 0xFFFFFFFFFFFFULL, info_CurrentNext);
	
	if (!evtlist.empty()) 
	{
		if (new_chan) 
		{
			for ( eli=evtlist.begin(); eli!=evtlist.end(); ++eli ) 
			{
				if ((uint)eli->startTime >= info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer)
					break;
			}
			
			if (eli == evtlist.end()) // the end is not valid, so go back
				--eli;
		}

		if (epgpos != 0) 
		{
			info_CurrentNext.flags = 0;
			if ((epgpos > 0) && (eli != evtlist.end())) 
			{
				++eli; // next epg
				if (eli == evtlist.end()) // the end is not valid, so go back
					--eli;
			}
			else if ((epgpos < 0) && (eli != evtlist.begin())) 
			{
				--eli; // prev epg
			}

			info_CurrentNext.flags = CSectionsdClient::epgflags::has_current;
			info_CurrentNext.current_uniqueKey      = eli->eventID;
			info_CurrentNext.current_zeit.startzeit = eli->startTime;
			info_CurrentNext.current_zeit.dauer     = eli->duration;

			if (eli->description.empty())
				info_CurrentNext.current_name   = g_Locale->getText(LOCALE_INFOVIEWER_NOEPG);
			else
				info_CurrentNext.current_name   = eli->description;

			info_CurrentNext.current_fsk            = '\0';

			if (eli != evtlist.end()) 
			{
				++eli;
				if (eli != evtlist.end()) 
				{
					info_CurrentNext.flags                  = CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_next;
					info_CurrentNext.next_uniqueKey         = eli->eventID;
					info_CurrentNext.next_zeit.startzeit    = eli->startTime;
					info_CurrentNext.next_zeit.dauer        = eli->duration;

					if (eli->description.empty())
						info_CurrentNext.next_name      = g_Locale->getText(LOCALE_INFOVIEWER_NOEPG);
					else
						info_CurrentNext.next_name      = eli->description;
				}
				--eli;
			}
		}
	}

	if (!(info_CurrentNext.flags & (CSectionsdClient::epgflags::has_later | CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::not_broadcast))) 
	{
		// nicht gefunden / noch nicht geladen
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (ChanNameX + 10, ChanInfoY + 2 * g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight () + 5, BoxEndX - (ChanNameX + 20), g_Locale->getText (gotTime ? (showButtonBar ? LOCALE_INFOVIEWER_EPGWAIT : LOCALE_INFOVIEWER_EPGNOTLOAD) : LOCALE_INFOVIEWER_WAITTIME), COL_INFOBAR, 0, true);	// UTF-8
	} 
	else 
	{
		show_Data();
	}
	
#ifdef FB_BLIT	
	frameBuffer->blit();
#endif	

	showLcdPercentOver();

	if ((g_RemoteControl->current_channel_id == channel_id) && !(((info_CurrentNext.flags & CSectionsdClient::epgflags::has_next) && (info_CurrentNext.flags & (CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_no_current))) || (info_CurrentNext.flags & CSectionsdClient::epgflags::not_broadcast))) 
	{
		g_Sectionsd->setServiceChanged (channel_id & 0xFFFFFFFFFFFFULL, true);
	}
	
	// radiotext
	if (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio)
	{
		if ((g_settings.radiotext_enable) && (!recordModeActive) && (!calledFromNumZap))
			showRadiotext();
		else
			showIcon_RadioText(false);
	}

	// loop msg
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	CNeutrinoApp * neutrino = CNeutrinoApp::getInstance ();

	if (!calledFromNumZap) 
	{

		bool hideIt = true;
		virtual_zap_mode = false;
		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd (g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR] == 0 ? 0xFFFF : g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR]);

		int res = messages_return::none;

		while (!(res & (messages_return::cancel_info | messages_return::cancel_all))) 
		{
			g_RCInput->getMsgAbsoluteTimeout (&msg, &data, &timeoutEnd);
			
#if 1
			time_t ta, tb;
			
			if (!(info_CurrentNext.flags & (CSectionsdClient::epgflags::has_current))) 
			{
				if (difftime (time (&tb), ta) > 1.1) 
				{
					time (&ta);
					info_CurrentNext = getEPG(channel_id, info_CurrentNext);
					if ((info_CurrentNext.flags & (CSectionsdClient::epgflags::has_current))) 
					{
						show_Data();
						showLcdPercentOver ();
					}
				}
			}
#endif

			if ( msg == CRCInput::RC_sat || msg == CRCInput::RC_favorites)
			{
				g_RCInput->postMsg (msg, 0);
				res = messages_return::cancel_info;
			}
			else if ( msg == CRCInput::RC_info )
			{
				g_RCInput->postMsg (NeutrinoMessages::SHOW_EPG, 0);
				res = messages_return::cancel_info;
			} 
			else if ((msg == NeutrinoMessages::EVT_TIMER) && (data == fadeTimer)) 
			{
				if (fadeOut) 
				{ 
					// disappear
					fadeValue -= 0x10;
					if (fadeValue <= 0x10) 
					{
						fadeValue = g_settings.gtx_alpha;
						g_RCInput->killTimer (fadeTimer);
						res = messages_return::cancel_info;
					} 
					else
						frameBuffer->setBlendLevel(fadeValue);
				} 
				else 
				{ 
					// appears
					fadeValue += 0x10;

					if (fadeValue >= g_settings.gtx_alpha) 
					{
						fadeValue = g_settings.gtx_alpha;
						g_RCInput->killTimer (fadeTimer);
						fadeIn = false;
						frameBuffer->setBlendLevel(g_settings.gtx_alpha);
					} 
					else
						frameBuffer->setBlendLevel(fadeValue);
				}
			} 
			else if ((msg == CRCInput::RC_ok) || (msg == CRCInput::RC_home) || (msg == CRCInput::RC_timeout)) 
			{
				if (fadeIn) 
				{
					g_RCInput->killTimer (fadeTimer);
					fadeIn = false;
				}

				if ((!fadeOut) && false /*g_settings.widget_fade*/) 
				{
					fadeOut = true;
					fadeTimer = g_RCInput->addTimer (FADE_TIME, false);
					timeoutEnd = CRCInput::calcTimeoutEnd (1);
				} 
				else 
				{
#if 0
					if ((msg != CRCInput::RC_timeout) && (msg != CRCInput::RC_ok))
						if (!fileplay && !timeshift)
							g_RCInput->postMsg (msg, data);
#endif
					res = messages_return::cancel_info;
				}
			} 
			else if ((msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id)) 
			{
				showSNR();
				
				paintTime (show_dot, false);
				showRecordIcon (show_dot);
				show_dot = !show_dot;
				
				// radiotext
				if ((g_settings.radiotext_enable) && (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio))
					showRadiotext();

				showIcon_16_9();
				
				if (is_visible && showButtonBar) 
					showIcon_Resolution();
			} 
			else if ( g_settings.virtual_zap_mode && ((msg == CRCInput::RC_right) || msg == CRCInput::RC_left )) 
			{
				virtual_zap_mode = true;
				res = messages_return::cancel_all;
				hideIt = true;
			} 
			else if ( !fileplay && !timeshift) 
			{
				if ((msg == (neutrino_msg_t) g_settings.key_quickzap_up) || (msg == (neutrino_msg_t) g_settings.key_quickzap_down) || (msg == CRCInput::RC_0) || (msg == NeutrinoMessages::SHOW_INFOBAR)) 
				{
					// radiotext
					if ((g_settings.radiotext_enable) && (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio))
						hideIt =  true;
					else
						hideIt = false;
					
					g_RCInput->postMsg (msg, data);
					res = messages_return::cancel_info;
				} 
				else if (msg == NeutrinoMessages::EVT_TIMESET) 
				{
					// Handle anyway!
					neutrino->handleMsg (msg, data);
					g_RCInput->postMsg (NeutrinoMessages::SHOW_INFOBAR, 0);
					hideIt = false;
					res = messages_return::cancel_all;
				} 
				else 
				{
					if (msg == CRCInput::RC_standby) 
					{
						g_RCInput->killTimer (sec_timer_id);
						if (fadeIn || fadeOut)
							g_RCInput->killTimer (fadeTimer);
					}

					res = neutrino->handleMsg (msg, data);
					if (res & messages_return::unhandled) 
					{
						// raus hier und im Hauptfenster behandeln...
						g_RCInput->postMsg (msg, data);
						res = messages_return::cancel_info;
					}
				}
			}
			
#ifdef FB_BLIT			
			frameBuffer->blit();
#endif			
		}

		if (hideIt)
			killTitle ();

		g_RCInput->killTimer (sec_timer_id);
		sec_timer_id = 0;

		if (fadeIn || fadeOut) 
		{
			g_RCInput->killTimer (fadeTimer);
			frameBuffer->setBlendLevel(g_settings.gtx_alpha);
		}

		if (virtual_zap_mode)
			CNeutrinoApp::getInstance()->channelList->virtual_zap_mode(msg == CRCInput::RC_right);

	}

	//test
	fileplay = 0;
}


/*
		 ___BoxStartX
		|-ChanWidth-|
		|           |  _recording icon                 _progress bar
    BoxStartY---+-----------+ |                               |
	|	|           | *  infobar.txt text            #######____
	|	|           |-------------------------------------------+--ChanNameY
	|	|           | Channelname                               |
    ChanHeight--+-----------+                                           |
		   |                                                    |
		   |01:23     Current Event                             |
		   |02:34     Next Event                                |
		   |                                                    |
    BoxEndY--------+----------------------------------------------------+
		                                                        |
		                                                BoxEndX-/
*/

extern int speed;
void CInfoViewer::showMovieTitle (const int playstate, const std::string & title, const std::string & sub_title, const std::string & sub_title1, const int position, const int duration, const int ac3state, const bool ShowBlueButton, unsigned char file_prozent )
{  
	fileplay = 1;
	//showButtonBar = true;

	is_visible = true;

	InfoHeightY = NUMBER_H * 9 / 8 + 2 * g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight() + 25;
	InfoHeightY_Info = 40;

	BoxStartX = g_settings.screen_StartX + 10;
	BoxEndX = g_settings.screen_EndX - 10;
	BoxEndY = g_settings.screen_EndY - 10 - 20;

	int BoxEndInfoY = BoxEndY - InfoHeightY_Info;
	BoxStartY = BoxEndInfoY - InfoHeightY;
	
	moviescale = new CScale(BoxEndX - ChanInfoX, BoxEndY - 18 - BoxEndInfoY, 30, GREEN_BAR, 70, true);
	moviescale->reset();
	MoviePercent = file_prozent;
	
	int fadeValue;
	//bool fadeIn = false /*g_settings.widget_fade*/ && (!is_visible) /*&& showButtonBar*/;
	bool fadeIn = !is_visible;

	if (fadeIn)
		fadeTimer = g_RCInput->addTimer (FADE_TIME, false);

	if (fadeIn) 
	{
		fadeValue = 0x10;
		frameBuffer->setBlendLevel(fadeValue);
	} 
	else
		fadeValue = g_settings.gtx_alpha;
	
	/* kill linke seite */
	frameBuffer->paintBackgroundBox (BoxStartX, BoxStartY + ChanHeight, BoxStartX + (ChanWidth / 3), BoxStartY + ChanHeight + InfoHeightY_Info + 10);

	/* kill progressbar */
	frameBuffer->paintBackgroundBox (BoxEndX - 120, BoxStartY, BoxEndX, BoxStartY + ChanHeight);

	int ChanNameX = BoxStartX + ChanWidth + SHADOW_OFFSET;
	int ChanNameY = BoxStartY + (ChanHeight / 2) + 5;	//oberkante schatten?
	ChanInfoX = BoxStartX + (ChanWidth / 3);

	/* Shadow linke seite */
	frameBuffer->paintBox (BoxEndX-20, ChanNameY + SHADOW_OFFSET, BoxEndX + SHADOW_OFFSET, BoxEndY, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID, CORNER_TOP);

	/* shadow untere seite */
	frameBuffer->paintBox (ChanInfoX + SHADOW_OFFSET, BoxEndY -20, BoxEndX + SHADOW_OFFSET, BoxEndY + SHADOW_OFFSET, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID, CORNER_BOTTOM); //round

	/* infobox */
	frameBuffer->paintBoxRel (ChanNameX-10, ChanNameY, BoxEndX-ChanNameX+10, BoxEndInfoY-ChanNameY, COL_INFOBAR_PLUS_0, RADIUS_MID, CORNER_TOP); // round

	/* numberbox shadow */
	frameBuffer->paintBoxRel (BoxStartX + SHADOW_OFFSET, BoxStartY + SHADOW_OFFSET, ChanWidth, ChanHeight + 4, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID, CORNER_BOTH); // round

	/* numberbox */
	frameBuffer->paintBoxRel (BoxStartX, BoxStartY, ChanWidth, ChanHeight + 4, COL_INFOBAR_PLUS_0, RADIUS_MID, CORNER_BOTH); // round
	
	//test
	/* time */
	//bool show_dot = true;
	paintTime (true, true);
	//show_dot = !show_dot;

	int ChanInfoY = BoxStartY + ChanHeight + 10;
	ButtonWidth = (BoxEndX - ChanInfoX - ICON_OFFSET) >> 2;

	frameBuffer->paintBox(ChanInfoX, ChanInfoY, ChanNameX, BoxEndInfoY, COL_INFOBAR_PLUS_0);

	/* buttons bar */
	asize = (BoxEndX - (2*ICON_LARGE_WIDTH + 2*ICON_SMALL_WIDTH + 4*2) - 102) - ChanInfoX;
	asize = asize - (NEUTRINO_ICON_BUTTON_RED_WIDTH+6)*4;
	asize = asize / 4;
	
	sec_timer_id = g_RCInput->addTimer (1*1000*1000, false);

	if (BOTTOM_BAR_OFFSET > 0)
		frameBuffer->paintBackgroundBox (ChanInfoX, BoxEndInfoY, BoxEndX, BoxEndInfoY + BOTTOM_BAR_OFFSET);

	//moviescale bar
	frameBuffer->paintBox(ChanInfoX, BoxEndInfoY-2, BoxEndX, BoxEndY-20, COL_INFOBAR_PLUS_1);
		
	// bottum bar
	frameBuffer->paintBox (ChanInfoX, BoxEndY-20, BoxEndX, BoxEndY, COL_INFOBAR_BUTTONS_BACKGROUND, RADIUS_MID, CORNER_BOTTOM); //round
		
	//red 
	//avsync
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, ChanInfoX + 2, BoxEndY - ICON_Y_1);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX + (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2), BoxEndY + 2, asize, /*g_Locale->getText(LOCALE_INFOVIEWER_EVENTLIST)*/ (char *)"AVSync", COL_INFOBAR_BUTTONS, 0, true); // UTF-8

	// green buttom (audio)
	//frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, ChanInfoX + 2, BoxEndY - ICON_Y_1);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, ChanInfoX + 2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + asize + 2, BoxEndY - ICON_Y_1);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX + 2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + asize + 2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2, BoxEndY+2, asize, g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES), COL_INFOBAR_BUTTONS, 0, true);		 // UTF-8
		
	//yellow
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, ChanInfoX + 2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + 2*asize + 2, BoxEndY - ICON_Y_1);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX + 2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + 2*asize + 2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2, BoxEndY+2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2 + 2), (char *)"help", COL_INFOBAR_BUTTONS, 0, true);	 // UTF-8
		
	// blue buttom (bookmark)
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, ChanInfoX + 2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + 3*asize + 2, BoxEndY - ICON_Y_1);
	if(ShowBlueButton)
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX + 2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + 3*asize + 2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2, BoxEndY+2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2 + 2), g_Locale->getText(LOCALE_MOVIEPLAYER_BOOKMARK), COL_INFOBAR_BUTTONS, 0, true);	 // UTF-8
		
	/* mp keys */
	frameBuffer->paintIcon("ico_mp_rewind", ChanInfoX + 16*3 + (asize * 4), BoxEndY - ICON_Y_1);
	frameBuffer->paintIcon("ico_mp_play", ChanInfoX + 16*3 + (asize * 4) + 17, BoxEndY - ICON_Y_1);
	frameBuffer->paintIcon("ico_mp_pause", ChanInfoX + 16*3 + (asize * 4) + 2*17, BoxEndY - ICON_Y_1);
	frameBuffer->paintIcon("ico_mp_stop", ChanInfoX + 16*3 + (asize * 4) + 3*17, BoxEndY - ICON_Y_1);
	frameBuffer->paintIcon("ico_mp_forward", ChanInfoX + 16*3 + (asize * 4) + 4*17, BoxEndY - ICON_Y_1);
		
	// 16_9
	//showIcon_16_9();
		
	// ac3
	showIcon_Audio(ac3state);
		
	// mp icon
	frameBuffer->paintIcon("mp", ChanInfoX, ChanInfoY);	

	// play-state icon 
	const char *icon;
	
	switch(playstate)
	{
		case CMoviePlayerGui::PAUSE: icon = "mp_pause"; break;
		case CMoviePlayerGui::PLAY: icon = "mp_play"; break;
		case CMoviePlayerGui::REW: icon = "mp_b-skip"; break;
		case CMoviePlayerGui::FF: icon = "mp_f-skip"; break;
	}

	// get icon size
	int icon_w = 0;
	int icon_h = 0;
	
	frameBuffer->getIconSize(icon, &icon_w, &icon_h);

	int icon_x = BoxStartX + ChanWidth / 2 - icon_w / 2 + 5;
	int icon_y = BoxStartY + ChanHeight / 2 - icon_h / 2;
	

	frameBuffer->paintIcon(icon, icon_x, icon_y);
	
	// paint speed
	char strSpeed[4];
	if( playstate == CMoviePlayerGui::FF || playstate == CMoviePlayerGui::REW )
	{
		sprintf(strSpeed, "%d", speed);
		
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->RenderString(BoxStartX + 5, BoxStartY + ChanHeight - 15, ChanWidth, strSpeed, /*COL_MENUHEAD*/ COL_MENUCONTENTINACTIVE); // UTF-8
	}
		

	/* Movieplayer Title */
	time_height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getHeight () + 5;
	//time_width = time_left_width * 2 + time_dot_width;
	//time_height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getHeight () + 5;
	time_left_width = 2 * g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getRenderWidth (widest_number);
	time_dot_width = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getRenderWidth (":");
	time_width = time_left_width * 2 + time_dot_width;
	
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(ChanNameX + 10, ChanNameY + time_height, BoxEndX - (ChanNameX + 20) - time_width - 15, g_Locale->getText(LOCALE_MOVIEPLAYER_HEAD), COL_INFOBAR, 0, true);	

	// file_procent bar
	moviescale->paint(BoxStartX + 40, BoxEndY - 40, MoviePercent);

	// Infos Titles
	int xStart = BoxStartX + ChanWidth;
	ChanInfoY = BoxStartY + ChanHeight + 15;	//+10

	int height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();
	
	char runningRest[32]; // %d can be 10 digits max...
	//sprintf(runningRest, "%ld / %ld min", (time_elapsed + 30) / 60, (time_remaining + 30) / 60);
	sprintf(runningRest, "%d / %d min", (position + 30000) / 60000, (duration + 30000) / 60000);

	int duration1Width = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth (runningRest);
	int duration1TextPos = BoxEndX - duration1Width - LEFT_OFFSET;

	//Title 1
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (xStart, ChanInfoY + height, duration1TextPos - xStart - 5, sub_title, COL_INFOBAR, 0, true);

	//Title2
	ChanInfoY += height;
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (xStart, ChanInfoY + height, duration1TextPos - xStart - 5, sub_title1, COL_INFOBAR, 0, true);

	//Time Elapsed/Time Remaining
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(duration1TextPos, ChanInfoY, duration1Width, runningRest, COL_INFOBAR);
	
#ifdef FB_BLIT	
	frameBuffer->blit();	
#endif	

	/* info loop msg */
	//InfoLoop(const boll calledFromNumZap, FadeIn)
	bool fadeOut = false;
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	bool hideIt = true;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd (g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR] == 0 ? 0xFFFF : g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR]);

	int res = messages_return::none;

	while (!(res & (messages_return::cancel_info | messages_return::cancel_all))) 
	{
		g_RCInput->getMsgAbsoluteTimeout (&msg, &data, &timeoutEnd);

		if ( (msg == CRCInput::RC_ok) || (msg == CRCInput::RC_home) || (msg == CRCInput::RC_timeout) ) 
		{
			if (fadeIn) 
			{
				g_RCInput->killTimer(fadeTimer);
				fadeIn = false;
			}

			if ((!fadeOut) && false /*g_settings.widget_fade*/) 
			{
				fadeOut = true;
				fadeTimer = g_RCInput->addTimer (FADE_TIME, false);
				timeoutEnd = CRCInput::calcTimeoutEnd(1);
			} 
			else 
			{
				res = messages_return::cancel_info;
			}
		}
		else
		{
			res = CNeutrinoApp::getInstance()->handleMsg(msg, data);

			if (res & messages_return::unhandled) 
			{
				// raus hier und im Hauptfenster behandeln...
				g_RCInput->postMsg (msg, data);
				res = messages_return::cancel_info;
			}
		}
			
#ifdef FB_BLIT
		frameBuffer->blit();
#endif			
	}

	if (hideIt)
		killTitle();

	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;

	if (fadeIn || fadeOut) 
	{
		g_RCInput->killTimer(fadeTimer);

		frameBuffer->setBlendLevel(g_settings.gtx_alpha);
	}
}

void CInfoViewer::showSubchan()
{
  	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();
  	CNeutrinoApp *neutrino = CNeutrinoApp::getInstance();

  	std::string subChannelName;	// holds the name of the subchannel/audio channel
  	int subchannel = 0;		// holds the channel index

  	if (!(g_RemoteControl->subChannels.empty ())) 
	{
		// get info for nvod/subchannel
		subchannel = g_RemoteControl->selected_subchannel;
		if (g_RemoteControl->selected_subchannel >= 0)
	  		subChannelName = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].subservice_name;
  	} 
	else if (g_RemoteControl->current_PIDs.APIDs.size () > 1 ) 
	{
		// get info for audio channel
		subchannel = g_RemoteControl->current_PIDs.PIDs.selected_apid;
		subChannelName = g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].desc;
  	}

  	if (!(subChannelName.empty ())) 
	{
		char text[100];
		sprintf (text, "%d - %s", subchannel, subChannelName.c_str ());

		int dx = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth (text) + 20;
		int dy = 25;

		if (g_RemoteControl->director_mode) 
		{
	  		int w = 20 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth (g_Locale->getText (LOCALE_NVODSELECTOR_DIRECTORMODE), true) + 20;	// UTF-8
	  		if (w > dx)
				dx = w;
	  			dy = dy * 2;
		} 
		else
	  		dy = dy + 5;

		int x = 0, y = 0;
		
		if (g_settings.infobar_subchan_disp_pos == 0) 
		{
	  		// Rechts-Oben
	  		x = g_settings.screen_EndX - dx - 10;
	  		y = g_settings.screen_StartY + 10;
		} 
		else if (g_settings.infobar_subchan_disp_pos == 1) 
		{
	  		// Links-Oben
	  		x = g_settings.screen_StartX + 10;
	  		y = g_settings.screen_StartY + 10;
		} 
		else if (g_settings.infobar_subchan_disp_pos == 2) 
		{
	  		// Links-Unten
	  		x = g_settings.screen_StartX + 10;
	  		y = g_settings.screen_EndY - dy - 10;
		} 
		else if (g_settings.infobar_subchan_disp_pos == 3) 
		{
	  		// Rechts-Unten
	  		x = g_settings.screen_EndX - dx - 10;
	  		y = g_settings.screen_EndY - dy - 10;
		}

		fb_pixel_t pixbuf[(dx + 2 * borderwidth) * (dy + 2 * borderwidth)];
		frameBuffer->SaveScreen (x - borderwidth, y - borderwidth, dx + 2 * borderwidth, dy + 2 * borderwidth, pixbuf);		

		// clear border
		frameBuffer->paintBackgroundBoxRel (x - borderwidth, y - borderwidth, dx + 2 * borderwidth, borderwidth);
		frameBuffer->paintBackgroundBoxRel (x - borderwidth, y + dy, dx + 2 * borderwidth, borderwidth);
		frameBuffer->paintBackgroundBoxRel (x - borderwidth, y, borderwidth, dy);
		frameBuffer->paintBackgroundBoxRel (x + dx, y, borderwidth, dy);

		frameBuffer->paintBoxRel (x, y, dx, dy, COL_MENUCONTENT_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (x + 10, y + 30, dx - 20, text, COL_MENUCONTENT);

		if (g_RemoteControl->director_mode) 
		{
	  		frameBuffer->paintIcon (NEUTRINO_ICON_BUTTON_YELLOW, x + 8, y + dy - 20);
	  		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString (x + 30, y + dy - 2, dx - 40, g_Locale->getText (LOCALE_NVODSELECTOR_DIRECTORMODE), COL_MENUCONTENT, 0, true);	// UTF-8
		}
		
#ifdef FB_BLIT
		frameBuffer->blit();
#endif		

		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd (2);
		int res = messages_return::none;

		neutrino_msg_t msg;
		neutrino_msg_data_t data;

		while (!(res & (messages_return::cancel_info | messages_return::cancel_all))) 
		{
	  		g_RCInput->getMsgAbsoluteTimeout (&msg, &data, &timeoutEnd);

	  		if (msg == CRCInput::RC_timeout) 
			{
				res = messages_return::cancel_info;
	  		} 
			else 
			{
				res = neutrino->handleMsg(msg, data);

				if (res & messages_return::unhandled) 
				{
		  			// raus hier und im Hauptfenster behandeln...
		  			g_RCInput->postMsg (msg, data);
		  			res = messages_return::cancel_info;
				}
	  		}
		}

		frameBuffer->RestoreScreen(x - borderwidth, y - borderwidth, dx + 2 * borderwidth, dy + 2 * borderwidth, pixbuf);
		
#ifdef FB_BLIT
		frameBuffer->blit();
#endif		
  		
	} 
	else 
	{
		g_RCInput->postMsg (NeutrinoMessages::SHOW_INFOBAR, 0);
  	}
}

// radiotext
void CInfoViewer::showIcon_RadioText(bool /*rt_available*/) const
// painting the icon for radiotext mode
{
#if 0
	if (showButtonBar)
	{
		int mode = CNeutrinoApp::getInstance()->getMode();
		std::string rt_icon = "radiotextoff.raw";
		if ((!virtual_zap_mode) && (!recordModeActive) && (mode == NeutrinoMessages::mode_radio))
		{
			if (g_settings.radiotext_enable){
					rt_icon = rt_available ? "radiotextget.raw" : "radiotextwait.raw";
				}
		}
		frameBuffer->paintIcon(rt_icon, BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
	}
#endif
}

void CInfoViewer::showIcon_16_9()
{			
	const char * aspect_icon = NEUTRINO_ICON_16_9_GREY;
			
	if(videoDecoder->getAspectRatio() == 1)
		aspect_icon = NEUTRINO_ICON_16_9;
			
	frameBuffer->paintIcon(aspect_icon, BoxEndX - (2*ICON_LARGE_WIDTH + 2*ICON_SMALL_WIDTH + 4*2), BoxEndY - ICON_Y_1);
}

void CInfoViewer::showIcon_VTXT () const
{
 	 frameBuffer->paintIcon ((g_RemoteControl->current_PIDs.PIDs.vtxtpid != 0) ? NEUTRINO_ICON_VTXT : NEUTRINO_ICON_VTXT_GREY, BoxEndX - (2*ICON_SMALL_WIDTH + 2*2), BoxEndY - ICON_Y_1);
}

void CInfoViewer::showIcon_Resolution() const
{
	int xres, yres, framerate;
	const char *icon_name = NULL;
	const char *icon_name_res = NULL;
	
	int icon_xres_width = 0;
	int dummy_h = 0;
	
	frameBuffer->getIconSize(NEUTRINO_ICON_RESOLUTION_000, &icon_xres_width, &dummy_h);

	// show resolution icon on infobar
	videoDecoder->getPictureInfo(xres, yres, framerate);
			
	switch (yres) 
	{
		case 1920:
			icon_name = NEUTRINO_ICON_RESOLUTION_1920;
			break;
			
		case 1088:
		case 1080:
			icon_name = NEUTRINO_ICON_RESOLUTION_1080;
			break;
			
		case 1440:
			icon_name = NEUTRINO_ICON_RESOLUTION_1440;
			break;
			
		case 1280:
			icon_name = NEUTRINO_ICON_RESOLUTION_1280;
			break;
			
		case 720:
			icon_name = NEUTRINO_ICON_RESOLUTION_720;
			break;
			
		case 704:
			icon_name = NEUTRINO_ICON_RESOLUTION_704;
			break;
			
		case 576:
			icon_name = NEUTRINO_ICON_RESOLUTION_576;
			break;
			
		case 544:
			icon_name = NEUTRINO_ICON_RESOLUTION_544;
			break;
			
		case 528:
			icon_name = NEUTRINO_ICON_RESOLUTION_528;
			break;
			
		case 480:
			icon_name = NEUTRINO_ICON_RESOLUTION_480;
			break;
			
		case 382:
			icon_name = NEUTRINO_ICON_RESOLUTION_382;
			break;
			
		case 352:
			icon_name = NEUTRINO_ICON_RESOLUTION_352;
			break;
			
		case 288:
			icon_name = NEUTRINO_ICON_RESOLUTION_288;
			break;
			
		default:
			icon_name = NEUTRINO_ICON_RESOLUTION_000;
			break;
	}
	frameBuffer->paintBoxRel(BoxEndX - (2*ICON_LARGE_WIDTH + 2*ICON_SMALL_WIDTH + 4*2) -30-30, BoxEndY - ICON_Y_1, icon_xres_width, dummy_h, COL_INFOBAR_BUTTONS_BACKGROUND);
	if(icon_name !=NEUTRINO_ICON_RESOLUTION_000)
		frameBuffer->paintIcon(icon_name, BoxEndX - (2*ICON_LARGE_WIDTH + 2*ICON_SMALL_WIDTH + 4*2) -30 -30, BoxEndY - ICON_Y_1);
		
	// show sd/hd icon on infobar		
	switch (yres) 
	{
		case 1920:
		case 1440:
		case 1280:
		case 1088:
		case 1080:
		case 720:
			icon_name_res = NEUTRINO_ICON_RESOLUTION_HD;
			//test
			CVFD::getInstance()->ShowIcon(VFD_ICON_HD, true); //FIXME: remove this to remotecontrol
			break;
			
		case 704:
		case 576:
		case 544:
		case 528:
		case 480:
		case 382:
		case 352:
		case 288:
			icon_name_res = NEUTRINO_ICON_RESOLUTION_SD;
			CVFD::getInstance()->ShowIcon(VFD_ICON_HD, false);
			break;
			
		default:
			icon_name_res = NEUTRINO_ICON_RESOLUTION_000;
			CVFD::getInstance()->ShowIcon(VFD_ICON_HD, false);
			break;	
	}
	
	frameBuffer->paintBoxRel(BoxEndX - (2*ICON_LARGE_WIDTH + 2*ICON_SMALL_WIDTH + 4*2) -30, BoxEndY - ICON_Y_1, icon_xres_width, dummy_h, COL_INFOBAR_BUTTONS_BACKGROUND);
	if(icon_name_res !=NEUTRINO_ICON_RESOLUTION_000)
		frameBuffer->paintIcon(icon_name_res, BoxEndX - (2*ICON_LARGE_WIDTH + 2*ICON_SMALL_WIDTH + 4*2) -30, BoxEndY - ICON_Y_1);
}
//

void CInfoViewer::showIcon_SubT() const
{
        bool have_sub = false;
	CZapitChannel * cc = CNeutrinoApp::getInstance()->channelList->getChannel(CNeutrinoApp::getInstance()->channelList->getActiveChannelNumber());
	if(cc && cc->getSubtitleCount())
		have_sub = true;

        frameBuffer->paintIcon(have_sub ? NEUTRINO_ICON_SUBT : NEUTRINO_ICON_SUBT_GREY, BoxEndX - (ICON_SMALL_WIDTH + 2), BoxEndY - ICON_Y_1);
}

void CInfoViewer::showFailure ()
{
  	ShowHintUTF (LOCALE_MESSAGEBOX_ERROR, g_Locale->getText (LOCALE_INFOVIEWER_NOTAVAILABLE), 430);	// UTF-8
}

void CInfoViewer::showMotorMoving (int duration)
{
	char text[256];
	char buffer[10];
	
	sprintf (buffer, "%d", duration);
	strcpy (text, g_Locale->getText (LOCALE_INFOVIEWER_MOTOR_MOVING));
	strcat (text, " (");
	strcat (text, buffer);
	strcat (text, " s)");
	
	ShowHintUTF (LOCALE_MESSAGEBOX_INFO, text, g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth (text, true) + 10, duration);	// UTF-8
}

// radiotext
void CInfoViewer::killRadiotext()
{
	frameBuffer->paintBackgroundBox(rt_x, rt_y, rt_w, rt_h);
	
#ifdef FB_BLIT
	frameBuffer->blit();
#endif
}

void CInfoViewer::showRadiotext()
{
	char stext[3][100];
	int yoff = 8, ii = 0;
	bool RTisIsUTF = false;

	if (g_Radiotext == NULL) return;
	showIcon_RadioText(g_Radiotext->haveRadiotext());

	if (g_Radiotext->S_RtOsd) 
	{
		// dimensions of radiotext window
		rt_dx = BoxEndX - BoxStartX;
		rt_dy = 25;
		rt_x = BoxStartX;
		rt_y = g_settings.screen_StartY + 10;
		rt_h = rt_y + 7 + rt_dy*(g_Radiotext->S_RtOsdRows+1)+SHADOW_OFFSET;
		rt_w = rt_x+rt_dx+SHADOW_OFFSET;
		
		int lines = 0;
		for (int i = 0; i < g_Radiotext->S_RtOsdRows; i++) {
			if (g_Radiotext->RT_Text[i][0] != '\0') lines++;
		}
		
		if (lines == 0)
		{
			frameBuffer->paintBackgroundBox(rt_x, rt_y, rt_w, rt_h);
		
#ifdef FB_BLIT
			frameBuffer->blit();
#endif
		}

		if (g_Radiotext->RT_MsgShow) 
		{
			if (g_Radiotext->S_RtOsdTitle == 1) 
			{
				// Title
				//sprintf(stext[0], g_Radiotext->RT_PTY == 0 ? "%s - %s %s%s" : "%s - %s (%s)%s",
				//g_Radiotext->RT_Titel, tr("Radiotext"), g_Radiotext->RT_PTY == 0 ? g_Radiotext->RDS_PTYN : g_Radiotext->ptynr2string(g_Radiotext->RT_PTY), g_Radiotext->RT_MsgShow ? ":" : tr("  [waiting ...]"));
				if ((lines) || (g_Radiotext->RT_PTY !=0)) 
				{
					sprintf(stext[0], g_Radiotext->RT_PTY == 0 ? "%s %s%s" : "%s (%s)%s", tr("Radiotext"), g_Radiotext->RT_PTY == 0 ? g_Radiotext->RDS_PTYN : g_Radiotext->ptynr2string(g_Radiotext->RT_PTY), ":");
					
					// shadow
					frameBuffer->paintBoxRel(rt_x+SHADOW_OFFSET, rt_y+SHADOW_OFFSET, rt_dx, rt_dy, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID, CORNER_TOP);
					frameBuffer->paintBoxRel(rt_x, rt_y, rt_dx, rt_dy, COL_INFOBAR_PLUS_0, RADIUS_MID, CORNER_TOP);
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rt_x+10, rt_y+ 30, rt_dx-20, stext[0], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
					
#ifdef FB_BLIT
					frameBuffer->blit();
#endif
				}
				yoff = 17;
				ii = 1;
#if 0
				// RDS- or Rass-Symbol, ARec-Symbol or Bitrate
				int inloff = (ftitel->Height() + 9 - 20) / 2;
				if (Rass_Flags[0][0]) {
				osd->DrawBitmap(Setup.OSDWidth-51, inloff, rass, bcolor, fcolor);
				if (ARec_Record)
					osd->DrawBitmap(Setup.OSDWidth-107, inloff, arec, bcolor, 0xFFFC1414);	// FG=Red
				else
					inloff = (ftitel->Height() + 9 - ftext->Height()) / 2;
				osd->DrawText(4, inloff, RadioAudio->bitrate, fcolor, clrTransparent, ftext, Setup.OSDWidth-59, ftext->Height(), taRight);
				}
				else 
				{
				osd->DrawBitmap(Setup.OSDWidth-84, inloff, rds, bcolor, fcolor);
				if (ARec_Record)
					osd->DrawBitmap(Setup.OSDWidth-140, inloff, arec, bcolor, 0xFFFC1414);	// FG=Red
				else
					inloff = (ftitel->Height() + 9 - ftext->Height()) / 2;
				osd->DrawText(4, inloff, RadioAudio->bitrate, fcolor, clrTransparent, ftext, Setup.OSDWidth-92, ftext->Height(), taRight);
			}
#endif
			}
			// Body
			if (lines) 
			{
				frameBuffer->paintBoxRel(rt_x+SHADOW_OFFSET, rt_y+rt_dy+SHADOW_OFFSET, rt_dx, 7+rt_dy* g_Radiotext->S_RtOsdRows, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID, CORNER_BOTTOM);
				frameBuffer->paintBoxRel(rt_x, rt_y+rt_dy, rt_dx, 7+rt_dy* g_Radiotext->S_RtOsdRows, COL_INFOBAR_PLUS_0, RADIUS_MID, CORNER_BOTTOM);

				// RT-Text roundloop
				int ind = (g_Radiotext->RT_Index == 0) ? g_Radiotext->S_RtOsdRows - 1 : g_Radiotext->RT_Index - 1;
				int rts_x = rt_x+10;
				int rts_y = rt_y+ 30;
				int rts_dx = rt_dx-20;
				
				if (g_Radiotext->S_RtOsdLoop == 1) 
				{ 
					// latest bottom
					for (int i = ind+1; i < g_Radiotext->S_RtOsdRows; i++)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
					for (int i = 0; i <= ind; i++)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
				}
				else 
				{ 
					// latest top
					for (int i = ind; i >= 0; i--)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
					for (int i = g_Radiotext->S_RtOsdRows-1; i > ind; i--)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
				}
				
#ifdef FB_BLIT
					frameBuffer->blit();
#endif				
			}
#if 0
			// + RT-Plus or PS-Text = 2 rows
			if ((S_RtOsdTags == 1 && RT_PlusShow) || S_RtOsdTags >= 2) {
				if (!RDS_PSShow || !strstr(RTP_Title, "---") || !strstr(RTP_Artist, "---")) {
					sprintf(stext[1], "> %s  %s", tr("Title  :"), RTP_Title);
					sprintf(stext[2], "> %s  %s", tr("Artist :"), RTP_Artist);
					osd->DrawText(4, 6+yoff+fheight*(ii++), stext[1], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
					osd->DrawText(4, 3+yoff+fheight*(ii++), stext[2], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
				}
				else 
				{
					char *temp = "";
					int ind = (RDS_PSIndex == 0) ? 11 : RDS_PSIndex - 1;
					for (int i = ind+1; i < 12; i++)
						asprintf(&temp, "%s%s ", temp, RDS_PSText[i]);
					for (int i = 0; i <= ind; i++)
						asprintf(&temp, "%s%s ", temp, RDS_PSText[i]);
					snprintf(stext[1], 6*9, "%s", temp);
					snprintf(stext[2], 6*9, "%s", temp+(6*9));
					free(temp);
					osd->DrawText(6, 6+yoff+fheight*ii, "[", fcolor, clrTransparent, ftext, 12, ftext->Height());
					osd->DrawText(Setup.OSDWidth-12, 6+yoff+fheight*ii, "]", fcolor, clrTransparent, ftext, Setup.OSDWidth-6, ftext->Height());
					osd->DrawText(16, 6+yoff+fheight*(ii++), stext[1], fcolor, clrTransparent, ftext, Setup.OSDWidth-16, ftext->Height(), taCenter);
					osd->DrawText(6, 3+yoff+fheight*ii, "[", fcolor, clrTransparent, ftext, 12, ftext->Height());
					osd->DrawText(Setup.OSDWidth-12, 3+yoff+fheight*ii, "]", fcolor, clrTransparent, ftext, Setup.OSDWidth-6, ftext->Height());
					osd->DrawText(16, 3+yoff+fheight*(ii++), stext[2], fcolor, clrTransparent, ftext, Setup.OSDWidth-16, ftext->Height(), taCenter);
				}
			}
#endif
		}
		
		
#if 0
		// framebuffer can only display raw images
		// show mpeg-still
		char *image;
		if (g_Radiotext->Rass_Archiv >= 0)
			asprintf(&image, "%s/Rass_%d.mpg", DataDir, g_Radiotext->Rass_Archiv);
		else
			asprintf(&image, "%s/Rass_show.mpg", DataDir);
		frameBuffer->useBackground(frameBuffer->loadBackground(image));// set useBackground true or false
		frameBuffer->paintBackground();
		
#ifdef __sh__
		frameBuffer->blit();
#endif
//		RadioAudio->SetBackgroundImage(image);
		free(image);
#endif
	}
	
	g_Radiotext->RT_MsgShow = false;

}

int CInfoViewer::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{

 	if ((msg == NeutrinoMessages::EVT_CURRENTNEXT_EPG) || (msg == NeutrinoMessages::EVT_NEXTPROGRAM)) 
	{
		//printf("CInfoViewer::handleMsg: NeutrinoMessages::EVT_CURRENTNEXT_EPG data %llx current %llx\n", *(t_channel_id *) data, channel_id & 0xFFFFFFFFFFFFULL);

		if ((*(t_channel_id *) data) == (channel_id & 0xFFFFFFFFFFFFULL)) 
		{
	  		getEPG (*(t_channel_id *) data, info_CurrentNext);
	  		if (is_visible && !fileplay)
				show_Data(true);
			
	  		showLcdPercentOver ();
		}

		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_TIMER) 
	{
		if (data == fadeTimer) 
		{
	  		// hierher kann das event nur dann kommen, wenn ein anderes Fenster im Vordergrund ist!
	  		g_RCInput->killTimer (fadeTimer);

	  		frameBuffer->setBlendLevel(g_settings.gtx_alpha);

	  		return messages_return::handled;
		} 
		else if (data == lcdUpdateTimer) 
		{
			//printf("CInfoViewer::handleMsg: lcdUpdateTimer\n");
			if ( is_visible && !fileplay)
				show_Data( true );

	  		showLcdPercentOver ();

	  		return messages_return::handled;
		} 
		else if (data == sec_timer_id) 
		{
			if(!fileplay)
			{
				showSNR();
			}

	  		return messages_return::handled;
		}
  	} 
	else if (msg == NeutrinoMessages::EVT_RECORDMODE) 
	{
		recordModeActive = data;
		if(is_visible) 
			showRecordIcon(true);
  	} 
	else if (msg == NeutrinoMessages::EVT_ZAP_GOTAPIDS) 
	{
		if ((*(t_channel_id *) data) == channel_id) 
		{
	  		if (is_visible && showButtonBar)
				showButton_Audio ();
			
			//TEST
			if (g_settings.radiotext_enable && g_Radiotext && ((CNeutrinoApp::getInstance()->getMode()) == NeutrinoMessages::mode_radio))
				g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
		}
		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_ZAP_GOTPIDS) 
	{
		if ((*(t_channel_id *) data) == channel_id) 
		{
	  		if (is_visible && showButtonBar) 
			{
				showIcon_VTXT ();
				showIcon_SubT();
				showIcon_CA_Status(0);
				showIcon_Resolution();
	  		}
		}
		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES) 
	{
		if ((*(t_channel_id *) data) == channel_id) 
		{
	  		if (is_visible && showButtonBar)
				showButton_SubServices ();
		}
		return messages_return::handled;
  	} 
  	else if ((msg == NeutrinoMessages::EVT_ZAP_COMPLETE) || (msg == NeutrinoMessages::EVT_ZAP_ISNVOD))
	{
		chanready = 1;
		showSNR();
		
		if (is_visible && showButtonBar) 
			showIcon_Resolution();
		
		channel_id = (*(t_channel_id *)data);
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_ZAP_SUB_COMPLETE) 
	{
		chanready = 1;
		showSNR();
		
		if (is_visible && showButtonBar) 
			showIcon_Resolution();

		//if ((*(t_channel_id *)data) == channel_id)
		{
	  		if (is_visible && showButtonBar && (!g_RemoteControl->are_subchannels))
				show_Data (true);
		}

		showLcdPercentOver ();

		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_ZAP_SUB_FAILED) 
	{
		chanready = 1;

		showSNR();
		
		if (is_visible && showButtonBar) 
			showIcon_Resolution();

		// show failure..!
#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD) || defined (PLATFORM_DUCKBOX)
		CVFD::getInstance()->showServicename ("(" + g_RemoteControl->getCurrentChannelName () + ')');
#endif		
		dprintf(DEBUG_NORMAL, "CInfoViewer::handleMsg: zap failed!\n");
		showFailure();

		CVFD::getInstance()->showPercentOver (255);		

		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_ZAP_FAILED) 
	{
		chanready = 1;

		showSNR();
		
		if (is_visible && showButtonBar) 
			showIcon_Resolution();

		if ((*(t_channel_id *) data) == channel_id) 
		{
	  		// show failure..!
#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD) || defined (PLATFORM_DUCKBOX)
	  		CVFD::getInstance()->showServicename ("(" + g_RemoteControl->getCurrentChannelName () + ')');
#endif			
	  		dprintf(DEBUG_NORMAL, "CInfoViewer::handleMsg: zap failed!\n");
	  		showFailure ();

	  		CVFD::getInstance()->showPercentOver(255);			
		}
		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_ZAP_MOTOR) 
	{
		chanready = 0;
		showMotorMoving (data);
		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_MODECHANGED) 
	{
		//aspectRatio = data;
		if (is_visible && showButtonBar)
	  		showIcon_16_9 ();

		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_TIMESET) 
	{
		gotTime = true;
		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_ZAP_CA_CLEAR) 
	{
		Set_CA_Status (false);
		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_ZAP_CA_LOCK) 
	{
		Set_CA_Status (true);
		return messages_return::handled;
  	} 
	else if (msg == NeutrinoMessages::EVT_ZAP_CA_FTA) 
	{
		Set_CA_Status (false);
		return messages_return::handled;
  	}
	else if (msg == NeutrinoMessages::EVT_ZAP_CA_ID) 
	{
		chanready = 1;
		Set_CA_Status(data);

		showSNR();

		return messages_return::handled;
  	}

  	return messages_return::unhandled;
}

void CInfoViewer::showButton_SubServices ()
{
  	if (!(g_RemoteControl->subChannels.empty ())) 
	{
        	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, ChanInfoX + 2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + asize + 2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2 + asize + 2, BoxEndY- ICON_Y_1 );
        	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX + 2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + asize + 2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2 + asize + 2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2, 
		BoxEndY+2, asize, g_Locale->getText((g_RemoteControl->are_subchannels) ? LOCALE_INFOVIEWER_SUBSERVICE : LOCALE_INFOVIEWER_SELECTTIME), COL_INFOBAR_BUTTONS, 0, true); // UTF-8
  	}
}

CSectionsdClient::CurrentNextInfo CInfoViewer::getEPG(const t_channel_id for_channel_id, CSectionsdClient::CurrentNextInfo &info)
{
  #if 0
	static CSectionsdClient::CurrentNextInfo oldinfo;

	//g_Sectionsd->getCurrentNextServiceKey (for_channel_id & 0xFFFFFFFFFFFFULL, info);
	sectionsd_getCurrentNextServiceKey(for_channel_id & 0xFFFFFFFFFFFFULL, info);

	dprintf(DEBUG_INFO, "CInfoViewer::getEPG: old uniqueKey %llx new %llx\n", oldinfo.current_uniqueKey, info.current_uniqueKey);
	
	if (info.current_uniqueKey != oldinfo.current_uniqueKey || info.next_uniqueKey != oldinfo.next_uniqueKey) 
	{
		if (info.flags & (CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_next)) 
		{
			CSectionsdClient::CurrentNextInfo * _info = new CSectionsdClient::CurrentNextInfo;
			*_info = info;
			//memcpy(&_info, &info, sizeof(CSectionsdClient::CurrentNextInfo));
			
			neutrino_msg_t msg;
			
			if (info.flags & CSectionsdClient::epgflags::has_current)
				msg = NeutrinoMessages::EVT_CURRENTEPG;
			else
				msg = NeutrinoMessages::EVT_NEXTEPG;
			
			g_RCInput->postMsg(msg, (unsigned) _info, false );
		} 
		else 
		{
			t_channel_id *p = new t_channel_id;
			*p = for_channel_id;
			g_RCInput->postMsg (NeutrinoMessages::EVT_NOEPG_YET, (const neutrino_msg_data_t) p, false);	// data is pointer to allocated memory
		}
		oldinfo = info;
		//memcpy(&oldinfo, &info, sizeof(CSectionsdClient::CurrentNextInfo));
	}

	return info;
#endif
	static CSectionsdClient::CurrentNextInfo oldinfo;

	//g_Sectionsd->getCurrentNextServiceKey (for_channel_id & 0xFFFFFFFFFFFFULL, info);
	sectionsd_getCurrentNextServiceKey(for_channel_id & 0xFFFFFFFFFFFFULL, info);

	printf("CInfoViewer::getEPG: old uniqueKey %llx new %llx\n", oldinfo.current_uniqueKey, info.current_uniqueKey);

	/* of there is no EPG, send an event so that parental lock can work */
	if (info.current_uniqueKey == 0 && info.next_uniqueKey == 0) 
	{
		oldinfo = info;
		t_channel_id *p = new t_channel_id;
		*p = for_channel_id;
		g_RCInput->postMsg (NeutrinoMessages::EVT_NOEPG_YET, (const neutrino_msg_data_t) p, false);
		return info;
	}

	if (info.current_uniqueKey != oldinfo.current_uniqueKey || info.next_uniqueKey != oldinfo.next_uniqueKey) 
	{
		if (info.flags & (CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_next)) 
		{
			CSectionsdClient::CurrentNextInfo * _info = new CSectionsdClient::CurrentNextInfo;
			*_info = info;
			neutrino_msg_t msg;
			
			if (info.flags & CSectionsdClient::epgflags::has_current)
				msg = NeutrinoMessages::EVT_CURRENTEPG;
			else
				msg = NeutrinoMessages::EVT_NEXTEPG;
			g_RCInput->postMsg(msg, (unsigned) _info, false );
		} 
		else 
		{
			t_channel_id *p = new t_channel_id;
			*p = for_channel_id;
			g_RCInput->postMsg (NeutrinoMessages::EVT_NOEPG_YET, (const neutrino_msg_data_t) p, false);	// data is pointer to allocated memory
		}
		oldinfo = info;
	}

	return info;
}

#define get_set CNeutrinoApp::getInstance()->getScanSettings()

void CInfoViewer::showSNR()
{
  	char percent[10];
  	uint16_t ssig, ssnr;
  	int sw, snr, sig, posx, posy;
  	int height, ChanNumYPos;
  	int barwidth = BAR_WIDTH;
	
  	if (is_visible && g_settings.infobar_sat_display) 
	{
		// freq
		if (newfreq && chanready) 
		{
	  		char freq[20];

	  		newfreq = false;
			
			// get current service info
	  		CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo();
			
			/* freq */
			if( live_fe->getInfo()->type == FE_QPSK || live_fe->getInfo()->type == FE_QAM)
			{
				sprintf (freq, "%d.%d MHz", si.tsfrequency / 1000, si.tsfrequency % 1000);
			}
			else if( live_fe->getInfo()->type == FE_OFDM)
			{
				sprintf (freq, "%d.%d MHz", si.tsfrequency / 1000000, si.tsfrequency % 1000);
			}

			int chanH = g_SignalFont->getHeight();
			int satNameWidth = g_SignalFont->getRenderWidth(freq);

			g_SignalFont->RenderString (3 + BoxStartX + ((ChanWidth - satNameWidth) / 2), BoxStartY + 2 * chanH - 3, satNameWidth, freq, COL_INFOBAR);
		
			ssig = live_fe->getSignalStrength();
			ssnr = live_fe->getSignalNoiseRatio();
					
			//show aktiv tuner
			if( FrontendCount > 1 )
			{
				char AktivTuner[255];
				
				sprintf(AktivTuner, "T%d", (live_fe->getFeIndex() + 1));
				
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX - (2*ICON_LARGE_WIDTH + 2*ICON_SMALL_WIDTH + 4*2) - 120, BoxEndY+2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2 + 2), AktivTuner, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
			}

			sig = (ssig & 0xFFFF) * 100 / 65535;
			snr = (ssnr & 0xFFFF) * 100 / 65535;
			
			height = g_SignalFont->getHeight () - 1;
			ChanNumYPos = BoxStartY + ChanHeight + 4 - 2 * height;

			if (sigscale->getPercent() != sig) 
			{
				posx = BoxStartX + 4;
				posy = ChanNumYPos + 3;

				sigscale->paint(posx, posy+4, sig);

				sprintf (percent, "%d%%S", sig);
				posx = posx + barwidth + 2;
				sw = BoxStartX + ChanWidth - posx;

				g_SignalFont->RenderString (posx, posy + height, sw, percent, COL_INFOBAR);
			}

			//SNR
			if (snrscale->getPercent() != snr) 
			{
				posx = BoxStartX + 4;
				posy = ChanNumYPos + 3 + height - 2;

				snrscale->paint(posx, posy+4, snr);

				sprintf (percent, "%d%%Q", snr);
				posx = posx + barwidth + 2;
				sw = BoxStartX + ChanWidth - posx -4;
				
				g_SignalFont->RenderString (posx, posy + height, sw, percent, COL_INFOBAR);
			}
		
		}
  	}
}

void CInfoViewer::show_Data(bool calledFromEvent)
{
  	char runningStart[10];
  	char runningRest[20];
  	char runningPercent = 0;
  	static char oldrunningPercent = 255;

  	char nextStart[10];
  	char nextDuration[10];

  	int is_nvod = false;

  	if (is_visible) 
	{
		if ((g_RemoteControl->current_channel_id == channel_id) && (g_RemoteControl->subChannels.size () > 0) && (!g_RemoteControl->are_subchannels)) 
		{
	  		is_nvod = true;
	  		info_CurrentNext.current_zeit.startzeit = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].startzeit;
	  		info_CurrentNext.current_zeit.dauer = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].dauer;
		} 
		else 
		{
	  		if ((info_CurrentNext.flags & CSectionsdClient::epgflags::has_current) && (info_CurrentNext.flags & CSectionsdClient::epgflags::has_next) && (showButtonBar)) 
			{
				if ((uint) info_CurrentNext.next_zeit.startzeit < (info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer)) {
		  			is_nvod = true;
				}
	  		}
		}

		time_t jetzt = time (NULL);

		if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current) 
		{
	  		int seit = (jetzt - info_CurrentNext.current_zeit.startzeit + 30) / 60;
	  		int rest = (info_CurrentNext.current_zeit.dauer / 60) - seit;
	  		if (seit < 0) 
			{
				runningPercent = 0;
				sprintf (runningRest, "in %d min", -seit);
	  		} 
			else 
			{
				runningPercent = (unsigned) ((float) (jetzt - info_CurrentNext.current_zeit.startzeit) / (float) info_CurrentNext.current_zeit.dauer * 100.);
				if(runningPercent > 100)
					runningPercent = 100;
				sprintf (runningRest, "%d / %d min", seit, rest);
	  		}

	  		struct tm *pStartZeit = localtime (&info_CurrentNext.current_zeit.startzeit);
	  		sprintf (runningStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
		} 
		else
			last_curr_id = 0;

		if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_next) 
		{
	  		unsigned dauer = info_CurrentNext.next_zeit.dauer / 60;
	  		sprintf (nextDuration, "%d min", dauer);
	  		struct tm *pStartZeit = localtime (&info_CurrentNext.next_zeit.startzeit);
	  		sprintf (nextStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
		} 
		else
			last_next_id = 0;

		int height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getHeight () / 3;
		int ChanInfoY = BoxStartY + ChanHeight + 15;	//+10

		if (showButtonBar) 
		{
	  		int posy = BoxStartY + 16;
	  		int height2 = 20;
			
	  		//percent
	  		if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current) 
			{
				//printf("CInfoViewer::show_Data: runningPercent %d\n", runningPercent);
				
				if(!calledFromEvent || (oldrunningPercent != runningPercent)) 
				{
					int pb_h = TIME_BAR_HEIGHT + 5;
					
					// shadow 
					frameBuffer->paintBoxRel (BoxEndX - 104, posy + 6, 108, pb_h, COL_INFOBAR_SHADOW_PLUS_0);
					
					// bg
					frameBuffer->paintBoxRel (BoxEndX - 108, posy + 2, 108, pb_h, COL_INFOBAR_PLUS_0);
					
					oldrunningPercent = runningPercent;
				}

				timescale->paint(BoxEndX - 108, posy + 2, runningPercent);
	  		} 
			else 
			{
				oldrunningPercent = 255;
				frameBuffer->paintBackgroundBoxRel (BoxEndX - 108, posy, 112, height2);
	  		}
	  		
	  		if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_anything) 
			{
				// red button
				
				// events text
				frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, ChanInfoX + 2, BoxEndY - ICON_Y_1);
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX + (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2), BoxEndY+2, asize, g_Locale->getText(LOCALE_INFOVIEWER_EVENTLIST), COL_INFOBAR_BUTTONS, 0, true); // UTF-8
	  		}
		}

		height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight ();
		int xStart = BoxStartX + ChanWidth;

		if ((info_CurrentNext.flags & CSectionsdClient::epgflags::not_broadcast) || ((calledFromEvent) && !(info_CurrentNext.flags & (CSectionsdClient::epgflags::has_next | CSectionsdClient::epgflags::has_current)))) 
		{
	  		// no EPG available
	  		ChanInfoY += height;
			
	  		frameBuffer->paintBox (ChanInfoX + 10, ChanInfoY, BoxEndX, ChanInfoY + height, COL_INFOBAR_PLUS_0);
			
			// noepg/waiting for time
	  		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (BoxStartX + ChanWidth + 20, ChanInfoY + height, BoxEndX - (BoxStartX + ChanWidth + 20), g_Locale->getText (gotTime ? LOCALE_INFOVIEWER_NOEPG : LOCALE_INFOVIEWER_WAITTIME), COL_INFOBAR, 0, true);	// UTF-8
		} 
		else 
		{
	  		// irgendein EPG gefunden
	  		int duration1Width = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth (runningRest);
	  		int duration1TextPos = BoxEndX - duration1Width - LEFT_OFFSET;

	  		int duration2Width = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(nextDuration);
	  		int duration2TextPos = BoxEndX - duration2Width - LEFT_OFFSET;

	  		if ((info_CurrentNext.flags & CSectionsdClient::epgflags::has_next) && (!(info_CurrentNext.flags & CSectionsdClient::epgflags::has_current))) 
			{
				// there are later events available - yet no current
				frameBuffer->paintBox (ChanInfoX + 10, ChanInfoY, BoxEndX, ChanInfoY + height, COL_INFOBAR_PLUS_0);
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (xStart, ChanInfoY + height, BoxEndX - xStart, g_Locale->getText(LOCALE_INFOVIEWER_NOCURRENT), COL_INFOBAR, 0, true);	// UTF-8

				ChanInfoY += height;

				if(last_next_id != info_CurrentNext.next_uniqueKey) 
				{
					frameBuffer->paintBox (ChanInfoX + 10, ChanInfoY, BoxEndX, ChanInfoY + height, COL_INFOBAR_PLUS_0);

					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (ChanInfoX + 10, ChanInfoY + height, 100, nextStart, COL_INFOBAR);
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (xStart, ChanInfoY + height, duration2TextPos - xStart - 5, info_CurrentNext.next_name, COL_INFOBAR, 0, true);
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (duration2TextPos, ChanInfoY + height, duration2Width, nextDuration, COL_INFOBAR);

					last_next_id = info_CurrentNext.next_uniqueKey;
				}
	  		} 
			else 
			{
		  		if(last_curr_id != info_CurrentNext.current_uniqueKey) 
				{
			  		frameBuffer->paintBox(ChanInfoX + 10, ChanInfoY, BoxEndX, ChanInfoY + height, COL_INFOBAR_PLUS_0);
					
			  		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (ChanInfoX + 10, ChanInfoY + height, 100, runningStart, COL_INFOBAR);
			  		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (xStart, ChanInfoY + height, duration1TextPos - xStart - 5, info_CurrentNext.current_name, COL_INFOBAR, 0, true);

			  		last_curr_id = info_CurrentNext.current_uniqueKey;
		  		}
		  		
		  		frameBuffer->paintBox(BoxEndX - 80, ChanInfoY, BoxEndX, ChanInfoY + height, COL_INFOBAR_PLUS_0);//FIXME duration1TextPos not really good
		  		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (duration1TextPos, ChanInfoY + height, duration1Width, runningRest, COL_INFOBAR);

				// next ä
				ChanInfoY += height;

				if ((!is_nvod) && (info_CurrentNext.flags & CSectionsdClient::epgflags::has_next)) 
				{
					if(last_next_id != info_CurrentNext.next_uniqueKey) 
					{
						frameBuffer->paintBox (ChanInfoX + 10, ChanInfoY, BoxEndX, ChanInfoY + height, COL_INFOBAR_PLUS_0);

						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (ChanInfoX + 10, ChanInfoY + height, 100, nextStart, /*COL_INFOBAR*/ COL_MENUCONTENTINACTIVE);
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (xStart, ChanInfoY + height, duration2TextPos - xStart - 5, info_CurrentNext.next_name, /*COL_INFOBAR*/COL_MENUCONTENTINACTIVE, 0, true);
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (duration2TextPos, ChanInfoY + height, duration2Width, nextDuration, /*COL_INFOBAR*/COL_MENUCONTENTINACTIVE );

						last_next_id = info_CurrentNext.next_uniqueKey;
					}
				} 
	  		}
		}
  	}
}

void CInfoViewer::showIcon_Audio(const int ac3state) const
{
	const char *dd_icon;
	
	switch (ac3state)
	{
		case AC3_ACTIVE:
			dd_icon = NEUTRINO_ICON_DD;
			//test
			CVFD::getInstance()->ShowIcon(VFD_ICON_DOLBY, true); //FIXME: remove to remotecontrol
			break;
		case AC3_AVAILABLE:
			dd_icon = NEUTRINO_ICON_DD_AVAIL;
			//test
			CVFD::getInstance()->ShowIcon(VFD_ICON_DOLBY, false); //FIXME: remove to remotecontrol
			break;
		case NO_AC3:
		default:
			dd_icon = NEUTRINO_ICON_DD_GREY;
			//test
			CVFD::getInstance()->ShowIcon(VFD_ICON_DOLBY, false); //FIXME: remove to remotecontrol
			break;
	}

	frameBuffer->paintIcon(dd_icon, BoxEndX - (ICON_LARGE_WIDTH + 2*ICON_SMALL_WIDTH + 3*2), BoxEndY - ICON_Y_1);
}

void CInfoViewer::showButton_Audio()
{
  	// green, in case of several APIDs
  	uint32_t count = g_RemoteControl->current_PIDs.APIDs.size();
  	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, ChanInfoX + 2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + asize + 2, BoxEndY- ICON_Y_1);

  	if (count > 0) 
	{
		int sx = ChanInfoX + 2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + asize + 2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2;

		//frameBuffer->paintBox (sx, BoxEndY-20, sx+asize, BoxEndY, COL_INFOBAR_BUTTONS_BACKGROUND);

		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(sx, BoxEndY+2, asize, g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES), COL_INFOBAR_BUTTONS, 0, true); // UTF-8
  	}

  	//const char *dd_icon;
	int ac3state;
  	if ((g_RemoteControl->current_PIDs.PIDs.selected_apid < count) && (g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].is_ac3))
	{
		ac3state = AC3_ACTIVE;
	}
  	else if (g_RemoteControl->has_ac3)
	{
		ac3state = AC3_AVAILABLE;
	}
  	else
	{
		ac3state = NO_AC3;
	}


	showIcon_Audio(ac3state);
}

void CInfoViewer::killTitle ()
{
  	if (is_visible) 
	{
		is_visible = false;
		
		if(fileplay)
		{
			delete moviescale;
			moviescale = NULL;
			MoviePercent = 0;
		}

		frameBuffer->paintBackgroundBox (BoxStartX, BoxStartY, BoxEndX + SHADOW_OFFSET, BoxEndY + SHADOW_OFFSET );
		
#ifdef FB_BLIT		
		frameBuffer->blit();
#endif

		// hide radiotext
		if (g_settings.radiotext_enable && g_Radiotext) 
		{
			g_Radiotext->S_RtOsd = g_Radiotext->haveRadiotext() ? 1 : 0;
			killRadiotext();
		}
  	}
}

void CInfoViewer::Set_CA_Status (int Status)
{
	CA_Status = Status;
	m_CA_Status = Status;

	if (is_visible && showButtonBar)
		showIcon_CA_Status(1);
}

void CInfoViewer::showLcdPercentOver()
{
	if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] != 1) 
	{
		
		//if (fileplay || (NeutrinoMessages::mode_ts == CNeutrinoApp::getInstance()->getMode())) 
		//{
		//	CVFD::getInstance()->showPercentOver(file_prozent);
		//	return;
		//}

		int runningPercent = -1;
		time_t jetzt = time (NULL);

		if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current) 
		{
			if (jetzt < info_CurrentNext.current_zeit.startzeit)
				runningPercent = 0;
			else
				runningPercent = MIN ((unsigned) ((float) (jetzt - info_CurrentNext.current_zeit.startzeit) / (float) info_CurrentNext.current_zeit.dauer * 100.), 100);
		}

		CVFD::getInstance()->showPercentOver(runningPercent);
	}
}

extern int pmt_caids[11];

void CInfoViewer::showIcon_CA_Status(int notfirst)
{
	int i;
	int caids[] = { 0x0600, 0x1700, 0x0100, 0x0500, 0x1800, 0x0B00, 0x0D00, 0x0900, 0x2600, 0x4a00, 0x0E00 };
		
	if(!notfirst) 
	{
		bool fta = true;
			
		for(i=0; i < (int)(sizeof(caids)/sizeof(int)); i++) 
		{
			if (pmt_caids[i]) 
			{
				fta = false;
				break;
			}
		}
			
		frameBuffer->paintIcon( fta ? "ca2_gray" : "ca2", BoxEndX - (2*ICON_LARGE_WIDTH + 2*ICON_SMALL_WIDTH + 4*2) - 85, BoxEndY - ICON_Y_1);
#if defined (PLATFORM_DUCKBOX)			
		CVFD::getInstance()->ShowIcon(VFD_ICON_LOCK, !fta);
#endif			
		return;
	}
}

void CInfoViewer::showEpgInfo()   //message on event change
{
	int mode = CNeutrinoApp::getInstance()->getMode();
	
	/* show epg info only if we in TV- or Radio mode and current event is not the same like before */
	if ((eventname != info_CurrentNext.current_name) && (mode == NeutrinoMessages::mode_tv || mode == NeutrinoMessages::mode_radio))
	{
		eventname = info_CurrentNext.current_name;
		//if (g_settings.infobar_show)
		g_RCInput->postMsg(NeutrinoMessages::SHOW_INFOBAR , 0);
#if 0
		/* let's check if this is still needed */
		else
			/* don't show anything, but update the LCD
			   TODO: we should not have to update the LCD from the _infoviewer_.
				 they have nothing to do with each other */
			showLcdPercentOver();
#endif
	}
}

int CInfoViewerHandler::exec (CMenuTarget * parent, const std::string & actionkey)
{
	int res = menu_return::RETURN_EXIT_ALL;
	CChannelList *channelList;
	CInfoViewer *i;
	
	if (parent) 
	{
		parent->hide ();
	}
	
	i = new CInfoViewer;
	
	channelList = CNeutrinoApp::getInstance()->channelList;
	i->start();
	i->showTitle (channelList->getActiveChannelNumber(), channelList->getActiveChannelName(), channelList->getActiveSatellitePosition(), channelList->getActiveChannel_ChannelID ());	// UTF-8
	delete i;
	
	return res;
}


