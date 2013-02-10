/*
	Neutrino-GUI  -   DBoxII-Project

	Timerliste by Zwen
	
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

#include <global.h>
#include <neutrino.h>
#include <gui/timeosd.h>
#include <driver/fontrenderer.h>
#include <system/settings.h>
#include <gui/widget/progressbar.h>
#include <driver/rcinput.h>

#include <video_cs.h>


extern cVideo * videoDecoder;

static CProgressBar * timescale;

#define TIMEOSD_FONT SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME
#define TIMEBARH 38
#define BARLEN 200
#define SHADOW_OFFSET	5


CTimeOSD::CTimeOSD()
{
	frameBuffer = CFrameBuffer::getInstance();
	visible = false;
	m_mode = MODE_ASC;
	GetDimensions();

	if(!timescale)
		timescale = new CProgressBar( BoxWidth - 15, 6, 40, 100, 70, true );
}

CTimeOSD::~CTimeOSD()
{
	hide();

	if(timescale) 
	{
		delete timescale;
		timescale = 0;
	}
}

extern CMoviePlayerGui::state playstate;
extern int speed;
extern unsigned int ac3state;
//extern int position;
extern int duration;
extern std::string g_file_epg;
extern std::string g_file_epg1;
extern bool isMovieBrowser;
extern int timeshift;
extern bool isHTTP;

void CTimeOSD::show(int Position)
{	
	// show / update
	GetDimensions();
	visible = true;
	//m_time_dis  = time(NULL);
	//m_time_show = time_show;
	
	// timescale
	timescale->reset();
	  
	// time shadow
	//frameBuffer->paintBoxRel(m_xend - m_width - 10 + SHADOW_OFFSET, m_y + SHADOW_OFFSET, m_width + 10, m_height, COL_INFOBAR_SHADOW_PLUS_0);
	
	if(!timeshift)
	{
		// paint shadow
		frameBuffer->paintBoxRel(BoxStartX + SHADOW_OFFSET, BoxStartY + SHADOW_OFFSET, BoxWidth, BoxHeight, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID, CORNER_TOP );
		
		// paint info box
		frameBuffer->paintBoxRel(BoxStartX, BoxStartY, BoxWidth, BoxHeight, COL_INFOBAR_PLUS_0, RADIUS_MID, CORNER_TOP ); 
		
		// timescale bg
		frameBuffer->paintBoxRel(BoxStartX + 10, BoxStartY + 15, BoxWidth - 20, 6, COL_INFOBAR_SHADOW_PLUS_1 ); 
		
		// bottum bar
		frameBuffer->paintBoxRel(BoxStartX, BoxStartY + (BoxHeight - 20), BoxWidth, 20, COL_INFOBAR_SHADOW_PLUS_1 ); 
		
		// mp icon
		int m_icon_w = 0;
		int m_icon_h = 0;
		
		frameBuffer->getIconSize("mp", &m_icon_w, &m_icon_h);

		int m_icon_x = BoxStartX + 5;
		int m_icon_y = BoxStartY + (BoxHeight - m_icon_h) / 2;
		frameBuffer->paintIcon("mp", m_icon_x, m_icon_y);
		
		// paint buttons
		// red
		// movie info
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, BoxStartX + 2, BoxEndY - 18);
		if( isMovieBrowser || isHTTP)
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString( BoxStartX + 2 + 16 + 2, BoxEndY + 2, BoxWidth/5, (char *)"Movie Info", (COL_INFOBAR_SHADOW + 1), 0, true); // UTF-8
		
		// green
		// audio
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, BoxStartX + BoxWidth/5, BoxEndY - 18);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString( BoxStartX + (BoxWidth/5)+ 18, BoxEndY + 2, BoxWidth/5, g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES), (COL_INFOBAR_SHADOW + 1), 0, true); // UTF-8
		
		// yellow
		// help
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, BoxStartX + (BoxWidth/5)*2, BoxEndY - 18);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString( BoxStartX + (BoxWidth/5)*2 + 18, BoxEndY + 2, BoxWidth/5, (char *)"help", (COL_INFOBAR_SHADOW * 1), 0, true); // UTF-8
		
		// blue
		// bookmark
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, BoxStartX + (BoxWidth/5)*3, BoxEndY - 18);
		if(isMovieBrowser)
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString( BoxStartX + (BoxWidth/5)*3 + 18, BoxEndY + 2, BoxWidth/5, g_Locale->getText(LOCALE_MOVIEPLAYER_BOOKMARK), (COL_INFOBAR_SHADOW + 1), 0, true); // UTF-8
		
		/* mp keys */
		frameBuffer->paintIcon("ico_mp_rewind", BoxEndX - 60 - 16*5, BoxEndY - 18);
		frameBuffer->paintIcon("ico_mp_play", BoxEndX - 60 - 16*4, BoxEndY - 18);
		frameBuffer->paintIcon("ico_mp_pause", BoxEndX - 60 - 16*3, BoxEndY - 18);
		frameBuffer->paintIcon("ico_mp_stop", BoxEndX - 60 - 16*2, BoxEndY - 18);
		frameBuffer->paintIcon("ico_mp_forward", BoxEndX - 60 - 16, BoxEndY - 18);
		
		// ac3
		frameBuffer->paintIcon( (ac3state == CInfoViewer::AC3_ACTIVE)?NEUTRINO_ICON_DD:NEUTRINO_ICON_DD_GREY, BoxEndX - 2 - 26, BoxEndY - 18);
		
		// 4:3/16:9
		const char * aspect_icon = NEUTRINO_ICON_16_9_GREY;
				
		if(videoDecoder->getAspectRatio() == 1)
			aspect_icon = NEUTRINO_ICON_16_9;
				
		frameBuffer->paintIcon(aspect_icon, BoxEndX - 2 - 55, BoxEndY - 18);
		
		//playstate
		const char *icon = "mp_play";
		
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

		//int icon_x = BoxStartX + 60 + 5;
		int icon_x = BoxStartX + 5 + m_icon_w + 10;
		int icon_y = BoxStartY + (BoxHeight - icon_h) / 2;
		

		frameBuffer->paintIcon(icon, icon_x, icon_y);
		
		// paint speed
		char strSpeed[4];
		if( playstate == CMoviePlayerGui::FF || playstate == CMoviePlayerGui::REW )
		{
			sprintf(strSpeed, "%d", speed);
			
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->RenderString(icon_x + icon_w + 5, BoxStartY + (BoxHeight/3)*2, BoxWidth/5, strSpeed, COL_COLORED_EVENTS_INFOBAR ); // UTF-8
		}
		
		// infos
		
		// duration
		char runningTotal[32]; // %d can be 10 digits max...	
		//sprintf(runningTotal, "%d / %d min", Position/60, (duration + 30000) / 60000 );	
		sprintf(runningTotal, "%d min", (duration + 30000) / 60000 );	
		
		int durationWidth = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(runningTotal);
		int durationTextPos = BoxEndX - durationWidth - 15;
		
		int speedWidth = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth("-8");
		
		int InfoStartX = BoxStartX + 5 + m_icon_w + 10 + icon_w + 5 + speedWidth + 20;
		int InfoWidth = durationTextPos - InfoStartX;
		
		//Title 1
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (InfoStartX, BoxStartY + BoxHeight/2 - 5, InfoWidth, g_file_epg, COL_INFOBAR, 0, true);

		//Title2
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (InfoStartX, BoxStartY + BoxHeight/2 + 25, InfoWidth, g_file_epg1, COL_INFOBAR, 0, true);

		// duration
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(durationTextPos, BoxStartY + BoxHeight/2 - 5, durationWidth, runningTotal, COL_INFOBAR);
		
		// runningrest
		
		// runningpercent
		//char running[32];
		//sprintf(running, "%d min", Position/60);
		//g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(durationTextPos, BoxStartY + BoxHeight/2 + 25, durationWidth, running, COL_INFOBAR);
	}
	
	// time
	//update();
}

void CTimeOSD::GetDimensions()
{
	// time
	m_xstart = g_settings.screen_StartX + 10;
	m_xend = g_settings.screen_EndX - 10;
	m_height = g_Font[TIMEOSD_FONT]->getHeight();
	m_y = g_settings.screen_StartY + 10;
	m_width = g_Font[TIMEOSD_FONT]->getRenderWidth("00:00:00");
	twidth = m_xend - m_xstart;
	
	// infobar
	BoxStartX = m_xstart;
	BoxWidth = m_xend - m_xstart;
	BoxHeight = TIMEBARH * 3;
	BoxStartY = g_settings.screen_EndY - BoxHeight - 10;
	BoxEndY = BoxStartY + BoxHeight;
	BoxEndX = m_xend;
}

void CTimeOSD::update(time_t time_show)
{
	time_t tDisplayTime;
	static time_t oldDisplayTime = 0;
	char cDisplayTime[8 + 1];
	fb_pixel_t color1, color2;

	//printf("CTimeOSD::update time %ld\n", time_show);
	
	if(!visible)
		return;

	if(m_mode == MODE_ASC) 
	{
		color1 = COL_MENUCONTENT_PLUS_0;
		color2 = COL_MENUCONTENT;
	} 
	else 
	{
		color1 = COL_MENUCONTENTSELECTED_PLUS_0;
		color2 = COL_MENUCONTENTSELECTED;
		
		if(!time_show) 
			time_show = 1;
	}

	if(time_show) 
	{
		m_time_show = time_show;
		tDisplayTime = m_time_show;
	} 
	else 
	{
		if(m_mode == MODE_ASC) 
		{
			tDisplayTime = m_time_show + (time(NULL) - m_time_dis);
		} 
		else 
		{
			tDisplayTime = m_time_show + (m_time_dis - time(NULL));
		}
	}

	if(tDisplayTime < 0)
		tDisplayTime = 0;

	if(tDisplayTime != oldDisplayTime) 
	{
		oldDisplayTime = tDisplayTime;
		strftime(cDisplayTime, 9, "%T", gmtime(&tDisplayTime));
		
		// time shadow
		frameBuffer->paintBoxRel(m_xend - m_width - 10 + SHADOW_OFFSET, m_y + SHADOW_OFFSET, m_width + 10, m_height, COL_INFOBAR_SHADOW_PLUS_0);

		// time window
		frameBuffer->paintBoxRel(m_xend - m_width - 10, m_y, m_width + 10, m_height, color1 );

		// time
		g_Font[TIMEOSD_FONT]->RenderString(m_xend - m_width - 5, m_y + m_height, m_width + 5, cDisplayTime, color2);
	}
	
#if !defined USE_OPENGL
	frameBuffer->blit();
#endif
}

void CTimeOSD::updatePos(short runningPercent)
{
	if(!timeshift)
		timescale->paint(BoxStartX + 10, BoxStartY + 15, runningPercent);
}

void CTimeOSD::hide()
{
	GetDimensions();
	
	//printf("CTimeOSD::hide: x %d y %d xend %d yend %d\n", m_xstart, m_y , m_xend, m_height + 15);

	if(!visible)
		return;

	// hide time
	frameBuffer->paintBackgroundBoxRel(m_xend - m_width - 10, m_y, m_width + 10 + SHADOW_OFFSET, m_height + SHADOW_OFFSET );
	
	// hide infobar
	frameBuffer->paintBackgroundBoxRel(BoxStartX, BoxStartY, BoxWidth + SHADOW_OFFSET, BoxHeight + SHADOW_OFFSET );
	
#if !defined USE_OPENGL
	frameBuffer->blit();
#endif
	visible = false;
	
	timescale->reset();
}
