f/*
	WebTV

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

#include <stdio.h>

#include <global.h>
#include <neutrino.h>
#include <driver/screen_max.h>
#include "movieplayer.h"
#include "webtv.h"
#include <gui/widget/buttons.h>

#include <video_cs.h>

/* zapit includes */
#include <channel.h>
#include <gui/pictureviewer.h>


#define DEFAULT_WEBTV_XMLFILE 		CONFIGDIR "/webtv.xml"
#define DEFAULT_IPTV_FILE		CONFIGDIR "iptv.tv"

extern cVideo * videoDecoder;
extern t_channel_id live_channel_id;		// zapit.cpp
extern CZapitChannel * live_channel;		// zapit.cpp

#define PIC_W 78

extern CPictureViewer * g_PicViewer;

CWebTV::CWebTV(int Mode)
{
	frameBuffer = CFrameBuffer::getInstance();
	
	selected = 0;
	liststart = 0;
	
	parser = NULL;
	
	mode = Mode;
}

CWebTV::~CWebTV()
{
	if (parser)
	{
		xmlFreeDoc(parser);
		parser = NULL;
	}
	
	for(unsigned int count = 0; count < channels.size(); count++)
	{
		delete channels[count];
	}
	channels.clear();
}

int CWebTV::exec()
{
	if(mode == WEBTV)
		readXml();
	else if(mode = IPTV)
		readIPTVlist();
	
	return Show();
}

CFile * CWebTV::getSelectedFile()
{
	if ((!(filelist.empty())) && (!(filelist[selected].Name.empty())))
		return &filelist[selected];
	else
		return NULL;
}

// readxml file
bool CWebTV::readXml()
{
	CFile file;
	
	for(unsigned int count = 0; count < channels.size(); count++)
	{
		delete channels[count];
	}
	channels.clear();
	
	webtv_channels * Channels_list = new webtv_channels();
	
	if (parser)
	{
		xmlFreeDoc(parser);
		parser = NULL;
	}
	
	parser = parseXmlFile(DEFAULT_WEBTV_XMLFILE);
	
	if (parser) 
	{
		xmlNodePtr l0 = NULL;
		xmlNodePtr l1 = NULL;
		l0 = xmlDocGetRootElement(parser);
		l1 = l0->xmlChildrenNode;
		
		if (l1) 
		{
			while ((xmlGetNextOccurence(l1, "webtv"))) 
			{
				char * title = xmlGetAttribute(l1, (char *)"title");
				char * url = xmlGetAttribute(l1, (char *)"url");
				char * description = xmlGetAttribute(l1, (char *)"description");
				char * locked = xmlGetAttribute(l1, (char *)"locked");
				
				bool ChLocked = locked ? (strcmp(locked, "1") == 0) : false;
				
				// fill webtv list
				Channels_list = new webtv_channels();
				
				Channels_list->title = title;
				Channels_list->url = url;
				Channels_list->description = description;
				Channels_list->locked = locked;
				
				// parentallock
				if ((g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_ONSIGNAL) && (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_CHANGETOLOCKED))
					ChLocked = false;			
			
				if(!ChLocked)
					channels.push_back(Channels_list);
				
				file.Url = url;
				file.Name = title;
				file.Description = description;
				
				filelist.push_back(file);

				l1 = l1->xmlNextNode;
			}
		}
		
		return true;
	}
	
	xmlFreeDoc(parser);
	
	return false;
}

//TODO:
bool CWebTV::readIPTVlist()
{
	/* set our iptv list */
	CFile file;
	
	for(unsigned int count = 0; count < IPTVChannels.size(); count++)
	{
		delete IPTVChannels[count];
	}
	IPTVChannels.clear();
	
	webtv_channels * IPTVChannels_list = new webtv_channels();
	
	std::string name, service, description;
	
	FILE * f = fopen(DEFAULT_IPTV_FILE, "r");
	if (!f)
	{
		return false;
	}
	
	while (1)
	{
		char line[1024];
		if (!fgets(line, 1024, f))
			break;
		
		size_t len = strlen(line);
		
		// skip lines with less than one char
		if (len < 2)
			continue;
		
		/* strip newline */
		line[--len] = 0;
		
		/* strip carriage return (when found) */
		if (line[len - 1] == '\r') 
			line[--len] = 0;
		
		if (!strncmp(line, "#NAME ", 6))
			name = line + 6;
		else if (strncmp(line, "#SERVICE ", 9) == 0)
			service = line + 9;
		else if (strncmp(line, "#DESCRIPTION ", 13) == 0)
			description = line + 12;
	}
	
	fclose(f);
	
	return false;
}

int CWebTV::Show()
{
	bool res = false;
	
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	
	// windows size
	if(g_settings.mini_tv)
	{
		width  = 755;
		height = 600;
	}
	else
	{
		width  = w_max ( (frameBuffer->getScreenWidth() / 20 * 17), (frameBuffer->getScreenWidth() / 20 ));
		height = h_max ( (frameBuffer->getScreenHeight() / 20 * 16), (frameBuffer->getScreenHeight() / 20));
	}

	// display channame in vfd	
	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8 );
	
	buttonHeight = 7 + std::min(16, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight());
	theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();

	fheight = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight();
	listmaxshow = (height - theight - buttonHeight)/fheight;
	height = theight + buttonHeight + listmaxshow * fheight;
	info_height = fheight + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getHeight() + 10;
	
	if(g_settings.mini_tv)
	{
		x = frameBuffer->getScreenX() + 10;
		y = frameBuffer->getScreenY() + 10;
	}
	else
	{
		x = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - width) / 2;
		y = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - (height + info_height)) / 2;
	}
	
	// head
	paintHead();
	
	if(g_settings.mini_tv)
		paintMiniTV();
	
	// paint all
	paint();
		
#if !defined USE_OPENGL
	frameBuffer->blit();
#endif

	int oldselected = selected;

	// loop control
	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);
	bool loop = true;
	
	while (loop) 
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
		
		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

		if ( msg == CRCInput::RC_timeout )
		{
			selected = oldselected;
			
			loop = false;
		}
		else if ( msg == CRCInput::RC_up || (int) msg == g_settings.key_channelList_pageup )
                {
                        int step = 0;
                        int prev_selected = selected;

			step =  ((int) msg == g_settings.key_channelList_pageup) ? listmaxshow : 1;  // browse or step 1
                        selected -= step;
                        if((prev_selected-step) < 0)            // because of uint
                                selected = channels.size() - 1;

                        paintItem(prev_selected - liststart);
			
                        unsigned int oldliststart = liststart;
                        liststart = (selected/listmaxshow)*listmaxshow;
                        if(oldliststart!=liststart)
                                paint();
                        else
                                paintItem(selected - liststart);
                }
                else if ( msg == CRCInput::RC_down || (int) msg == g_settings.key_channelList_pagedown )
                {
                        unsigned int step = 0;
                        int prev_selected = selected;

			step =  ((int) msg == g_settings.key_channelList_pagedown) ? listmaxshow : 1;  // browse or step 1
                        selected += step;

                        if(selected >= channels.size()) 
			{
                                if (((channels.size() / listmaxshow) + 1) * listmaxshow == channels.size() + listmaxshow) 	// last page has full entries
                                        selected = 0;
                                else
                                        selected = ((step == listmaxshow) && (selected < (((channels.size() / listmaxshow)+1) * listmaxshow))) ? (channels.size() - 1) : 0;
			}

                        paintItem(prev_selected - liststart);
			
                        unsigned int oldliststart = liststart;
                        liststart = (selected/listmaxshow)*listmaxshow;
                        if(oldliststart != liststart)
                                paint();
                        else
                                paintItem(selected - liststart);
                }
                else if ( msg == CRCInput::RC_red || msg == CRCInput::RC_ok || msg == (neutrino_msg_t) g_settings.mpkey_play) 
		{	  
			filelist[selected].Url = channels[selected]->url;
			filelist[selected].Name = channels[selected]->title;
			filelist[selected].Description = channels[selected]->description;
			
			res = true;
		
			loop = false;
		}
		else if ( msg == CRCInput::RC_green || msg == CRCInput::RC_home ) 
		{
			loop = false;
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
		{
			loop = false;
		}
			
#if !defined USE_OPENGL
		frameBuffer->blit();
#endif		
	}
	
	hide();
			
	return (res);
}

void CWebTV::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width + 5, height + info_height + 5);
			
        clearItem2DetailsLine();
	
	//
	if(g_settings.mini_tv)
		frameBuffer->paintBackgroundBoxRel(830, y, 400, 660);
	//
	
#if !defined USE_OPENGL
	frameBuffer->blit();
#endif	

	//
	if(g_settings.mini_tv)
		videoDecoder->Pig(-1, -1, -1, -1);
	//
}

void CWebTV::paintItem(int pos)
{
	int ypos = y + theight + pos*fheight;
	uint8_t    color;
	fb_pixel_t bgcolor;
	unsigned int curr = liststart + pos;
	
	if (curr == selected) 
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		
		// itemlines	
		paintItem2DetailsLine(pos, curr);		
		
		// details
		paintDetails(curr);

		// itembox
		frameBuffer->paintBoxRel(x, ypos, width - 15, fheight, bgcolor);
	} 
	else 
	{
		color = COL_MENUCONTENT;
		bgcolor = COL_MENUCONTENT_PLUS_0;
		
		// itembox
		frameBuffer->paintBoxRel(x, ypos, width - 15, fheight, bgcolor);
	}

	//name and description
	if(curr < channels.size()) 
	{
		char tmp[10];
		char nameAndDescription[255];
		int l = 0;
		
		sprintf((char*) tmp, "%d", curr + 1);
		l = snprintf(nameAndDescription, sizeof(nameAndDescription), "%s", channels[curr]->title);
		
		// nummer
		int numpos = x + 10 + numwidth - g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(tmp);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(numpos, ypos + fheight, numwidth + 5, tmp, color, fheight);
		
		// description
		std::string Descr = channels[curr]->description;
		if(!(Descr.empty()))
		{
			snprintf(nameAndDescription + l, sizeof(nameAndDescription) -l, "  -  ");
			
			unsigned int ch_name_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(nameAndDescription, true);
			unsigned int ch_desc_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getRenderWidth(channels[curr]->description, true);
			
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + 10 + numwidth + 10, ypos + fheight, width - numwidth - 20 - 15, nameAndDescription, color, 0, true);
			
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString(x + 5 + numwidth + 10 + ch_name_len, ypos + fheight, ch_desc_len, channels[curr]->description, (curr == selected)?COL_MENUCONTENTSELECTED : COL_COLORED_EVENTS_CHANNELLIST, 0, true);
		}
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + 10 + numwidth + 10, ypos + fheight, width - numwidth - 20 - 15, nameAndDescription, color, 0, true);
	}
}

#define NUM_LIST_BUTTONS 2
struct button_label CWebTVButtons[NUM_LIST_BUTTONS] =
{
	{ NEUTRINO_ICON_BUTTON_RED, LOCALE_AUDIOPLAYER_PLAY},
	{ NEUTRINO_ICON_BUTTON_GREEN, LOCALE_MENU_EXIT},
};

// paint head
void CWebTV::paintHead()
{
	// head
	frameBuffer->paintBoxRel(x, y, width, theight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP); //round
	
	// foot
	int ButtonWidth = (width - 20) / 4;
	
	frameBuffer->paintBoxRel(x, y + (height - buttonHeight), width, buttonHeight - 1, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_BOTTOM); //round
	
	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + (height - buttonHeight) + 3, ButtonWidth, NUM_LIST_BUTTONS, CWebTVButtons);
	
	// head icon
	int icon_w, icon_h;
	frameBuffer->getIconSize(NEUTRINO_ICON_STREAMING, &icon_w, &icon_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_STREAMING, x + 10, y + ( theight - icon_h)/2 );
	
	// paint time/date
	int timestr_len = 0;
	char timestr[18];
	
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	
	bool gotTime = g_Sectionsd->getIsTimeSet();

	if(gotTime)
	{
		strftime(timestr, 18, "%d.%m.%Y %H:%M", tm);
		timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(timestr, true); // UTF-8
		
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->RenderString(x + width - 20 - timestr_len, y + g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getHeight() + 5, timestr_len+1, timestr, COL_MENUHEAD, 0, true); // UTF-8 // 100 is pic_w refresh box
	}
	
	//head title
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + 10 + icon_w + 10, y + theight, width - 20 - icon_w - timestr_len, g_Locale->getText(LOCALE_WEBTV_HEAD), COL_MENUHEAD, 0, true); // UTF-8
}

// infos
void CWebTV::paintDetails(int index)
{
	// infobox refresh
	frameBuffer->paintBoxRel(x + 2, y + height + 2, width - 4, info_height - 4, COL_MENUCONTENTDARK_PLUS_0);
	
	//if(channels.empty()
	//	return;
	
	// name/description
	g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + 10, y + height + 5 + fheight, width - 30, channels[index]->title, COL_MENUCONTENTDARK, 0, true);
	g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString (x + 10, y+ height + 5 + 2* fheight- 2, width - 30, channels[index]->description, COL_MENUCONTENTDARK, 0, true); // UTF-8
}

void CWebTV::clearItem2DetailsLine()
{  
	  paintItem2DetailsLine(-1, 0);  
}

void CWebTV::paintItem2DetailsLine(int pos, int ch_index)
{
#define ConnectLineBox_Width	16

	int xpos  = x - ConnectLineBox_Width;
	int ypos1 = y + theight + pos*fheight;
	int ypos2 = y + height;
	int ypos1a = ypos1 + (fheight/2) - 2;
	int ypos2a = ypos2 + (info_height/2) - 2;
	fb_pixel_t col1 = COL_MENUCONTENT_PLUS_6;
	fb_pixel_t col2 = COL_MENUCONTENT_PLUS_1;

	// Clear
	frameBuffer->paintBackgroundBoxRel(xpos, y, ConnectLineBox_Width, height + info_height);

#if !defined USE_OPENGL
	frameBuffer->blit();
#endif

	// paint Line if detail info (and not valid list pos)
	if (pos >= 0) 
	{ 
		int fh = fheight > 10 ? fheight - 10 : 5;
			
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 4, ypos1 + 5, 4, fh, col1);
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 4, ypos1 + 5, 1, fh, col2);			

		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width - 4, ypos2 + 7, 4, info_height - 14, col1);
		frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width - 4, ypos2 + 7, 1, info_height - 14, col2);			

		// vertical line
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 16, ypos1a, 4, ypos2a - ypos1a, col1);
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 16, ypos1a, 1, ypos2a - ypos1a + 4, col2);		

		// Hline Oben
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 15, ypos1a, 12,4, col1);
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 16, ypos1a, 12,1, col2);
		
		// Hline Unten
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 15, ypos2a, 12, 4, col1);
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 12, ypos2a, 8, 1, col2);

		// untere info box lines
		frameBuffer->paintBoxRel(x, ypos2, width, info_height, col1);
	}
}

// paint
void CWebTV::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	
	int lastnum =  liststart + listmaxshow;
	
	if(lastnum<10)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("0");
	else if(lastnum<100)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("00");
	else if(lastnum<1000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("000");
	else if(lastnum<10000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("0000");
	else // if(lastnum<100000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("00000");
	
	// channelslist body
	frameBuffer->paintBoxRel(x, y + theight, width, height - buttonHeight - theight, COL_MENUCONTENT_PLUS_0);
	
	// paint pig
	if(g_settings.mini_tv)
	{
		frameBuffer->paintBackgroundBoxRel(835, y + theight + 5, 390, 225);	
		videoDecoder->Pig(835, y + theight + 5, 390, 225);
	}
	//
	
	// paint item
	for(unsigned int count = 0; count < listmaxshow; count++) 
	{
		paintItem(count);
	}

	// sb
	int ypos = y + theight;
	int sb = fheight*listmaxshow;
	
	frameBuffer->paintBoxRel(x + width - 15, ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

	int sbc = ((channels.size()- 1)/ listmaxshow)+ 1;
	int sbs = (selected/listmaxshow);

	frameBuffer->paintBoxRel(x + width- 13, ypos + 2 + sbs*(sb - 4)/sbc, 11, (sb - 4)/sbc, COL_MENUCONTENT_PLUS_3);
}

void CWebTV::paintMiniTV()
{
	// head for minitv
	frameBuffer->paintBoxRel(830, y, 400, theight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);//round
	
	// pig body
	frameBuffer->paintBoxRel(830, y + theight, 400, 225 + theight/2, COL_MENUCONTENT_PLUS_0, RADIUS_MID, CORNER_BOTTOM);
	
	//info head
	frameBuffer->paintBoxRel(830, y + theight + 255 + theight/2 + 5, 400, theight, /*COL_MENUHEAD_PLUS_0*/ COL_MENUCONTENTDARK_PLUS_0);
	
	// info body
	frameBuffer->paintBoxRel(830, y + theight + 255 + theight/2 + 5 + theight, 400, 660 - (y + theight + 255 + theight/2 + 5 + theight) - theight + info_height, COL_MENUCONTENT_PLUS_0);
	
	int channelname_len = 0;
	if (channels.size() && CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_ts)
	{
		if(strlen(channels[selected]->title) != 0)
			channelname_len = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(channels[selected]->title, true); // UTF-8
		
		// title (channelname)
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(830 + 5, y + theight, 400 - 5 - channelname_len, channels[selected]->title, COL_MENUHEAD, 0, true); // UTF-8
	
		// name/description (minitv)
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(830 + 5, y + theight + 255 + theight/2 + 5 + theight, 390 - 30, channels[selected]->title, COL_MENUCONTENTDARK, 0, true);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString (830 + 5, y + theight + 255 + theight/2 + 5 + theight + fheight, 390 - 30, channels[selected]->description, COL_MENUCONTENTDARK, 0, true); // UTF-8
	}
	else
	{
		if(live_channel)
		{
			std::string liveChanName = g_Zapit->getChannelName(live_channel_id);
			if(!liveChanName.empty())
				channelname_len = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(liveChanName.c_str(), true); // UTF-8
				
			// live channel name
			g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(830 + 10, y + theight, (channelname_len > 390? 390 : channelname_len), liveChanName.c_str(), COL_MENUHEAD, 0, true); // UTF-8
			
			// FIXME: paint logo ??? channelname else channel logo
			bool logo_ok = false;
			if( 390 - channelname_len >= PIC_W)
				logo_ok = g_PicViewer->DisplayLogo(live_channel_id, 830 + 5 + channelname_len + 5, y, PIC_W, theight);
			
			//CChannelEvent * p_event = NULL;
			//p_event = &live_channel->currentEvent;
			
			//if (!p_event->description.empty()) 
			if (!live_channel->currentEvent.description.empty()) 
			{
				char cNoch[50]; // UTF-8
				char cSeit[50]; // UTF-8

				//struct tm * pStartZeit = localtime(&p_event->startTime);
				struct tm * pStartZeit = localtime(&live_channel->currentEvent.startTime);
				//unsigned seit = ( time(NULL) - p_event->startTime ) / 60;
				unsigned seit = ( time(NULL) - live_channel->currentEvent.startTime ) / 60;

				sprintf(cSeit, g_Locale->getText(LOCALE_CHANNELLIST_SINCE), pStartZeit->tm_hour, pStartZeit->tm_min);
				//int noch = (p_event->startTime + p_event->duration - time(NULL)) / 60;
				int noch = (live_channel->currentEvent.startTime + live_channel->currentEvent.duration - time(NULL)) / 60;
				if ((noch < 0) || (noch >= 10000))
					noch = 0;
				sprintf(cNoch, "(%d / %d min)", seit, noch);
				
				int seit_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getRenderWidth(cSeit, true); // UTF-8
				int noch_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(cNoch, true); // UTF-8

				//std::string text1 = p_event->description;
				//std::string text2 = p_event->text;
				std::string text1 = live_channel->currentEvent.description;
				std::string text2 = live_channel->currentEvent.text;

				// description
				int descr_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth((char *)text1.c_str(), true); // UTF-8

				// description
				std::string text3 = "";
				
				if (g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(text1, true) > 390)
				{
					// zu breit, Umbruch versuchen...
					int pos;
					do 
					{
						pos = text1.find_last_of("[ -.]+");
					
						if ( pos != -1 )
							text1 = text1.substr( 0, pos );
					} while ( ( pos != -1 ) && (g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(text1, true) > 390) );

					//text3 = p_event->description.substr(text1.length() + 1);
					text3 = live_channel->currentEvent.description.substr(text1.length() + 1);

					g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(830 + 5, y + theight + 255 + theight/2 + 5 +theight + /*2**/fheight, 390, text3, COL_MENUCONTENTDARK, 0, true);
				}
				
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(830 + 5, y + theight + 255 + theight/2 + 5 + theight, (descr_len > 390? 390 : descr_len), text1, COL_MENUCONTENTDARK, 0, true);
					
				// since
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString (830 + 5, y + theight + 255 + theight/2 + 5 + theight + (text3.empty()? fheight : 2*fheight)/*fheight*/, seit_len, cSeit, COL_MENUCONTENTDARK, 0, true); // UTF-8
					
				// rest/duration
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(830 + 390 - 5 - noch_len, y + theight + 255 + theight/2 + 5 + theight + (text3.empty()? fheight : 2*fheight)/*fheight*/, noch_len, cNoch, COL_MENUCONTENTDARK, 0, true); // UTF-8
				
				// text
				epgText.clear();
				emptyLineCount = 0;
						
				if (!(text2.empty())) 
				{
					processTextToArray(text2);
							
					// recalculate
					medlineheight = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight();
					medlinecount = (660 - (y + theight + 255 + theight/2 + 5 + theight) - theight + info_height -fheight)/medlineheight;

					int textCount = epgText.size();
					int ypos = y + theight + 255 + theight/2 + 5 +theight + (text3.empty()? 2*fheight : 3*fheight);

					for(int i = 0; i < textCount && i < medlinecount; i++, ypos += medlineheight)
					{
						if ( i < epgText.size() )
							g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(835, ypos + medlineheight, 390, epgText[i], COL_MENUCONTENTDARK, 0, true); // UTF-8
						else
							g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->RenderString(835, ypos + medlineheight, 390, epgText[i], COL_MENUCONTENTDARK, 0, true); // UTF-8
					}
				}
			}
		}
	}
	//
}

void CWebTV::addTextToArray(const std::string & text) // UTF-8
{
	//printf("line: >%s<\n", text.c_str() );
	
	if (text==" ")
	{
		emptyLineCount ++;
		if(emptyLineCount<2)
		{
			epgText.push_back(text);
		}
	}
	else
	{
		emptyLineCount = 0;
		epgText.push_back(text);
	}
}

void CWebTV::processTextToArray(std::string text) // UTF-8
{
	std::string	aktLine = "";
	std::string	aktWord = "";
	int	aktWidth = 0;
	text += ' ';
	char* text_= (char*) text.c_str();

	while(*text_!=0)
	{
		if ( (*text_==' ') || (*text_=='\n') || (*text_=='-') || (*text_=='.') )
		{
			// Houdini: if there is a newline (especially in the Premiere Portal EPGs) do not forget to add aktWord to aktLine 
			// after width check, if width check failes do newline, add aktWord to next line 
			// and "reinsert" i.e. reloop for the \n
			if(*text_!='\n')
				aktWord += *text_;

			// check the wordwidth - add to this line if size ok
			int aktWordWidth = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getRenderWidth(aktWord, true);
			if((aktWordWidth+aktWidth) < (390))
			{
				//space ok, add
				aktWidth += aktWordWidth;
				aktLine += aktWord;
			
				if(*text_=='\n')
				{	//enter-handler
					addTextToArray( aktLine );
					aktLine = "";
					aktWidth= 0;
				}	
				aktWord = "";
			}
			else
			{
				//new line needed
				addTextToArray( aktLine );
				aktLine = aktWord;
				aktWidth = aktWordWidth;
				aktWord = "";
				// Houdini: in this case where we skipped \n and space is too low, exec newline and rescan \n 
				// otherwise next word comes direct after aktLine
				if(*text_=='\n')
					continue;
			}
		}
		else
		{
			aktWord += *text_;
		}
		text_++;
	}
	//add the rest
	addTextToArray( aktLine + aktWord );
}

