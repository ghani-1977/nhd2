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
#include <gui/scale.h>
#include <driver/rcinput.h>

//static CScale * timescale;

#define TIMEOSD_FONT SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME
#define TIMEBARH 38
#define BARLEN 200

// in us
#define FADE_TIME 4000000

CTimeOSD::CTimeOSD()
{
	frameBuffer = CFrameBuffer::getInstance();
	visible=false;
	m_mode=MODE_ASC;
	GetDimensions();

	//if(!timescale)
	//	timescale = new CScale(200, 32, 40, 100, 70, true);
}

CTimeOSD::~CTimeOSD()
{
	hide();

	//if(timescale) 
	//{
	//	delete timescale;
	//	timescale = 0;
	//}
}

void CTimeOSD::show(time_t time_show)
{	
	//TEST
	#if 0
	int fadeValue;
	bool fadeIn = false;

	if (fadeIn)
		fadeTimer = g_RCInput->addTimer (FADE_TIME, false);

	if (fadeIn) 
	{
		fadeValue = 0x10;
		frameBuffer->setBlendLevel(fadeValue);
	} 
	else
		fadeValue = g_settings.gtx_alpha;
	#endif	
	
	// show / update
	GetDimensions();
	visible = true;
	m_time_dis  = time(NULL);
	m_time_show = time_show;
	
	// timescale
	//frameBuffer->paintBoxRel(m_xstart-2, m_y, 2+BARLEN+2, TIMEBARH, COL_INFOBAR_SHADOW_PLUS_0); //border
	//timescale->reset();
	
	update();
	//
	
	#if 0
	bool fadeOut = false;
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	//if (true) 
	{
		bool hideIt = true;

		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd (g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR] == 0 ? 0xFFFF : g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR]);

		int res = messages_return::none;

		while (!(res & (messages_return::cancel_info | messages_return::cancel_all))) 
		{
			g_RCInput->getMsgAbsoluteTimeout (&msg, &data, &timeoutEnd);

			if ((msg == CRCInput::RC_ok) || (msg == CRCInput::RC_home) || (msg == CRCInput::RC_timeout)) 
			{
				if (fadeIn) 
				{
					g_RCInput->killTimer(fadeTimer);
					fadeIn = false;
				}

				if ( !fadeOut ) 
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
			/*else
			{
				//test
				res = CNeutrinoApp::getInstance()->handleMsg(msg, data);

				if (res & messages_return::unhandled) 
				{
					// raus hier und im Hauptfenster behandeln...
					g_RCInput->postMsg (msg, data);
					res = messages_return::cancel_info;
				}
			}*/
			
#ifdef FB_BLIT
			//frameBuffer->blit();
#endif			
		}

		if (hideIt)
			hide();

		//g_RCInput->killTimer(sec_timer_id);
		//sec_timer_id = 0;

		if (fadeIn || fadeOut) 
		{
			g_RCInput->killTimer(fadeTimer);

			frameBuffer->setBlendLevel(g_settings.gtx_alpha);
		}
	}
	#endif
}

void CTimeOSD::GetDimensions()
{
	m_xstart = g_settings.screen_StartX + 10;
	m_xend = g_settings.screen_EndX - 10;
	m_height = g_Font[TIMEOSD_FONT]->getHeight();
	m_y = g_settings.screen_StartY + 10;
	m_width = g_Font[TIMEOSD_FONT]->getRenderWidth("00:00:00");
}

void CTimeOSD::update(time_t time_show)
{
	time_t tDisplayTime;
	static time_t oldDisplayTime = 0;
	char cDisplayTime[8+1];
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
		if(!time_show) time_show = 1;
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
		tDisplayTime=0;

	if(tDisplayTime != oldDisplayTime) 
	{
		oldDisplayTime = tDisplayTime;
		strftime(cDisplayTime, 9, "%T", gmtime(&tDisplayTime));

		frameBuffer->paintBoxRel(m_xend - m_width - 10, m_y, m_width + 10, m_height, color1, RADIUS_MID, CORNER_BOTH);

		g_Font[TIMEOSD_FONT]->RenderString(m_xend - m_width - 5,  m_y + m_height, m_width +5,    cDisplayTime, color2);
	}
	
#ifdef FB_BLIT
	frameBuffer->blit();
#endif	
}

void CTimeOSD::updatePos(short runningPercent)
{
	//timescale->paint(m_xstart, m_y, runningPercent);
}

void CTimeOSD::hide()
{
	GetDimensions();
	printf("CTimeOSD::hide: x %d y %d xend %d yend %d\n", m_xstart, m_y , m_xend, m_height + 15);

	if(!visible)
		return;

	frameBuffer->paintBackgroundBoxRel(m_xstart - 10, m_y - 10, m_xend, m_height + 15);
#ifdef FB_BLIT
	frameBuffer->blit();
#endif
	visible=false;
	
	//timescale->reset();
}
