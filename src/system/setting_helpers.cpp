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

#include <system/setting_helpers.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <fcntl.h>
#include <signal.h>
#include "libnet.h"

#include <config.h>

#include <global.h>
#include <neutrino.h>
#include <gui/widget/stringinput.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>

#include <gui/plugins.h>
#include <daemonc/remotecontrol.h>
#include <xmlinterface.h>

#include "debug.h"

#include <audio_cs.h>
#include <video_cs.h>
#include <dmx_cs.h>

/*zapit includes*/
#include <frontend_c.h>

/*gui includes*/
#include <gui/scan_setup.h>
#include <gui/filebrowser.h>


extern CPlugins       * g_PluginList;    /* neutrino.cpp */
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */
extern cVideo *videoDecoder;
extern cAudio *audioDecoder;

extern cDemux *videoDemux;
extern cDemux *audioDemux;
extern cDemux *pcrDemux;

// dvbsub
//extern int dvbsub_init(int source);
extern int dvbsub_init();
extern int dvbsub_stop();
extern int dvbsub_close();
extern int dvbsub_start(int pid);
extern int dvbsub_pause();
//extern int dvbsub_getpid();
//extern void dvbsub_setpid(int pid);

// tuxtxt
//extern int  tuxtxt_init();
extern void tuxtxt_start(int tpid, int source);
//extern int  tuxtxt_stop();
//extern void tuxtxt_close();
//extern void tuxtx_pause_subtitle(bool pause, int source);
extern void tuxtx_stop_subtitle();
extern void tuxtx_set_pid(int pid, int page, const char * cc);
//extern int tuxtx_subtitle_running(int *pid, int *page, int *running);
extern int tuxtx_main(int _rc, int pid, int page, int source);

//extern int tuner_to_scan;		//defined in scan_setup.cpp
extern CFrontend * live_fe;
extern CScanSettings * scanSettings;
extern CFrontend * getFE(int index);

extern t_channel_id live_channel_id;

extern "C" int pinghost( const char *hostname );

CSatelliteSetupNotifier::CSatelliteSetupNotifier(int num)
{
	feindex = num;
}

/* items1 enabled for advanced diseqc settings, items2 for diseqc != NO_DISEQC, items3 disabled for NO_DISEQC */
bool CSatelliteSetupNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	std::vector<CMenuItem*>::iterator it;
	int type = *((int*) Data);

	if (type == NO_DISEQC) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			(*it)->setActive(false);
		}
		for(it = items2.begin(); it != items2.end(); it++) 
		{
			(*it)->setActive(false);
		}
		for(it = items3.begin(); it != items3.end(); it++) 
		{
			(*it)->setActive(false);
		}
	}
	else if(type < DISEQC_ADVANCED) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			(*it)->setActive(false);
		}
		for(it = items2.begin(); it != items2.end(); it++) 
		{
			(*it)->setActive(true);
		}
		for(it = items3.begin(); it != items3.end(); it++) 
		{
			(*it)->setActive(true);
		}
	}
	else if(type == DISEQC_ADVANCED) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			(*it)->setActive(true);
		}
		for(it = items2.begin(); it != items2.end(); it++) 
		{
			(*it)->setActive(false);
		}
		for(it = items3.begin(); it != items3.end(); it++) 
		{
			(*it)->setActive(true);
		}
	}

	getFE(feindex)->setDiseqcType( getFE(feindex)->diseqcType );
	getFE(feindex)->setDiseqcRepeats( getFE(feindex)->diseqcRepeats );

	return true;
}

void CSatelliteSetupNotifier::addItem(int list, CMenuItem* item)
{
	switch(list) 
	{
		case 0:
			items1.push_back(item);
			break;
		case 1:
			items2.push_back(item);
			break;
		case 2:
			items3.push_back(item);
			break;
		default:
			break;
	}
}

// dhcp notifier
CDHCPNotifier::CDHCPNotifier( CMenuForwarder* a1, CMenuForwarder* a2, CMenuForwarder* a3, CMenuForwarder* a4, CMenuForwarder* a5)
{
	toDisable[0] = a1;
	toDisable[1] = a2;
	toDisable[2] = a3;
	toDisable[3] = a4;
	toDisable[4] = a5;
}


bool CDHCPNotifier::changeNotify(const neutrino_locale_t, void * data)
{
	CNeutrinoApp::getInstance()->networkConfig.inet_static = ((*(int*)(data)) == 0);
	
	for(int x=0; x < 5; x++)
		toDisable[x]->setActive(CNeutrinoApp::getInstance()->networkConfig.inet_static);
	
	return true;
}

// onoff notifier needed by moviebrowser
COnOffNotifier::COnOffNotifier( CMenuItem* a1,CMenuItem* a2,CMenuItem* a3,CMenuItem* a4,CMenuItem* a5)
{
        number = 0;
        if(a1 != NULL){ toDisable[0] =a1;number++;};
        if(a2 != NULL){ toDisable[1] =a2;number++;};
        if(a3 != NULL){ toDisable[2] =a3;number++;};
        if(a4 != NULL){ toDisable[3] =a4;number++;};
        if(a5 != NULL){ toDisable[4] =a5;number++;};
}

COnOffNotifier::COnOffNotifier(int /*OffValue*/)
{
	number = 0;
}

bool COnOffNotifier::changeNotify(const neutrino_locale_t, void *Data)
{
	if(*(int*)(Data) == 0)
	{
		for (int i=0; i<number ; i++)
			toDisable[i]->setActive(false);
	}
	else
	{
		for (int i=0; i<number ; i++)
			toDisable[i]->setActive(true);
	}
	
	return true;
}

void COnOffNotifier::addItem(CMenuItem* menuItem)
{
	if (number < 15)
	{
		toDisable[number] = menuItem;
		number++;
	}
}

// recording safety notifier
bool CRecordingSafetyNotifier::changeNotify(const neutrino_locale_t, void *)
{
	g_Timerd->setRecordingSafety(atoi(g_settings.record_safety_time_before)*60, atoi(g_settings.record_safety_time_after)*60);

   	return true;
}

// misc notifier
CMiscNotifier::CMiscNotifier( CMenuItem* i1)
{
   	toDisable[0] = i1;
}

bool CMiscNotifier::changeNotify(const neutrino_locale_t, void *)
{
   	toDisable[0]->setActive(!g_settings.shutdown_real);

   	return true;
}

// lcd notifier
bool CLcdNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	int state = *(int *)Data;

	dprintf(DEBUG_NORMAL, "ClcdNotifier: state: %d\n", state);
	
#if defined (PLATFORM_GIGABLUE) && !defined (ENABLE_LCD)
	CVFD::getInstance()->vfd_led(state);
#else	
	CVFD::getInstance()->setPower(state);
	CVFD::getInstance()->setlcdparameter();
#endif	

	return true;
}

bool CPauseSectionsdNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	g_Sectionsd->setPauseScanning((*((int *)Data)) == 0);

	return true;
}

bool CSectionsdConfigNotifier::changeNotify(const neutrino_locale_t, void *)
{
        CNeutrinoApp::getInstance()->SendSectionsdConfig();
	
        return true;
}

// color setup notifier
bool CColorSetupNotifier::changeNotify(const neutrino_locale_t, void *)
{
	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();
	
	//unsigned char r,g,b;
	//setting colors-..
	
	// head
	frameBuffer->paletteGenFade(COL_MENUHEAD,
	                              convertSetupColor2RGB(g_settings.menu_Head_red, g_settings.menu_Head_green, g_settings.menu_Head_blue),
	                              convertSetupColor2RGB(g_settings.menu_Head_Text_red, g_settings.menu_Head_Text_green, g_settings.menu_Head_Text_blue),
	                              8, convertSetupAlpha2Alpha( g_settings.menu_Head_alpha ) );

	// menu content
	frameBuffer->paletteGenFade(COL_MENUCONTENT,
	                              convertSetupColor2RGB(g_settings.menu_Content_red, g_settings.menu_Content_green, g_settings.menu_Content_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );

	// menu content dark
	frameBuffer->paletteGenFade(COL_MENUCONTENTDARK,
	                              convertSetupColor2RGB(int(g_settings.menu_Content_red*0.6), int(g_settings.menu_Content_green*0.6), int(g_settings.menu_Content_blue*0.6)),
	                              convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );

	// menu selected
	frameBuffer->paletteGenFade(COL_MENUCONTENTSELECTED,
	                              convertSetupColor2RGB(g_settings.menu_Content_Selected_red, g_settings.menu_Content_Selected_green, g_settings.menu_Content_Selected_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_Selected_Text_red, g_settings.menu_Content_Selected_Text_green, g_settings.menu_Content_Selected_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_Selected_alpha) );

	// menu inactiv
	frameBuffer->paletteGenFade(COL_MENUCONTENTINACTIVE,
	                              convertSetupColor2RGB(g_settings.menu_Content_inactive_red, g_settings.menu_Content_inactive_green, g_settings.menu_Content_inactive_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_inactive_Text_red, g_settings.menu_Content_inactive_Text_green, g_settings.menu_Content_inactive_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_inactive_alpha) );

	// infobar
	frameBuffer->paletteGenFade(COL_INFOBAR,
	                              convertSetupColor2RGB(g_settings.infobar_red, g_settings.infobar_green, g_settings.infobar_blue),
	                              convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
	// infobar shadow
	frameBuffer->paletteGenFade(COL_INFOBAR_SHADOW,
	                              convertSetupColor2RGB(int(g_settings.infobar_red*0.4), int(g_settings.infobar_green*0.4), int(g_settings.infobar_blue*0.4)),
	                              convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );

	// infobar colored events
	frameBuffer->paletteGenFade(COL_COLORED_EVENTS_INFOBAR,
	                              convertSetupColor2RGB(g_settings.infobar_red, g_settings.infobar_green, g_settings.infobar_blue),
	                              convertSetupColor2RGB(g_settings.infobar_colored_events_red, g_settings.infobar_colored_events_green, g_settings.infobar_colored_events_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
				      
	// foot
	frameBuffer->paletteGenFade(COL_MENUFOOT,
	                              convertSetupColor2RGB(g_settings.menu_Foot_red, g_settings.menu_Foot_green, g_settings.menu_Foot_blue),
	                              convertSetupColor2RGB(g_settings.menu_Foot_Text_red, g_settings.menu_Foot_Text_green, g_settings.menu_Foot_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Foot_alpha) );
	
	// channellist colored events
	frameBuffer->paletteGenFade(COL_COLORED_EVENTS_CHANNELLIST,
	                              convertSetupColor2RGB(int(g_settings.menu_Content_red), int(g_settings.menu_Content_green), int(g_settings.menu_Content_blue)),
	                              convertSetupColor2RGB(g_settings.infobar_colored_events_red, g_settings.infobar_colored_events_green, g_settings.infobar_colored_events_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );

	frameBuffer->paletteSet();

	return false;
}

// video setup notifier
extern int prev_video_Mode;

bool CVideoSetupNotifier::changeNotify(const neutrino_locale_t OptionName, void *)
{
	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();

	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_ANALOG_MODE))	/* video analoue mode */
	{
		if(videoDecoder)
#if defined (PLATFORM_COOLSTREAM)
			videoDecoder->SetVideoMode((analog_mode_t) g_settings.analog_mode);
#else			
			videoDecoder->SetAnalogMode(g_settings.analog_mode);
#endif			
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_VIDEORATIO) || ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_VIDEOFORMAT ))	/* format aspect-ratio */
	{
		if(videoDecoder)
			videoDecoder->setAspectRatio(g_settings.video_Ratio, g_settings.video_Format);
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_VIDEOMODE))	/* mode */
	{
		if(videoDecoder)
			videoDecoder->SetVideoSystem(g_settings.video_Mode);
		
		// clear screen
		frameBuffer->paintBackground();
#ifdef FB_BLIT
		frameBuffer->blit();
#endif		

		if(prev_video_Mode != g_settings.video_Mode) 
		{
			if(ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_VIDEOMODE_OK), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_INFO) != CMessageBox::mbrYes) 
			{
				g_settings.video_Mode = prev_video_Mode;
				if(videoDecoder)
					videoDecoder->SetVideoSystem(g_settings.video_Mode);	//no-> return to prev mode
			} 
			else
			{
				prev_video_Mode = g_settings.video_Mode;
			}
		}
	}
#if !defined (PLATFORM_COOLSTREAM)	
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_HDMI_COLOR_SPACE)) 
	{
		if(videoDecoder)
			videoDecoder->SetSpaceColour(g_settings.hdmi_color_space);
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_WSS)) 
	{
		if(videoDecoder)
			videoDecoder->SetWideScreen(g_settings.wss_mode);
	}
#endif	

	return true;
}

// audio setup notifier
bool CAudioSetupNotifier::changeNotify(const neutrino_locale_t OptionName, void *)
{
	//printf("notify: %d\n", OptionName);

	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_AUDIOMENU_ANALOGOUT)) 
	{
		g_Zapit->setAudioMode(g_settings.audio_AnalogMode);
	} 
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_AUDIOMENU_HDMI_DD)) 
	{
		if(audioDecoder)
			audioDecoder->SetHdmiDD(g_settings.hdmi_dd );
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_AUDIOMENU_AVSYNC)) 
	{
#if defined (PLATFORM_COOLSTREAM)
		videoDecoder->SetSyncMode((AVSYNC_TYPE)g_settings.avsync);
		audioDecoder->SetSyncMode((AVSYNC_TYPE)g_settings.avsync);
		videoDemux->SetSyncMode((AVSYNC_TYPE)g_settings.avsync);
		audioDemux->SetSyncMode((AVSYNC_TYPE)g_settings.avsync);
		pcrDemux->SetSyncMode((AVSYNC_TYPE)g_settings.avsync);
#else
		if(videoDecoder)
			videoDecoder->SetSyncMode(g_settings.avsync);			
		
		if(audioDecoder)
			audioDecoder->SetSyncMode(g_settings.avsync);
#endif		
		
		//videoDemux->SetSyncMode(g_settings.avsync);
		//audioDemux->SetSyncMode(g_settings.avsync);
		//pcrDemux->SetSyncMode((g_settings.avsync);		
	}
#if !defined (PLATFORM_COOLSTREAM)	
	else if( ARE_LOCALES_EQUAL(OptionName, LOCALE_AUDIOMENU_AC3_DELAY) )
	{
		if(audioDecoder)
			audioDecoder->setHwAC3Delay(g_settings.ac3_delay);
	}
	else if( ARE_LOCALES_EQUAL(OptionName, LOCALE_AUDIOMENU_PCM_DELAY) )
	{
		if(audioDecoder)
			audioDecoder->setHwPCMDelay(g_settings.pcm_delay);
	}
#endif	

	return true;
}

bool CKeySetupNotifier::changeNotify(const neutrino_locale_t, void *)
{
	g_RCInput->setRepeat(atoi(g_settings.repeat_blocker), atoi(g_settings.repeat_genericblocker));

	return false;
}

// IP notifier
bool CIPChangeNotifier::changeNotify(const neutrino_locale_t locale, void * Data)
{
	if(locale == LOCALE_NETWORKMENU_IPADDRESS) 
	{
		char ip[16];
		unsigned char _ip[4];
		sscanf((char*) Data, "%hhu.%hhu.%hhu.%hhu", &_ip[0], &_ip[1], &_ip[2], &_ip[3]);

		sprintf(ip, "%hhu.%hhu.%hhu.255", _ip[0], _ip[1], _ip[2]);
		CNeutrinoApp::getInstance()->networkConfig.broadcast = ip;

		CNeutrinoApp::getInstance()->networkConfig.netmask = (_ip[0] == 10) ? "255.0.0.0" : "255.255.255.0";
	}
	else if(locale == LOCALE_NETWORKMENU_SELECT_IF) 
	{
#ifdef FB_BLIT
		// Width may change. Clear framebuffer, caller will redraw anyway.
		//CFrameBuffer::getInstance()->Clear();
		CFrameBuffer::getInstance()->blit();
#endif
		CNeutrinoApp::getInstance()->networkConfig.readConfig(g_settings.ifname);
		//readNetworkSettings(); //???
		dprintf(DEBUG_NORMAL, "CNetworkSetup::changeNotify: using %s, static %d\n", g_settings.ifname, CNeutrinoApp::getInstance()->networkConfig.inet_static);

		changeNotify(LOCALE_NETWORKMENU_DHCP, &CNeutrinoApp::getInstance()->networkConfig.inet_static);

#if 1
		int ecnt = sizeof(CNeutrinoApp::getInstance()->wlanEnable) / sizeof(CMenuItem*);
#endif
		for(int i = 0; i < ecnt; i++)
			CNeutrinoApp::getInstance()->wlanEnable[i]->setActive(CNeutrinoApp::getInstance()->networkConfig.wireless);

	}
	/*
	else if(locale == LOCALE_NETWORKMENU_DHCP) 
	{
		CNeutrinoApp::getInstance()->networkConfig.inet_static = (network_dhcp == 0 );
		int ecnt = sizeof(dhcpDisable) / sizeof(CMenuForwarder*);

		for(int i = 0; i < ecnt; i++)
			dhcpDisable[i]->setActive(CNetworkConfig::getInstance()->inet_static);
	}
	*/

	return true;
}

// timing settings notifier
bool CTimingSettingsNotifier::changeNotify(const neutrino_locale_t OptionName, void *)
{
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
	{
		if (ARE_LOCALES_EQUAL(OptionName, timing_setting_name[i]))
		{
			g_settings.timing[i] = 	atoi(g_settings.timing_string[i]);
			return true;
		}
	}
	return false;
}

// rec apids notifier
bool CRecAPIDSettingsNotifier::changeNotify(const neutrino_locale_t, void *)
{
	g_settings.recording_audio_pids_default = ( (g_settings.recording_audio_pids_std ? TIMERD_APIDS_STD : 0) | (g_settings.recording_audio_pids_alt ? TIMERD_APIDS_ALT : 0) | (g_settings.recording_audio_pids_ac3 ? TIMERD_APIDS_AC3 : 0));

	return true;
}

// apid changer exec
int CAPIDChangeExec::exec(CMenuTarget */*parent*/, const std::string & actionKey)
{
	dprintf(DEBUG_INFO, "CAPIDChangeExec exec: %s\n", actionKey.c_str());

	unsigned int sel = atoi(actionKey.c_str());
	
	if (g_RemoteControl->current_PIDs.PIDs.selected_apid != sel )
	{
		g_RemoteControl->setAPID(sel);
	}

	return menu_return::RETURN_EXIT;
}

// txt/dvb sub change exec
int CSubtitleChangeExec::exec(CMenuTarget */*parent*/, const std::string & actionKey)
{
	dprintf(DEBUG_INFO, "CSubtitleChangeExec::exec: action %s\n", actionKey.c_str());
	
	if(actionKey == "off") 
	{
		// tuxtxt stop
		tuxtx_stop_subtitle(); //this kill sub thread
		
		// dvbsub stop
		dvbsub_stop();
		//dvbsub_close();
		
		return menu_return::RETURN_EXIT;
	}
	
	if(!strncmp(actionKey.c_str(), "DVB", 3)) 
	{
		char const * pidptr = strchr(actionKey.c_str(), ':');
		int pid = atoi(pidptr+1);
		
		// tuxtxt stop
		tuxtx_stop_subtitle();
		
		dvbsub_pause();
		dvbsub_start(pid);
	} 
	else 
	{
		char const * ptr = strchr(actionKey.c_str(), ':');
		ptr++;
		int pid = atoi(ptr);
		ptr = strchr(ptr, ':');
		ptr++;
		int page = strtol(ptr, NULL, 16);
		ptr = strchr(ptr, ':');
		ptr++;
		
		dprintf(DEBUG_NORMAL, "CSubtitleChangeExec::exec: TTX, pid %x page %x lang %s\n", pid, page, ptr);
		
		dvbsub_stop();
		
		tuxtx_stop_subtitle();
		
		tuxtx_set_pid(pid, page, ptr);
		
		// start tuxtxt
		tuxtx_main(g_RCInput->getFileHandle(), pid, page, (live_fe)?live_fe->fenumber : 0 ); //FIXME
	}
	
        return menu_return::RETURN_EXIT;
}

// nvod change exec
int CNVODChangeExec::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_INFO, "CNVODChangeExec exec: %s\n", actionKey.c_str());
	
	unsigned int sel = atoi(actionKey.c_str());
	g_RemoteControl->setSubChannel(sel);

	parent->hide();
	g_InfoViewer->showSubchan();

	return menu_return::RETURN_EXIT;
}

// tuxtxt changer
extern int current_muted;
int CTuxtxtChangeExec::exec(CMenuTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_INFO, "CTuxtxtChangeExec exec: %s\n", actionKey.c_str());

	if(parent)
		parent->hide();
	
	g_RCInput->clearRCMsg();

	CNeutrinoApp::getInstance()->StopSubtitles();
				
	tuxtx_stop_subtitle();

	tuxtx_main(g_RCInput->getFileHandle(), g_RemoteControl->current_PIDs.PIDs.vtxtpid, 0, live_fe?live_fe->fenumber:0 );

	CFrameBuffer::getInstance()->paintBackground();

#if !defined USE_OPENGL
	CFrameBuffer::getInstance()->blit();
#endif
				
	g_RCInput->clearRCMsg();
				
	CNeutrinoApp::getInstance()->AudioMute(current_muted, true);

	CNeutrinoApp::getInstance()->StartSubtitles();

	return menu_return::RETURN_REPAINT;
}

const char * mypinghost(const char * const host)
{
	int retvalue = pinghost(host);
	switch (retvalue)
	{
		case 1: return (g_Locale->getText(LOCALE_PING_OK));
		case 0: return (g_Locale->getText(LOCALE_PING_UNREACHABLE));
		case -1: return (g_Locale->getText(LOCALE_PING_PROTOCOL));
		case -2: return (g_Locale->getText(LOCALE_PING_SOCKET));
	}
	return "";
}

void testNetworkSettings(const char* ip, const char* netmask, const char* broadcast, const char* gateway, const char* nameserver, bool ip_static)
{
	char our_ip[16];
	char our_mask[16];
	char our_broadcast[16];
	char our_gateway[16];
	char our_nameserver[16];
	std::string text;

	if (ip_static) 
	{
		strcpy(our_ip,ip);
		strcpy(our_mask,netmask);
		strcpy(our_broadcast,broadcast);
		strcpy(our_gateway,gateway);
		strcpy(our_nameserver,nameserver);
	}
	else 
	{
		netGetIP((char *) "eth0",our_ip,our_mask,our_broadcast);
		netGetDefaultRoute(our_gateway);
		netGetNameserver(our_nameserver);
	}

	dprintf(DEBUG_NORMAL, "testNw IP       : %s\n", our_ip);
	dprintf(DEBUG_NORMAL, "testNw Netmask  : %s\n", our_mask);
	dprintf(DEBUG_NORMAL, "testNw Broadcast: %s\n", our_broadcast);
	dprintf(DEBUG_NORMAL, "testNw Gateway: %s\n", our_gateway);
	dprintf(DEBUG_NORMAL, "testNw Nameserver: %s\n", our_nameserver);

	text = our_ip;
	text += ": ";
	text += mypinghost(our_ip);
	text += '\n';
	text += g_Locale->getText(LOCALE_NETWORKMENU_GATEWAY);
	text += ": ";
	text += our_gateway;
	text += ' ';
	text += mypinghost(our_gateway);
	text += '\n';
	text += g_Locale->getText(LOCALE_NETWORKMENU_NAMESERVER);
	text += ": ";
	text += our_nameserver;
	text += ' ';
	text += mypinghost(our_nameserver);
	text += "\nwww.google.de: ";
	text += mypinghost("173.194.35.152");

	ShowMsgUTF(LOCALE_NETWORKMENU_TEST, text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
}

void showCurrentNetworkSettings()
{
	char ip[16];
	char mask[16];
	char broadcast[16];
	char router[16];
	char nameserver[16];
	std::string mac;
	std::string text;

	//netGetIP((char *) "eth0",ip,mask,broadcast);
	netGetIP(g_settings.ifname, ip, mask, broadcast);
	
	if (ip[0] == 0) {
		text = "Network inactive\n";
	}
	else {
		netGetNameserver(nameserver);
		netGetDefaultRoute(router);
		//netGetMacAddr(g_settings.ifname, (unsigned char *)mac.c_str());
		
		//text = "Box: " + mac + "\n    ";
		
		text  = g_Locale->getText(LOCALE_NETWORKMENU_IPADDRESS );
		text += ": ";
		text += ip;
		text += '\n';
		text += g_Locale->getText(LOCALE_NETWORKMENU_NETMASK   );
		text += ": ";
		text += mask;
		text += '\n';
		text += g_Locale->getText(LOCALE_NETWORKMENU_BROADCAST );
		text += ": ";
		text += broadcast;
		text += '\n';
		text += g_Locale->getText(LOCALE_NETWORKMENU_NAMESERVER);
		text += ": ";
		text += nameserver;
		text += '\n';
		text += g_Locale->getText(LOCALE_NETWORKMENU_GATEWAY   );
		text += ": ";
		text += router;
	}
	ShowMsgUTF(LOCALE_NETWORKMENU_SHOW, text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
}

unsigned long long getcurrenttime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);
}

// USERMENU
#define USERMENU_ITEM_OPTION_COUNT SNeutrinoSettings::ITEM_MAX
const CMenuOptionChooser::keyval USERMENU_ITEM_OPTIONS[USERMENU_ITEM_OPTION_COUNT] =
{
        {SNeutrinoSettings::ITEM_NONE, LOCALE_USERMENU_ITEM_NONE, NULL},
        {SNeutrinoSettings::ITEM_BAR, LOCALE_USERMENU_ITEM_BAR, NULL},
        {SNeutrinoSettings::ITEM_EPG_LIST, LOCALE_EPGMENU_EVENTLIST, NULL},
        {SNeutrinoSettings::ITEM_EPG_SUPER, LOCALE_EPGMENU_EPGPLUS, NULL},
        {SNeutrinoSettings::ITEM_EPG_INFO, LOCALE_EPGMENU_EVENTINFO, NULL},
        {SNeutrinoSettings::ITEM_EPG_MISC, LOCALE_USERMENU_ITEM_EPG_MISC, NULL},
        {SNeutrinoSettings::ITEM_AUDIO_SELECT, LOCALE_AUDIOSELECTMENUE_HEAD, NULL},
        {SNeutrinoSettings::ITEM_SUBCHANNEL, LOCALE_INFOVIEWER_SUBSERVICE, NULL},
        {SNeutrinoSettings::ITEM_RECORD, LOCALE_TIMERLIST_TYPE_RECORD, NULL} ,
        {SNeutrinoSettings::ITEM_MOVIEPLAYER_MB, LOCALE_MOVIEPLAYER_MOVIES, NULL},
        {SNeutrinoSettings::ITEM_TIMERLIST, LOCALE_TIMERLIST_NAME, NULL},
        {SNeutrinoSettings::ITEM_REMOTE, LOCALE_RCLOCK_MENUEADD, NULL},
        {SNeutrinoSettings::ITEM_TECHINFO, LOCALE_EPGMENU_STREAMINFO, NULL},
        {SNeutrinoSettings::ITEM_PLUGIN, LOCALE_USERMENU_ITEM_PLUGINS, NULL},
        {SNeutrinoSettings::ITEM_VTXT, LOCALE_USERMENU_ITEM_VTXT, NULL},
        {SNeutrinoSettings::ITEM_GAME, LOCALE_MAINMENU_GAMES, NULL},
        {SNeutrinoSettings::ITEM_OPKG, LOCALE_OPKG_MANAGER, NULL},       
        {SNeutrinoSettings::ITEM_GLCD, LOCALE_GLCD_HEAD, NULL},
	{SNeutrinoSettings::ITEM_MOVIEPLAYER_TSMB, LOCALE_MOVIEPLAYER_RECORDS, NULL},
	{SNeutrinoSettings::ITEM_WEBTV, LOCALE_WEBTV_HEAD, NULL},
};

int CUserMenuMenu::exec(CMenuTarget *parent, const std::string &/*actionKey*/)
{
        if(parent != NULL)
                parent->hide();

        CMenuWidget menu(local , NEUTRINO_ICON_KEYBINDING);
	//menu.addItem(GenericMenuSeparator);
        menu.addItem(GenericMenuBack);
        menu.addItem(GenericMenuSeparatorLine);

        CStringInputSMS name(LOCALE_USERMENU_NAME, &g_settings.usermenu_text[button]);
        menu.addItem(new CMenuForwarder(LOCALE_USERMENU_NAME, true, g_settings.usermenu_text[button],&name));
        menu.addItem(GenericMenuSeparatorLine);

        char text[10];
        for(int item = 0; item < SNeutrinoSettings::ITEM_MAX; item++)
        {
                snprintf(text, 10, "%d:", item);
                text[9] = 0;// terminate for sure
                
                menu.addItem( new CMenuOptionChooser(text, &g_settings.usermenu[button][item], USERMENU_ITEM_OPTIONS, USERMENU_ITEM_OPTION_COUNT,true, NULL, CRCInput::RC_nokey, "", true ));
        }

        menu.exec(NULL, "");

        return menu_return::RETURN_REPAINT;
}

// TZ notifier
bool CTZChangeNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	bool found = false;
	std::string name, zone;
	
	dprintf(DEBUG_NORMAL, "CTZChangeNotifier::changeNotify: %s\n", (char *) Data);

        xmlDocPtr parser = parseXmlFile("/etc/timezone.xml");
        if (parser != NULL) 
	{
                xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
                while (search) 
		{
                        if (!strcmp(xmlGetName(search), "zone")) 
			{
                                name = xmlGetAttribute(search, (char *) "name");
                                zone = xmlGetAttribute(search, (char *) "zone");

				if(!strcmp(g_settings.timezone, name.c_str())) 
				{
					found = true;
					break;
				}
                        }
                        search = search->xmlNextNode;
                }
                xmlFreeDoc(parser);
        }

	if(found) 
	{
		dprintf(DEBUG_NORMAL, "CTZChangeNotifier::changeNotify: Timezone: %s -> %s\n", name.c_str(), zone.c_str());
		
		std::string cmd = "cp /usr/share/zoneinfo/" + zone + " /etc/localtime";
		
		dprintf(DEBUG_NORMAL, "exec %s\n", cmd.c_str());
		
		system(cmd.c_str());
		cmd = ":" + zone;
		setenv("TZ", cmd.c_str(), 1);
	}

	return true;
}

// data reset notifier
extern Zapit_config zapitCfg;
void loadZapitSettings();
void getZapitConfig(Zapit_config *Cfg);

int CDataResetNotifier::exec(CMenuTarget */*parent*/, const std::string& actionKey)
{
	CFileBrowser fileBrowser;
	CFileFilter fileFilter;

	if( actionKey == "factory") 
	{
		int result = ShowMsgUTF(LOCALE_RESET_SETTINGS, g_Locale->getText(LOCALE_RESET_CONFIRM), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo);
		if(result != CMessageBox::mbrYes) 
			return true;
		
		unlink(NEUTRINO_SETTINGS_FILE);
		CNeutrinoApp::getInstance()->loadSetup(NEUTRINO_SETTINGS_FILE);
		CNeutrinoApp::getInstance()->saveSetup(NEUTRINO_SETTINGS_FILE);
		
		CFrameBuffer::getInstance()->paintBackground();
#ifdef FB_BLIT
		CFrameBuffer::getInstance()->blit();
#endif		
		/*video mode*/
		if(videoDecoder)
		{
			videoDecoder->SetVideoSystem(g_settings.video_Mode);

			/*aspect-ratio*/
			videoDecoder->setAspectRatio(g_settings.video_Ratio, g_settings.video_Format);
#if defined (PLATFORM_COOLSTREAM)
			videoDecoder->SetVideoMode((analog_mode_t) g_settings.analog_mode);
#else			
			videoDecoder->SetAnalogMode( g_settings.analog_mode); 
#endif

#if !defined (PLATFORM_COOLSTREAM)	
			videoDecoder->SetSpaceColour(g_settings.hdmi_color_space);
#endif
		}
		/*audio mode */
		g_Zapit->setAudioMode(g_settings.audio_AnalogMode);

		if(audioDecoder)
			audioDecoder->SetHdmiDD(g_settings.hdmi_dd );

		CNeutrinoApp::getInstance()->SetupTiming();
	}
	else if(actionKey == "backup") 
	{
		fileBrowser.Dir_Mode = true;
		if (fileBrowser.exec("/media") == true) 
		{
			char  fname[256];
			struct statfs s;
			int ret = ::statfs(fileBrowser.getSelectedFile()->Name.c_str(), &s);

			if(ret == 0 && s.f_type != 0x72b6L/*jffs2*/ && s.f_type != 0x5941ff53L /*yaffs2*/)
			{ 
				const char backup_sh[] = "backup.sh";

				sprintf(fname, "%s %s", backup_sh, fileBrowser.getSelectedFile()->Name.c_str());
				
				dprintf(DEBUG_NORMAL, "CDataResetNotifier::exec: executing %s\n", fname);
				
				system(fname);
			} 
			else
				ShowMsgUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(LOCALE_SETTINGS_BACKUP_FAILED),CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_ERROR);
		}
	}
	else if(actionKey == "restore") 
	{
		fileFilter.addFilter("tar");
		fileBrowser.Filter = &fileFilter;
		if (fileBrowser.exec("/media") == true) 
		{
			int result = ShowMsgUTF(LOCALE_SETTINGS_RESTORE, g_Locale->getText(LOCALE_SETTINGS_RESTORE_WARN), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo);
			if(result == CMessageBox::mbrYes) 
			{
				char  fname[256];
				
				const char restore_sh[] = "restore.sh";
				
				sprintf(fname, "%s %s", restore_sh, fileBrowser.getSelectedFile()->Name.c_str());
				
				dprintf(DEBUG_NORMAL, "CDataResetNotifier::exec: executing %s\n", fname);
				
				system(fname);
			}
			
			
		}
	}

	return true;
}

// auto audio select notifier
CAutoAudioNotifier::CAutoAudioNotifier(CMenuItem * item1, CMenuItem * item2, CMenuItem * item3, CMenuItem * item4)
{
	toDisable[0] = item1;
	toDisable[1] = item2;
	toDisable[2] = item3;
	toDisable[3] = item4;
}

bool CAutoAudioNotifier::changeNotify(const neutrino_locale_t, void *)
{
	// only ac3
	toDisable[0]->setActive(g_settings.auto_lang);
	toDisable[1]->setActive(g_settings.auto_lang);
	toDisable[2]->setActive(g_settings.auto_lang);
	toDisable[3]->setActive(g_settings.auto_lang);
	
	return true;
}

void sectionsd_set_languages(const std::vector<std::string>& newLanguages);

bool CLangSelectNotifier::changeNotify(const neutrino_locale_t, void *)
{
	std::vector<std::string> v_languages;
	bool found = false;
	std::map<std::string, std::string>::const_iterator it;

	//prefered audio languages
	for(int i = 0; i < 3; i++) 
	{
		if(strlen(g_settings.pref_lang[i])) 
		{
			dprintf(DEBUG_NORMAL, "setLanguages: %d: %s\n", i, g_settings.pref_lang[i]);
			
			std::string temp(g_settings.pref_lang[i]);
			for(it = iso639.begin(); it != iso639.end(); it++) 
			{
				if(temp == it->second) 
				{
					v_languages.push_back(it->first);
					
					dprintf(DEBUG_NORMAL, "setLanguages: adding %s\n", it->first.c_str());
					
					found = true;
				}
			}
		}
	}
	
	if(found)
		sectionsd_set_languages(v_languages);
	
	return true;
}

// sublang select notifier
CSubLangSelectNotifier::CSubLangSelectNotifier(CMenuItem * item1, CMenuItem * item2, CMenuItem * item3)
{
	toDisable[0] = item1;
	toDisable[1] = item2;
	toDisable[2] = item3;
}

bool CSubLangSelectNotifier::changeNotify(const neutrino_locale_t, void *)
{
	// only ac3
	toDisable[0]->setActive(g_settings.auto_subs);
	toDisable[1]->setActive(g_settings.auto_subs);
	toDisable[2]->setActive(g_settings.auto_subs);
	
	return true;
}

// scansetup notifier
CScanSetupNotifier::CScanSetupNotifier(int num)
{
	feindex = num;
}

/* items1 enabled for advanced diseqc settings, items2 for diseqc != NO_DISEQC, items3 disabled for NO_DISEQC */
bool CScanSetupNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	std::vector<CMenuItem*>::iterator it;
	int FeMode = *((int*) Data);
	
	dprintf(DEBUG_NORMAL, "CScanSetupNotifier::changeNotify: Femode:%d\n", FeMode);

	if ( (FeMode == FE_NOTCONNECTED) || (FeMode == FE_LOOP) ) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			(*it)->setActive(false);
		}
		for(it = items2.begin(); it != items2.end(); it++) 
		{
			(*it)->setActive(false);
		}
		for(it = items3.begin(); it != items3.end(); it++) 
		{
			(*it)->setActive(false);
		}
	}
	else
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			(*it)->setActive(true);
		}
		for(it = items2.begin(); it != items2.end(); it++) 
		{
			(*it)->setActive(true);
		}
		for(it = items3.begin(); it != items3.end(); it++) 
		{
			(*it)->setActive(true);
		}
	}

	return true;
}

void CScanSetupNotifier::addItem(int list, CMenuItem* item)
{
	switch(list) 
	{
		case 0:
			items1.push_back(item);
			break;	
		case 1:
			items2.push_back(item);
			break;
		case 2:
			items3.push_back(item);
			break;
		default:
			break;
	}
}

// volume conf
bool CAudioSetupNotifierVolPercent::changeNotify(const neutrino_locale_t OptionName __attribute__((unused)), void *data)
{
	int percent = *(int *) data;
	
	g_Zapit->setVolumePercent(percent, live_channel_id, g_RemoteControl->current_PIDs.PIDs.selected_apid);
	
	return true;
}

// mkdir (0755)
int safe_mkdir(char * path)
{
	struct statfs s;
	int ret = 0;

	if(!strncmp(path, "/hdd", 4)) 
	{
		ret = statfs("/hdd", &s);

		if((ret != 0) || (s.f_type == 0x72b6)) 
			ret = -1;
		else 
			mkdir(path, 0755);
	} 
	else
		mkdir(path, 0755);

	return ret;
}

// check fs
int check_dir(const char * newdir)
{
  
  	struct statfs s;
	
	if (::statfs(newdir, &s) == 0) 
	{
		switch (s.f_type)	/* f_type is long */
		{
			case 0xEF53L:		/*EXT2 & EXT3 & EXT4*/
			case 0x6969L:		/*NFS*/
			case 0xFF534D42L:	/*CIFS*/
			case 0x517BL:		/*SMB*/
			case 0x52654973L:	/*REISERFS*/
			case 0x65735546L:	/*fuse for ntfs*/
			case 0x5346544eL:	/*ntfs*/
			case 0x58465342L:	/*xfs*/
			case 0x4d44L:		/*msdos*/
			case 0x3153464aL:	/*jfs*/
			case 0x4006L:		/*fat*/
				return 0;//ok
			default:
				fprintf( stderr,"%s Unknow File system type: %i\n",newdir ,s.f_type);
			  break;
		}
	}
	
	return 1;//error			  
}

bool get_fs_usage(const char * dir, long &btotal, long &bused, long *bsize /*=NULL*/)
{
	btotal = bused = 0;
	struct statfs s;

	if (::statfs(dir, &s) == 0 && s.f_blocks) 
	{
		btotal = s.f_blocks;
		bused = s.f_blocks - s.f_bfree;
		if (bsize != NULL)
			*bsize = s.f_bsize;
		//printf("fs (%s): total %ld used %ld\n", dir, btotal, bused);
		return true;
	}
	
	return false;
}



