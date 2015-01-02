/*
	$Id: webtv.cpp 2013/09/03 10:45:30 mohousch Exp $
	based on martii webtv

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

#include <stdio.h>

#include <algorithm>    // std::sort

#include <global.h>
#include <neutrino.h>
#include <driver/screen_max.h>
#include "movieplayer.h"
#include "webtv.h"
#include <gui/widget/buttons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/msgbox.h>

#include <gui/filebrowser.h>

#include <xmlinterface.h>

#include <sectionsd/edvbstring.h>
#include <client/zapittools.h>

#include <system/debug.h>
#include <system/helpers.h>

/* libdvbapi */
#include <playback_cs.h>
#include <video_cs.h>
#include <audio_cs.h>


extern cPlayback *playback;

#define DEFAULT_WEBTV_XMLFILE 		CONFIGDIR "/webtv.xml"

extern cVideo * videoDecoder;
extern CPictureViewer * g_PicViewer;

//
unsigned short w_apids[10];
unsigned short w_ac3flags[10];
unsigned short w_numpida = 0;
unsigned short w_vpid = 0;
unsigned short w_vtype = 0;
std::string    w_language[10];

unsigned int w_currentapid = 0, w_currentac3 = 0, w_apidchanged = 0;

unsigned int w_ac3state = CInfoViewer::NO_AC3;

extern CVideoSetupNotifier * videoSetupNotifier;	/* defined neutrino.cpp */
// aspect ratio
#if defined (__sh__)
#define VIDEOMENU_VIDEORATIO_OPTION_COUNT 2
const CMenuOptionChooser::keyval VIDEOMENU_VIDEORATIO_OPTIONS[VIDEOMENU_VIDEORATIO_OPTION_COUNT] =
{
	{ ASPECTRATIO_43, LOCALE_VIDEOMENU_VIDEORATIO_43, NULL },
	{ ASPECTRATIO_169, LOCALE_VIDEOMENU_VIDEORATIO_169, NULL }
};
#else
#define VIDEOMENU_VIDEORATIO_OPTION_COUNT 3
const CMenuOptionChooser::keyval VIDEOMENU_VIDEORATIO_OPTIONS[VIDEOMENU_VIDEORATIO_OPTION_COUNT] =
{
	{ ASPECTRATIO_43, LOCALE_VIDEOMENU_VIDEORATIO_43, NULL },
	{ ASPECTRATIO_169, LOCALE_VIDEOMENU_VIDEORATIO_169, NULL },
	{ ASPECTRATIO_AUTO, NONEXISTANT_LOCALE, "Auto" }
};
#endif

// policy
#if defined (__sh__)
/*
letterbox 
panscan 
non 
bestfit
*/
#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const CMenuOptionChooser::keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] = 
{
	{ VIDEOFORMAT_LETTERBOX, LOCALE_VIDEOMENU_LETTERBOX, NULL },
	{ VIDEOFORMAT_PANSCAN, LOCALE_VIDEOMENU_PANSCAN, NULL },
	{ VIDEOFORMAT_FULLSCREEN, LOCALE_VIDEOMENU_FULLSCREEN, NULL },
	{ VIDEOFORMAT_PANSCAN2, LOCALE_VIDEOMENU_PANSCAN2, NULL }
};
#else
// giga/generic
/*
letterbox 
panscan 
bestfit 
nonlinear
*/
#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const CMenuOptionChooser::keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] = 
{
	{ VIDEOFORMAT_LETTERBOX, LOCALE_VIDEOMENU_LETTERBOX, NULL },
	{ VIDEOFORMAT_PANSCAN, LOCALE_VIDEOMENU_PANSCAN, NULL },
	{ VIDEOFORMAT_PANSCAN2, LOCALE_VIDEOMENU_PANSCAN2, NULL },
	{ VIDEOFORMAT_FULLSCREEN, LOCALE_VIDEOMENU_FULLSCREEN, NULL }
};
#endif

// ac3
extern CAudioSetupNotifier * audioSetupNotifier;	/* defined neutrino.cpp */

#if !defined (PLATFORM_COOLSTREAM)
#define AC3_OPTION_COUNT 2
const CMenuOptionChooser::keyval AC3_OPTIONS[AC3_OPTION_COUNT] =
{
	{ AC3_PASSTHROUGH, NONEXISTANT_LOCALE, "passthrough" },
	{ AC3_DOWNMIX, NONEXISTANT_LOCALE, "downmix" }
};
#endif

int CWebTVAPIDSelectExec::exec(CMenuTarget */*parent*/, const std::string & actionKey)
{
	w_apidchanged = 0;
	unsigned int sel = atoi(actionKey.c_str());

	if (w_currentapid != w_apids[sel - 1]) 
	{
		w_currentapid = w_apids[sel - 1];
		w_currentac3 = w_ac3flags[sel - 1];
		w_apidchanged = 1;
		
		dprintf(DEBUG_NORMAL, "[movieplayer] apid changed to %d\n", w_apids[sel - 1]);
	}

	return menu_return::RETURN_EXIT;
}

CWebTV::CWebTV()
{
	frameBuffer = CFrameBuffer::getInstance();
	
	selected = 0;
	liststart = 0;
	tuned = -1;
	count = g_settings.user_bouquet_count;
	n_count = 1;
	
	parser = NULL;
	mode = WEBTV;
	
	position = 0;
	duration = 0;
	file_prozent = 0;
	
	zapProtection = NULL;
	
	playstate = STOPPED;
	speed = 0;
}

CWebTV::~CWebTV()
{
	ClearChannels();
	g_settings.user_bouquet_count = count;
}

void CWebTV::ClearChannels(void)
{
	if (parser)
	{
		xmlFreeDoc(parser);
		parser = NULL;
	}
	
	for(unsigned int j = 0; j < channels.size(); j++)
	{
		delete channels[j];
	}
	channels.clear();
}

int CWebTV::exec(bool rezap)
{
	// load streams channels list
	loadChannels();
	
	int nNewChannel = Show();
	
	// zapto
	if ( nNewChannel > -1 && nNewChannel < (int) channels.size()) 
		zapTo(nNewChannel, rezap);

	return nNewChannel;
}

void CWebTV::loadChannels(void)
{
	// load streams channels list
	switch(mode)
	{
		case WEBTV:
		{
			readChannellist(DEFAULT_WEBTV_XMLFILE);
			title = g_Locale->getText(LOCALE_WEBTV_HEAD);
		}
		break;
			
		case USER:
		{
			readChannellist(g_settings.webtv_user_bouquet[n_count]);
			title = std::string(rindex(g_settings.webtv_user_bouquet[n_count], '/') + 1);
			strReplace(title, ".xml", "");
			strReplace(title, ".tv", "");
			strReplace(title, "userbouquet.", "");
		}
		break;
			
		default: break;	
	}
}

// readxml file
bool CWebTV::readChannellist(std::string filename)
{
	dprintf(DEBUG_NORMAL, "CWebTV::readChannellist parsing %s\n", filename.c_str());
	
	// clear channellist
	ClearChannels();
	
	// check for extension
	int ext_pos = 0;
	ext_pos = filename.rfind('.');
	bool iptv = false;
					
	if( ext_pos > 0)
	{
		std::string extension;
		extension = filename.substr(ext_pos + 1, filename.length() - ext_pos);
						
		if( strcasecmp("tv", extension.c_str()) == 0)
			iptv = true;
	}
	
	if(iptv)
	{
		FILE * f = fopen(filename.c_str(), "r");
		std::string title;
		std::string URL;
		std::string url;
		std::string description;
		
		if(f != NULL)
		{
			while(1)
			{
				char line[1024];
				if (!fgets(line, 1024, f))
					break;
				
				size_t len = strlen(line);
				if (len < 2)
					// Lines with less than one char aren't meaningful
					continue;
				
				/* strip newline */
				line[--len] = 0;
				
				// strip carriage return (when found)
				if (line[len - 1] == '\r')
					line[len - 1 ] = 0;
				
				if (strncmp(line, "#SERVICE 4097:0:1:0:0:0:0:0:0:0:", 32) == 0)
					url = line + 32;
				//else if ( (strncmp(line, "#DESCRIPTION: ", 14) == 0) || (strncmp(line, "#DESCRIPTION ", 13) == 0) )
				else if (strncmp(line, "#DESCRIPTION", 12) == 0)
				{
					int offs = line[12] == ':' ? 14 : 13;
			
					title = line + offs;
				
					description = "stream";
					
					webtv_channels * tmp = new webtv_channels();
						
					tmp->title = title.c_str();
					tmp->url = urlDecode(url).c_str();
					tmp->description = description.c_str();
					tmp->locked = false;
						
					// fill channelslist
					channels.push_back(tmp);
				}
			}
			
			fclose(f);
			
			return true;
		}
	}
	else
	{
		parser = parseXmlFile(filename.c_str());
		
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
					bool locked = xmlGetAttribute(l1, (char *)"locked");
					
					// fill webtv list
					webtv_channels * tmp = new webtv_channels();
					
					tmp->title = title;
					tmp->url = url;
					tmp->description = description;
					tmp->locked = locked;
					
					// fill channelslist
					channels.push_back(tmp);

					l1 = l1->xmlNextNode;
				}
			}
			
			return true;
		}
		
		xmlFreeDoc(parser);
	}
	
	return false;
}

void CWebTV::showUserBouquet(void)
{
	static int old_select = 0;
	char cnt[5];
	CMenuWidget InputSelector(LOCALE_WEBTV_HEAD, NEUTRINO_ICON_WEBTV_SMALL);
	//int count = 0;
	int select = -1;
					
	CMenuSelectorTarget *WebTVInputChanger = new CMenuSelectorTarget(&select);
			
	// webtv
	sprintf(cnt, "%d", 0);
	InputSelector.addItem(new CMenuForwarder(LOCALE_WEBTV_HEAD, true, NULL, WebTVInputChanger, cnt, NULL), old_select == 0);
	
	// divers
	for (int i = 0; i < count; i++)
	{
		sprintf(cnt, "%d", i + 1);
		userBouquet = std::string(rindex(g_settings.webtv_user_bouquet[i], '/') + 1);
		strReplace(userBouquet, ".xml", "");
		strReplace(userBouquet, ".tv", "");
		strReplace(userBouquet, "userbouquet.", "");
		InputSelector.addItem(new CMenuForwarderNonLocalized(userBouquet.c_str(), true, NULL, WebTVInputChanger, cnt, NULL), old_select == i + 1);
	}
	
	hide();
	InputSelector.exec(NULL, "");
	delete WebTVInputChanger;
	
	printf("webtv: select:%d\n", select);
					
	if(select >= 0)
	{
		old_select = select;
		
		if(select == WEBTV)
		{
			mode = WEBTV;
			readChannellist(DEFAULT_WEBTV_XMLFILE);
			selected = 0;
			title = g_Locale->getText(LOCALE_WEBTV_HEAD);
		}
		else
		{
			mode = USER;
			readChannellist(g_settings.webtv_user_bouquet[select - 1]);
			selected = 0;
			n_count = select - 1;
			title = std::string(rindex(g_settings.webtv_user_bouquet[select - 1], '/') + 1);
			strReplace(title, ".xml", "");
			strReplace(title, ".tv", "");
			strReplace(title, "userbouquet.", "");
		}
	}
}

void CWebTV::showAudioDialog(void)
{
	CMenuWidget APIDSelector(LOCALE_APIDSELECTOR_HEAD, NEUTRINO_ICON_AUDIO);

	// g_apids will be rewritten for mb
	playback->FindAllPids(w_apids, w_ac3flags, &w_numpida, w_language);
			
	if (w_numpida > 0) 
	{
		CWebTVAPIDSelectExec * APIDChanger = new CWebTVAPIDSelectExec;
		bool enabled;
		bool defpid;

		for (unsigned int count = 0; count < w_numpida; count++) 
		{
			bool name_ok = false;
			char apidnumber[10];
			sprintf(apidnumber, "%d %X", count + 1, w_apids[count]);
			enabled = true;
			defpid = w_currentapid ? (w_currentapid == w_apids[count]) : (count == 0);
			std::string apidtitle = "Stream ";

			// language
			if (!w_language[count].empty())
			{
				apidtitle = w_language[count];
				name_ok = true;
			}

			if (!name_ok)
			{
				apidtitle = "Stream ";
				name_ok = true;
			}

			switch(w_ac3flags[count])
			{
				case 1: /*AC3,EAC3*/
					if (apidtitle.find("AC3") <= 0)
					{
						apidtitle.append(" (AC3)");
								
						// ac3 state
						w_ac3state = CInfoViewer::AC3_AVAILABLE;
					}
					break;

				case 2: /*teletext*/
					apidtitle.append(" (Teletext)");
					enabled = false;
					break;

				case 3: /*MP2*/
					apidtitle.append(" (MP2)");
					break;

				case 4: /*MP3*/
					apidtitle.append(" (MP3)");
					break;

				case 5: /*AAC*/
					apidtitle.append(" (AAC)");
					break;

				case 6: /*DTS*/
					apidtitle.append(" (DTS)");
					break;

				case 7: /*MLP*/
					apidtitle.append(" (MLP)");
					break;

				default:
					break;
			}

			if (!name_ok)
				apidtitle.append(apidnumber);

			APIDSelector.addItem(new CMenuForwarderNonLocalized( apidtitle.c_str(), enabled, NULL, APIDChanger, apidnumber, CRCInput::convertDigitToKey(count + 1)), defpid);
		}
				
				// ac3
#if !defined (PLATFORM_COOLSTREAM)				
		APIDSelector.addItem(GenericMenuSeparatorLine);
		APIDSelector.addItem(new CMenuOptionChooser(LOCALE_AUDIOMENU_HDMI_DD, &g_settings.hdmi_dd, AC3_OPTIONS, AC3_OPTION_COUNT, true, audioSetupNotifier, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED ));
#endif				
				
		// policy/aspect ratio
		APIDSelector.addItem(GenericMenuSeparatorLine);
				
		// video aspect ratio 4:3/16:9
		APIDSelector.addItem(new CMenuOptionChooser(LOCALE_VIDEOMENU_VIDEORATIO, &g_settings.video_Ratio, VIDEOMENU_VIDEORATIO_OPTIONS, VIDEOMENU_VIDEORATIO_OPTION_COUNT, true, videoSetupNotifier, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN, true ));
	
		// video format bestfit/letterbox/panscan/non
		APIDSelector.addItem(new CMenuOptionChooser(LOCALE_VIDEOMENU_VIDEOFORMAT, &g_settings.video_Format, VIDEOMENU_VIDEOFORMAT_OPTIONS, VIDEOMENU_VIDEOFORMAT_OPTION_COUNT, true, videoSetupNotifier, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW, true ));

		w_apidchanged = 0;
		APIDSelector.exec(NULL, "");

		if (w_apidchanged) 
		{
			if (w_currentapid == 0) 
			{
				w_currentapid = w_apids[0];
				w_currentac3 = w_ac3flags[0];

				if(w_currentac3)
					w_ac3state = CInfoViewer::AC3_ACTIVE;
			}

#if defined (PLATFORM_COOLSTREAM)
			playback->SetAPid(w_currentapid, w_currentac3);
#else					
			playback->SetAPid(w_currentapid);
#endif					
			w_apidchanged = 0;
		}
				
		delete APIDChanger;
				
		//CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);

	} 
	else 
	{
		DisplayErrorMessage(g_Locale->getText(LOCALE_AUDIOSELECTMENUE_NO_TRACKS)); // UTF-8
	}
}

bool CWebTV::startPlayBack(int pos)
{
	playback->Open();
	
	// if not mached
	if ( (pos >= (signed int) channels.size()) || (pos < 0) ) 
	{
		pos = 0;
	}
	
	if (!playback->Start((char *)channels[pos]->url.c_str()))
		return false;
	
	playstate = PLAY;
	speed = 1;
	return true;
}

void CWebTV::stopPlayBack(void)
{
	playback->Close();
	playstate = STOPPED;
}

void CWebTV::pausePlayBack(void)
{
	playback->SetSpeed(0);
	playstate = PAUSE;
	speed = 0;
}

void CWebTV::continuePlayBack(void)
{
	playback->SetSpeed(1);
	playstate = PLAY;
	speed = 1;
}

//
void CWebTV::zapTo(int pos, bool rezap)
{
	// show emty channellist error msg
	if (channels.empty()) 
	{
		DisplayErrorMessage(g_Locale->getText(LOCALE_WEBTVCHANNELLIST_NONEFOUND)); // UTF-8
		return;
	}

	// if not mached
	if ( (pos >= (signed int) channels.size()) || (pos < 0) ) 
	{
		pos = 0;
	}
	
	// check if the same channel
	if ( pos != tuned || rezap) 
	{
		tuned = pos;
		
		// 
		if(playback->playing)
			playback->Stop();
	
		// parentallock
		if ( (channels[pos]->locked) && ( (g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_ONSIGNAL) || (g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_CHANGETOLOCKED)) )
		{
			if ( zapProtection != NULL )
				zapProtection->fsk = g_settings.parentallock_lockage;
			else
			{
				zapProtection = new CZapProtection( g_settings.parentallock_pincode, g_settings.parentallock_lockage);
							
				if ( !zapProtection->check() )
				{
					delete zapProtection;
					zapProtection = NULL;
					
					// do not thing
				}
				else
				{
					delete zapProtection;
					zapProtection = NULL;
					
					// start playback
					startPlayBack(pos);
				}
			}
		}
		else
			startPlayBack(pos);
	}
	
	// vfd
	if (CVFD::getInstance()->is4digits)
		CVFD::getInstance()->LCDshowText(pos + 1);
	else
		CVFD::getInstance()->showServicename(channels[pos]->title); // UTF-8		
	
	//infoviewer
	g_InfoViewer->showMovieInfo(channels[pos]->title, channels[pos]->description, file_prozent, duration, w_ac3state, speed, playstate, false, false);
}

void CWebTV::quickZap(int key)
{
	if (key == g_settings.key_quickzap_down)
	{
                if(selected == 0)
                        selected = channels.size() - 1;
                else
                        selected--;
        }
	else if (key == g_settings.key_quickzap_up)
	{
                selected = (selected+1)%channels.size();
        }
	
	zapTo(selected);
}

void CWebTV::showInfo()
{
	//infoviewer
	if(tuned > -1)
		g_InfoViewer->showMovieInfo(channels[tuned]->title, channels[tuned]->description, file_prozent, duration, w_ac3state, speed, playstate, false, false);
}

void CWebTV::getInfos()
{
	playback->GetPosition((int64_t &)position, (int64_t &)duration);
	
	if(duration > 100)
		file_prozent = (unsigned char) (position / (duration / 100));
}

int CWebTV::Show()
{
	int res = -1;
	
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	
	if(channels.empty())
		loadChannels();
	
	// display channame in vfd	
	CVFD::getInstance()->setMode(CVFD::MODE_IPTV);
	
	// windows size
	width  = w_max ( (frameBuffer->getScreenWidth() / 20 * 17), (frameBuffer->getScreenWidth() / 20 ));
	height = h_max ( (frameBuffer->getScreenHeight() / 20 * 16), (frameBuffer->getScreenHeight() / 20));
	
	// head height
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_HELP, &icon_hd_w, &icon_hd_h);
	theight = std::max(icon_hd_h, g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight()) + 6;
	
	// buttonheight
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_bf_w, &icon_bf_h);
	buttonHeight = std::max(icon_bf_h, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight()) + 6;

	// listbox/items
	iheight = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight() + 2;
	listmaxshow = (height - theight - buttonHeight)/iheight;
	height = theight + buttonHeight + listmaxshow * iheight;
	
	// info height
	info_height = 5 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight() + 5 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getHeight() + 5;
	
	// x/y
	x = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - width) / 2;
	y = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - (height + info_height)) / 2;
	
showList:	
	
	// head
	paintHead();
	
	// foot
	paintFoot();
		
	// paint all
	paint();
		
	frameBuffer->blit();

	oldselected = selected;
	int zapOnExit = false;

	// loop control
	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);
	bool loop = true;
	
	while (loop) 
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
		
		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

		if ( ( msg == CRCInput::RC_timeout ) || ( msg == (neutrino_msg_t)g_settings.key_channelList_cancel) ) 
		{
			selected = oldselected;
			
			loop = false;
			res = -1;
		}
		else if ( msg == CRCInput::RC_up || (int) msg == g_settings.key_channelList_pageup || msg == CRCInput::RC_yellow)
                {
                        int step = 0;
                        int prev_selected = selected;

			step =  ((int) msg == g_settings.key_channelList_pageup || (int) msg == CRCInput::RC_yellow) ? listmaxshow : 1;  // browse or step 1
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
                else if ( msg == CRCInput::RC_down || (int) msg == g_settings.key_channelList_pagedown || msg == CRCInput::RC_green)
                {
                        unsigned int step = 0;
                        int prev_selected = selected;

			step =  ((int) msg == g_settings.key_channelList_pagedown || (int)msg == CRCInput::RC_green) ? listmaxshow : 1;  // browse or step 1
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
                else if ( msg == CRCInput::RC_ok || msg == (neutrino_msg_t) g_settings.mpkey_play) 
		{
			zapOnExit = true;
			loop = false;
		}
		else if (msg == CRCInput::RC_info) 
		{
			showFileInfoWebTV(selected);
			res = -1;
			
			goto showList;
		}
		else if (msg == CRCInput::RC_red) 
		{
			openFilebrowser();
			g_settings.user_bouquet_count = count;
			res = -1;
			
			goto showList;
		}
		else if(msg == CRCInput::RC_blue || msg == CRCInput::RC_favorites)
		{
			showUserBouquet();
			res = -1;
			
			goto showList;
		}
		else if( msg == (neutrino_msg_t) g_settings.key_timeshift) // pause playing
		{
			if(playstate == PAUSE)
				continuePlayBack();
			else if(playstate == PLAY)
				pausePlayBack();
			
			res = -1;
			loop = false;
		}
		else if( msg == CRCInput::RC_stop) // pause playing
		{
			if(playstate == PLAY || playstate == PAUSE)
				stopPlayBack();
			
			res = -1;
			loop = false;
		}
		else if(msg == (neutrino_msg_t)g_settings.mpkey_play)
		{
			if(playstate == PAUSE)
				continuePlayBack();
			
			res = -1;
			loop = false;
		}
		else
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
			{
				loop = false;
				res = - 1;
			}
		}
			
		frameBuffer->blit();	
	}
	
	hide();
	
	//CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
	
	if(zapOnExit)
		res = selected;

	printf("CWebTV::show res %d\n", res);
			
	return (res);
}

void CWebTV::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width + 5, height + info_height + 5);
			
        clearItem2DetailsLine();
	frameBuffer->blit();
}

void CWebTV::paintItem(int pos)
{
	int ypos = y + theight + pos*iheight;
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
		frameBuffer->paintBoxRel(x, ypos, width - SCROLLBAR_WIDTH, iheight, bgcolor);
	} 
	else 
	{
		color = COL_MENUCONTENT;
		bgcolor = COL_MENUCONTENT_PLUS_0;
		
		// itembox
		frameBuffer->paintBoxRel(x, ypos, width - SCROLLBAR_WIDTH, iheight, bgcolor);
	}

	//name and description
	if(curr < channels.size()) 
	{
		char tmp[10];
		char nameAndDescription[255];
		int l = 0;
		
		sprintf((char*) tmp, "%d", curr + 1);
		l = snprintf(nameAndDescription, sizeof(nameAndDescription), "%s", channels[curr]->title.c_str());
		
		// number
		int numpos = x + 10 + numwidth - g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(tmp);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(numpos, ypos + (iheight - g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight(), numwidth + 5, tmp, color, 0, true);
		
		// description
		std::string Descr = channels[curr]->description.c_str();
		if(!(Descr.empty()))
		{
			snprintf(nameAndDescription + l, sizeof(nameAndDescription) -l, "  -  ");
			
			unsigned int ch_name_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(nameAndDescription, true);
			unsigned int ch_desc_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getRenderWidth(channels[curr]->description, true);
			
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + 10 + numwidth + 10, ypos + iheight, width - numwidth - 20 - 15, nameAndDescription, color, 0, true);
			
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString(x + 5 + numwidth + 10 + ch_name_len, ypos + iheight, ch_desc_len, channels[curr]->description, (curr == selected)?COL_MENUCONTENTSELECTED : COL_COLORED_EVENTS_CHANNELLIST, 0, true);
		}
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + 10 + numwidth + 10, ypos + iheight, width - numwidth - 20 - 15, nameAndDescription, color, 0, true);
	}
}

#define NUM_LIST_BUTTONS 4
struct button_label CWebTVButtons[NUM_LIST_BUTTONS] =
{
	{ NEUTRINO_ICON_BUTTON_RED, LOCALE_WEBTV_ADD_BOUQUETS},
	{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_FILEBROWSER_NEXTPAGE},
	{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_FILEBROWSER_PREVPAGE},
	{ NEUTRINO_ICON_BUTTON_BLUE, LOCALE_WEBTV_BOUQUETS}
};

// paint head
void CWebTV::paintHead()
{
	// head
	frameBuffer->paintBoxRel(x, y, width, theight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP); //round
	
	// head icon
	int icon_webtv_w, icon_webtv_h;
	frameBuffer->getIconSize(NEUTRINO_ICON_WEBTV_SMALL, &icon_webtv_w, &icon_webtv_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_WEBTV_SMALL, x + BORDER_LEFT, y + ( theight - icon_webtv_h)/2 );
	
	// help icon
	int icon_help_w, icon_help_h;
	
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_HELP, &icon_help_w, &icon_help_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x + width - (BORDER_RIGHT + icon_help_w) , y + (theight - icon_help_h)/2 );
	
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
		
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->RenderString(x + width - BORDER_LEFT - BORDER_RIGHT - icon_help_w - timestr_len, y + (theight - g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getHeight(), timestr_len, timestr, COL_MENUHEAD, 0, true); //utf-8
	}
	
	//head title
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + BORDER_LEFT + icon_webtv_w + 5, y + (theight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight(), width - 20 - icon_webtv_w - timestr_len, title.c_str(), COL_MENUHEAD, 0, true); // UTF-8
}

void CWebTV::paintFoot()
{
	// foot
	int ButtonWidth = (width - 20) / 4;
	int f_x = x;
	int f_y = y + (height - buttonHeight);
	
	frameBuffer->paintBoxRel(f_x, f_y, width, buttonHeight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_BOTTOM); //round
	
	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, f_x + BORDER_LEFT, f_y + (buttonHeight - icon_bf_h)/2, ButtonWidth, NUM_LIST_BUTTONS, CWebTVButtons);
}

// infos
void CWebTV::paintDetails(int index)
{
	// infobox refresh
	frameBuffer->paintBoxRel(x + 2, y + height + 2, width - 4, info_height - 4, COL_MENUCONTENTDARK_PLUS_0);
	
	if(channels.empty() )
		return;
	
	// name/description
	g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + BORDER_LEFT, y + height + 5 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight(), width - 30, channels[index]->title, COL_MENUCONTENTDARK, 0, true);
	g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString (x + BORDER_LEFT, y + height + 5 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight() + 5 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getHeight(), width - 30, channels[index]->description, COL_MENUCONTENTDARK, 0, true); // UTF-8
}

void CWebTV::clearItem2DetailsLine()
{  
	  paintItem2DetailsLine(-1, 0);  
}

void CWebTV::paintItem2DetailsLine(int pos, int /*ch_index*/)
{
#define ConnectLineBox_Width	16

	int xpos  = x - ConnectLineBox_Width;
	int ypos1 = y + theight + pos*iheight;
	int ypos2 = y + height;
	int ypos1a = ypos1 + (iheight/2) - 2;
	int ypos2a = ypos2 + (info_height/2) - 2;
	fb_pixel_t col1 = COL_MENUCONTENT_PLUS_6;
	fb_pixel_t col2 = COL_MENUCONTENT_PLUS_1;

	// Clear
	frameBuffer->paintBackgroundBoxRel(xpos, y, ConnectLineBox_Width, height + info_height);

	// blit
	frameBuffer->blit();

	// paint Line if detail info (and not valid list pos)
	if (pos >= 0) 
	{ 
		int fh = iheight > 10 ? iheight - 10 : 5;
			
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 4, ypos1 + 5, 4, fh, col1);
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 4, ypos1 + 5, 1, fh, col2);			

		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 4, ypos2 + 7, 4, info_height - 14, col1);
		frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 4, ypos2 + 7, 1, info_height - 14, col2);			

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
		frameBuffer->paintBoxRel(x, ypos2, width, info_height, col1, true);
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
	
	// paint item
	for(unsigned int count = 0; count < listmaxshow; count++) 
	{
		paintItem(count);
	}

	// sb
	int ypos = y + theight;
	int sb = iheight*listmaxshow;
	
	frameBuffer->paintBoxRel(x + width - SCROLLBAR_WIDTH, ypos, SCROLLBAR_WIDTH, sb,  COL_MENUCONTENT_PLUS_1);

	int sbc = ((channels.size() - 1)/ listmaxshow) + 1;
	int sbs = (selected/listmaxshow);

	frameBuffer->paintBoxRel(x + width - 13, ypos + 2 + sbs*(sb - 4)/sbc, 11, (sb - 4)/sbc, COL_MENUCONTENT_PLUS_3);
}

void CWebTV::showFileInfoWebTV(int pos)
{
	if(pos > -1)
		ShowMsg2UTF(channels[pos]->title.c_str(), channels[pos]->description.c_str(), CMsgBox::mbrBack, CMsgBox::mbBack);
}

void CWebTV::openFilebrowser(void)
{
	CFileBrowser filebrowser;
	CFileFilter fileFilter;
	
	fileFilter.addFilter("xml");
	fileFilter.addFilter("tv");

	//filebrowser.Multi_Select    = true;
	//filebrowser.Dirs_Selectable = true;
	filebrowser.Filter = &fileFilter;

	if (filebrowser.exec(CONFIGDIR))
	{
		// select file
		strcpy(g_settings.webtv_user_bouquet[count], filebrowser.getSelectedFile()->Name.c_str());
		
		printf("[webtv] webtv settings file %s\n", filebrowser.getSelectedFile()->Name.c_str());
		
		// load channels
		mode = USER;
		
		readChannellist(g_settings.webtv_user_bouquet[count]);
		title = std::string(rindex(g_settings.webtv_user_bouquet[count], '/') + 1);
		strReplace(title, ".xml", "");
		strReplace(title, ".tv", "");
		strReplace(title, "userbouquet.", "");
		selected = 0;
		++count;
	}
}



