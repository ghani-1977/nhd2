/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define NEUTRINO_CPP

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>

#include <sys/socket.h>

#include <iostream>
#include <fstream>
#include <string>

#include <linux/fs.h>

#include "global.h"
#include "neutrino.h"

#include <daemonc/remotecontrol.h>

#include <driver/encoding.h>
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/stream2file.h>
#include <driver/vcrcontrol.h>
#include <driver/shutdown_count.h>

#include <gui/epgplus.h>
#include <gui/streaminfo2.h>

#include "gui/widget/colorchooser.h"
#include "gui/widget/menue.h"
#include "gui/widget/messagebox.h"
#include "gui/widget/hintbox.h"
#include "gui/widget/icons.h"
#include "gui/widget/vfdcontroler.h"
#include "gui/widget/keychooser.h"
#include "gui/widget/stringinput.h"
#include "gui/widget/stringinput_ext.h"
#include "gui/widget/mountchooser.h"

#include "gui/color.h"

#include "gui/bedit/bouqueteditor_bouquets.h"
#include "gui/bouquetlist.h"
#include "gui/eventlist.h"
#include "gui/channellist.h"
#include "gui/screensetup.h"
#include "gui/pluginlist.h"
#include "gui/plugins.h"
#include "gui/infoviewer.h"
#include "gui/epgview.h"
#include "gui/epg_menu.h"
#include "gui/update.h"
#include "gui/scan.h"
#include "gui/favorites.h"
#include "gui/sleeptimer.h"
#include "gui/rc_lock.h"
#include "gui/timerlist.h"
#include "gui/alphasetup.h"
#include "gui/audioplayer.h"
#include "gui/imageinfo.h"
#include "gui/movieplayer.h"
#include "gui/nfs.h"
#include "gui/pictureviewer.h"
#include "gui/motorcontrol.h"
#include "gui/filebrowser.h"
#include "gui/widget/progressbar.h"

#include "gui/cam_menu.h"

#include "gui/hdd_menu.h"

#include <system/setting_helpers.h>
#include <system/settings.h>
#include <system/debug.h>
#include <system/flashtool.h>
#include <system/fsmounter.h>

#include <timerdclient/timerdmsg.h>

#include <zapit/frontend_c.h>
#include <zapit/getservices.h>
#include <zapit/satconfig.h>

#include <string.h>
#include <linux/reboot.h>
#include <sys/reboot.h>

#include "gui/dboxinfo.h"
#include "gui/audio_select.h"

#include "gui/scan_setup.h"

#if ENABLE_GRAPHLCD
#include <driver/nglcd.h>
#endif

//libcoolstream
#include <video_cs.h>
#include <audio_cs.h>

// dvbsubs selct menu
#include <gui/dvbsub_select.h>

#include <zapit/channel.h>
#include <zapit/bouquets.h>


extern tallchans allchans;
extern CBouquetManager * g_bouquetManager;

// debug
extern int sections_debug;

int old_b_id = -1;

// hintbox
CHintBox * reloadhintBox = 0;

extern bool has_hdd;

// record and timeshift
bool autoshift = false;
uint32_t shift_timer;
uint32_t scrambled_timer;
char recDir[255];
char timeshiftDir[255];
std::string tmode;

// tuxtxt
//extern int  tuxtxt_init();
//extern void tuxtxt_start(int tpid, int source );
extern int  tuxtxt_stop();
extern void tuxtxt_close();
extern void tuxtx_pause_subtitle(bool pause, int source);
extern void tuxtx_stop_subtitle();
extern void tuxtx_set_pid(int pid, int page, const char * cc);
extern int tuxtx_subtitle_running(int *pid, int *page, int *running);
extern int tuxtx_main(int _rc, int pid, int page, int source );

// dvbsub
//extern int dvbsub_initialise();
extern int dvbsub_init( /*int source*/);
extern int dvbsub_stop();
extern int dvbsub_close();
extern int dvbsub_start(int pid);
extern int dvbsub_pause();
extern int dvbsub_getpid();
extern void dvbsub_setpid(int pid);
extern int dvbsub_terminate();

// volume bar
static CProgressBar 		* g_volscale;


// timerd thread
static pthread_t timer_thread;
void * timerd_main_thread(void *data);

// streamts thread
extern int streamts_stop;
void * streamts_main_thread(void *data);
static pthread_t stream_thread ;

// zapit thread
extern int zapit_ready;					//defined in zapit.cpp
static pthread_t zapit_thread ;
void * zapit_main_thread(void *data);
extern t_channel_id live_channel_id; 			//defined in zapit.cpp
Zapit_config zapitCfg;
void setZapitConfig(Zapit_config * Cfg);
void getZapitConfig(Zapit_config * Cfg);
extern CZapitChannel * live_channel;			// zapit.cpp
extern CFrontend * live_fe;
extern CScanSettings * scanSettings;

//nhttpd thread
void * nhttpd_main_thread(void *data);
static pthread_t nhttpd_thread ;

// sectionsd thread
extern int sectionsd_stop;
static pthread_t sections_thread;
void * sectionsd_main_thread(void *data);
extern bool timeset;

//Audio/Video Decoder
extern cVideo 		* videoDecoder;		//libcoolstream (video_cs.cpp)
extern cAudio 		* audioDecoder;		//libcoolstream (audio_cs.cpp)

int prev_video_Mode;

void stop_daemons();

CAPIDChangeExec		* APIDChanger;			
CVideoSetupNotifier	* videoSetupNotifier;
CAudioSetupNotifier	* audioSetupNotifier;

/* bouquets lists */
CBouquetList   		* bouquetList; 				//current bqt list

CBouquetList   		* TVbouquetList;
CBouquetList   		* TVsatList;
CBouquetList   		* TVfavList;
CBouquetList   		* TVallList;

CBouquetList   		* RADIObouquetList;
CBouquetList   		* RADIOsatList;
CBouquetList   		* RADIOfavList;
CBouquetList   		* RADIOallList;

CPlugins       		* g_PluginList;
CRemoteControl 		* g_RemoteControl;
SMSKeyInput 		* c_SMSKeyInput;	//defined in filebrowser and used in ChanneList
CMoviePlayerGui		* moviePlayerGui;
CPictureViewer 		* g_PicViewer;
CCAMMenuHandler 	* g_CamHandler;

bool parentallocked = false;
static char **global_argv;

extern const char * locale_real_names[]; 		//#include <system/locals_intern.h>

//user menu
const char* usermenu_button_def[SNeutrinoSettings::BUTTON_MAX]={
	"red", 
	"green", 
	"yellow", 
	"blue",
#if defined (PLATFORM_GIGABLUE)
	"f1",
	"f2",
	"f3",
	"f4"
#endif
};

CVCRControl::CDevice * recordingdevice = NULL;

// init globals
static void initGlobals(void)
{
	g_fontRenderer  = NULL;
	g_RCInput       = NULL;
	g_Timerd        = NULL;
	g_Zapit 	= new CZapitClient;
	g_RemoteControl = NULL;
	g_EpgData       = NULL;
	g_InfoViewer    = NULL;
	g_EventList     = NULL;
	g_Locale        = new CLocaleManager;
	g_PluginList    = NULL;
	g_CamHandler 	= NULL;

#if ENABLE_RADIOTEXT
	g_Radiotext     = NULL;
#endif	
}

// CNeutrinoApp - Constructor, initialize g_fontRenderer
CNeutrinoApp::CNeutrinoApp()
: configfile('\t')
{
	standby_pressed_at.tv_sec = 0;

	frameBuffer = CFrameBuffer::getInstance();
	frameBuffer->setIconBasePath(DATADIR "/neutrino/icons/");
	SetupFrameBuffer();

	mode = mode_unknown;
	
	channelList       = NULL;
	TVchannelList       = NULL;
	RADIOchannelList       = NULL;
	
	nextRecordingInfo = NULL;
	skipShutdownTimer = false;
	current_muted = 0;
	
	memset(&font, 0, sizeof(neutrino_font_descr_struct));
}

// CNeutrinoApp - Destructor
CNeutrinoApp::~CNeutrinoApp()
{
	if (channelList)
		delete channelList;
}

// getInstance
CNeutrinoApp * CNeutrinoApp::getInstance()
{
	static CNeutrinoApp * neutrinoApp = NULL;

	if(!neutrinoApp) 
	{
		neutrinoApp = new CNeutrinoApp();
		dprintf(DEBUG_NORMAL, "NeutrinoApp Instance created\n");
	}

	return neutrinoApp;
}

// CNeutrinoApp -  setup Color Sheme (Neutrino)
void CNeutrinoApp::setupColors_neutrino()
{
	g_settings.menu_Head_alpha = 0x00;
	g_settings.menu_Head_red   = 0x00;
	g_settings.menu_Head_green = 0x0A;
	g_settings.menu_Head_blue  = 0x19;

	g_settings.menu_Head_Text_alpha = 0x00;
	g_settings.menu_Head_Text_red   = 0x5f;
	g_settings.menu_Head_Text_green = 0x46;
	g_settings.menu_Head_Text_blue  = 0x00;

	g_settings.menu_Content_alpha = 0x14;
	g_settings.menu_Content_red   = 0x00;
	g_settings.menu_Content_green = 0x0f;
	g_settings.menu_Content_blue  = 0x23;

	g_settings.menu_Content_Text_alpha = 0x00;
	g_settings.menu_Content_Text_red   = 0x64;
	g_settings.menu_Content_Text_green = 0x64;
	g_settings.menu_Content_Text_blue  = 0x64;

	g_settings.menu_Content_Selected_alpha = 0x14;
	g_settings.menu_Content_Selected_red   = 0x19;
	g_settings.menu_Content_Selected_green = 0x37;
	g_settings.menu_Content_Selected_blue  = 0x64;

	g_settings.menu_Content_Selected_Text_alpha  = 0x00;
	g_settings.menu_Content_Selected_Text_red    = 0x00;
	g_settings.menu_Content_Selected_Text_green  = 0x00;
	g_settings.menu_Content_Selected_Text_blue   = 0x00;

	g_settings.menu_Content_inactive_alpha = 0x14;
	g_settings.menu_Content_inactive_red   = 0x00;
	g_settings.menu_Content_inactive_green = 0x0f;
	g_settings.menu_Content_inactive_blue  = 0x23;

	g_settings.menu_Content_inactive_Text_alpha  = 0x00;
	g_settings.menu_Content_inactive_Text_red    = 55;
	g_settings.menu_Content_inactive_Text_green  = 70;
	g_settings.menu_Content_inactive_Text_blue   = 85;

	g_settings.infobar_alpha = 0x14;
	g_settings.infobar_red   = 0x00;
	g_settings.infobar_green = 0x0e;
	g_settings.infobar_blue  = 0x23;

	g_settings.infobar_Text_alpha = 0x00;
	g_settings.infobar_Text_red   = 0x64;
	g_settings.infobar_Text_green = 0x64;
	g_settings.infobar_Text_blue  = 0x64;
}

// CNeutrinoApp -  setup Color Sheme (classic)
void CNeutrinoApp::setupColors_classic()
{
	g_settings.menu_Head_alpha = 20;
	g_settings.menu_Head_red   =  5;
	g_settings.menu_Head_green = 10;
	g_settings.menu_Head_blue  = 60;

	g_settings.menu_Head_Text_alpha = 0;
	g_settings.menu_Head_Text_red   = 100;
	g_settings.menu_Head_Text_green = 100;
	g_settings.menu_Head_Text_blue  = 100;

	g_settings.menu_Content_alpha = 20;
	g_settings.menu_Content_red   = 50;
	g_settings.menu_Content_green = 50;
	g_settings.menu_Content_blue  = 50;

	g_settings.menu_Content_Text_alpha = 0;
	g_settings.menu_Content_Text_red   = 100;
	g_settings.menu_Content_Text_green = 100;
	g_settings.menu_Content_Text_blue  = 100;

	g_settings.menu_Content_Selected_alpha = 20;
	g_settings.menu_Content_Selected_red   = 5;
	g_settings.menu_Content_Selected_green = 10;
	g_settings.menu_Content_Selected_blue  = 60;

	g_settings.menu_Content_Selected_Text_alpha  = 0;
	g_settings.menu_Content_Selected_Text_red    = 100;
	g_settings.menu_Content_Selected_Text_green  = 100;
	g_settings.menu_Content_Selected_Text_blue   = 100;

	g_settings.menu_Content_inactive_alpha = 20;
	g_settings.menu_Content_inactive_red   = 50;
	g_settings.menu_Content_inactive_green = 50;
	g_settings.menu_Content_inactive_blue  = 50;

	g_settings.menu_Content_inactive_Text_alpha  = 0;
	g_settings.menu_Content_inactive_Text_red    = 80;
	g_settings.menu_Content_inactive_Text_green  = 80;
	g_settings.menu_Content_inactive_Text_blue   = 80;

	g_settings.infobar_alpha = 20;
	g_settings.infobar_red   = 5;
	g_settings.infobar_green = 10;
	g_settings.infobar_blue  = 60;

	g_settings.infobar_Text_alpha = 0;
	g_settings.infobar_Text_red   = 100;
	g_settings.infobar_Text_green = 100;
	g_settings.infobar_Text_blue  = 100;
}

//CNeutrinoApp -  setup Color Sheme (gray)
void CNeutrinoApp::setupColors_ru()
{
	g_settings.infobar_Text_alpha=0;
	g_settings.infobar_Text_blue=100;
	g_settings.infobar_Text_green=100;
	g_settings.infobar_Text_red=100;

	g_settings.infobar_alpha=0;
	g_settings.infobar_blue=40;
	g_settings.infobar_green=29;
	g_settings.infobar_red=25;

	g_settings.menu_Content_Selected_Text_alpha=0;
	g_settings.menu_Content_Selected_Text_blue=0;
	g_settings.menu_Content_Selected_Text_green=0;
	g_settings.menu_Content_Selected_Text_red=0;

	g_settings.menu_Content_Selected_alpha=0;
	g_settings.menu_Content_Selected_blue=70;
	g_settings.menu_Content_Selected_green=65;
	g_settings.menu_Content_Selected_red=65;

	g_settings.menu_Content_Text_alpha=0;
	g_settings.menu_Content_Text_blue=100;
	g_settings.menu_Content_Text_green=100;
	g_settings.menu_Content_Text_red=100;

	g_settings.menu_Content_alpha=0;
	g_settings.menu_Content_blue=40;
	g_settings.menu_Content_green=30;
	g_settings.menu_Content_red=25;

	g_settings.menu_Content_inactive_Text_alpha=0;
	g_settings.menu_Content_inactive_Text_blue=64;
	g_settings.menu_Content_inactive_Text_green=60;
	g_settings.menu_Content_inactive_Text_red=56;
	
	g_settings.menu_Content_inactive_alpha=0;
	g_settings.menu_Content_inactive_blue=40;
	g_settings.menu_Content_inactive_green=30;
	g_settings.menu_Content_inactive_red=25;

	g_settings.menu_Head_Text_alpha=0;
	g_settings.menu_Head_Text_blue=0;
	g_settings.menu_Head_Text_green=70;
	g_settings.menu_Head_Text_red=95;

	g_settings.menu_Head_alpha=0;
	g_settings.menu_Head_blue=30;
	g_settings.menu_Head_green=20;
	g_settings.menu_Head_red=15;
}

//CNeutrinoApp -  setup Color Sheme (red)
void CNeutrinoApp::setupColors_red()
{
	g_settings.infobar_Text_alpha=0;
	g_settings.infobar_Text_blue=100;
	g_settings.infobar_Text_green=100;
	g_settings.infobar_Text_red=100;

	g_settings.infobar_alpha=20;
	g_settings.infobar_blue=0;
	g_settings.infobar_green=14;
	g_settings.infobar_red=40;

	g_settings.menu_Content_Selected_Text_alpha=0;
	g_settings.menu_Content_Selected_Text_blue=0;
	g_settings.menu_Content_Selected_Text_green=0;
	g_settings.menu_Content_Selected_Text_red=0;

	g_settings.menu_Content_Selected_alpha=20;
	g_settings.menu_Content_Selected_blue=100;
	g_settings.menu_Content_Selected_green=100;
	g_settings.menu_Content_Selected_red=100;

	g_settings.menu_Content_Text_alpha=0;
	g_settings.menu_Content_Text_blue=100;
	g_settings.menu_Content_Text_green=100;
	g_settings.menu_Content_Text_red=100;

	g_settings.menu_Content_inactive_Text_alpha=0;
	g_settings.menu_Content_inactive_Text_blue=35;
	g_settings.menu_Content_inactive_Text_green=85;
	g_settings.menu_Content_inactive_Text_red=85;

	g_settings.menu_Content_inactive_alpha=20;
	g_settings.menu_Content_inactive_blue=0;
	g_settings.menu_Content_inactive_green=15;
	g_settings.menu_Content_inactive_red=35;

	g_settings.menu_Content_alpha=20;
	g_settings.menu_Content_blue=0;
	g_settings.menu_Content_green=15;
	g_settings.menu_Content_red=40;

	g_settings.menu_Head_Text_alpha=0;
	g_settings.menu_Head_Text_blue=0;
	g_settings.menu_Head_Text_green=70;
	g_settings.menu_Head_Text_red=95;

	g_settings.menu_Head_alpha=0;
	g_settings.menu_Head_blue=0;
	g_settings.menu_Head_green=10;
	g_settings.menu_Head_red=40;
}

// CNeutrinoApp -  setup Color Sheme (dvb2000)
void CNeutrinoApp::setupColors_dvb2k()
{
	g_settings.menu_Head_alpha = 0;
	g_settings.menu_Head_red   = 25;
	g_settings.menu_Head_green = 25;
	g_settings.menu_Head_blue  = 25;

	g_settings.menu_Head_Text_alpha = 0;
	g_settings.menu_Head_Text_red   = 100;
	g_settings.menu_Head_Text_green = 100;
	g_settings.menu_Head_Text_blue  = 0;

	g_settings.menu_Content_alpha = 0;
	g_settings.menu_Content_red   = 0;
	g_settings.menu_Content_green = 20;
	g_settings.menu_Content_blue  = 0;

	g_settings.menu_Content_Text_alpha = 0;
	g_settings.menu_Content_Text_red   = 100;
	g_settings.menu_Content_Text_green = 100;
	g_settings.menu_Content_Text_blue  = 100;

	g_settings.menu_Content_Selected_alpha = 0;
	g_settings.menu_Content_Selected_red   = 100;
	g_settings.menu_Content_Selected_green = 100;
	g_settings.menu_Content_Selected_blue  = 100;

	g_settings.menu_Content_Selected_Text_alpha  = 0;
	g_settings.menu_Content_Selected_Text_red    = 0;
	g_settings.menu_Content_Selected_Text_green  = 0;
	g_settings.menu_Content_Selected_Text_blue   = 0;

	g_settings.menu_Content_inactive_alpha = 20;
	g_settings.menu_Content_inactive_red   = 0;
	g_settings.menu_Content_inactive_green = 25;
	g_settings.menu_Content_inactive_blue  = 0;

	g_settings.menu_Content_inactive_Text_alpha  = 0;
	g_settings.menu_Content_inactive_Text_red    = 100;
	g_settings.menu_Content_inactive_Text_green  = 100;
	g_settings.menu_Content_inactive_Text_blue   = 0;

	g_settings.infobar_alpha = 5;
	g_settings.infobar_red   = 0;
	g_settings.infobar_green = 19;
	g_settings.infobar_blue  = 0;

	g_settings.infobar_Text_alpha = 0;
	g_settings.infobar_Text_red   = 100;
	g_settings.infobar_Text_green = 100;
	g_settings.infobar_Text_blue  = 100;
}

// nhd2
void CNeutrinoApp::setupColors_nhd2()
{
	g_settings.menu_Head_alpha = 0;
	g_settings.menu_Head_red   = 35;
	g_settings.menu_Head_green = 35;
	g_settings.menu_Head_blue  = 35;

	g_settings.menu_Head_Text_alpha = 0;
	g_settings.menu_Head_Text_red   = 100;
	g_settings.menu_Head_Text_green = 100;
	g_settings.menu_Head_Text_blue  = 100;

	g_settings.menu_Content_alpha = 20;
	g_settings.menu_Content_red   = 50;
	g_settings.menu_Content_green = 50;
	g_settings.menu_Content_blue  = 50;

	g_settings.menu_Content_Text_alpha = 0;
	g_settings.menu_Content_Text_red   = 100;
	g_settings.menu_Content_Text_green = 100;
	g_settings.menu_Content_Text_blue  = 100;

	g_settings.menu_Content_Selected_alpha = 15;
	g_settings.menu_Content_Selected_red   = 20;
	g_settings.menu_Content_Selected_green = 20;
	g_settings.menu_Content_Selected_blue  = 20;

	g_settings.menu_Content_Selected_Text_alpha  = 0;
	g_settings.menu_Content_Selected_Text_red    = 100;
	g_settings.menu_Content_Selected_Text_green  = 100;
	g_settings.menu_Content_Selected_Text_blue   = 100;

	g_settings.menu_Content_inactive_alpha = 20;
	g_settings.menu_Content_inactive_red   = 50;
	g_settings.menu_Content_inactive_green = 50;
	g_settings.menu_Content_inactive_blue  = 50;

	g_settings.menu_Content_inactive_Text_alpha  = 0;
	g_settings.menu_Content_inactive_Text_red    = 70;
	g_settings.menu_Content_inactive_Text_green  = 70;
	g_settings.menu_Content_inactive_Text_blue   = 70;

	g_settings.infobar_alpha = 20;
	g_settings.infobar_red   = 50;
	g_settings.infobar_green = 50;
	g_settings.infobar_blue  = 50;

	g_settings.infobar_Text_alpha = 0;
	g_settings.infobar_Text_red   = 100;
	g_settings.infobar_Text_green = 100;
	g_settings.infobar_Text_blue  = 100;
}

// fonts
#define FONT_STYLE_REGULAR 0
#define FONT_STYLE_BOLD    1
#define FONT_STYLE_ITALIC  2

/* neutrino_font */
font_sizes_struct neutrino_font[FONT_TYPE_COUNT] =
{
        {LOCALE_FONTSIZE_MENU               ,  20, FONT_STYLE_BOLD   , 0},
        {LOCALE_FONTSIZE_MENU_TITLE         ,  30, FONT_STYLE_BOLD   , 0},
        {LOCALE_FONTSIZE_MENU_INFO          ,  16, FONT_STYLE_REGULAR, 0},
        {LOCALE_FONTSIZE_EPG_TITLE          ,  25, FONT_STYLE_REGULAR, 1},
        {LOCALE_FONTSIZE_EPG_INFO1          ,  17, FONT_STYLE_ITALIC , 2},
        {LOCALE_FONTSIZE_EPG_INFO2          ,  17, FONT_STYLE_REGULAR, 2},
        {LOCALE_FONTSIZE_EPG_DATE           ,  15, FONT_STYLE_REGULAR, 2},
        {LOCALE_FONTSIZE_EVENTLIST_TITLE    ,  30, FONT_STYLE_REGULAR, 0},
        {LOCALE_FONTSIZE_EVENTLIST_ITEMLARGE,  20, FONT_STYLE_BOLD   , 1},
        {LOCALE_FONTSIZE_EVENTLIST_ITEMSMALL,  14, FONT_STYLE_REGULAR, 1},
        {LOCALE_FONTSIZE_EVENTLIST_DATETIME ,  16, FONT_STYLE_REGULAR, 1},
        {LOCALE_FONTSIZE_GAMELIST_ITEMLARGE ,  20, FONT_STYLE_BOLD   , 1},
        {LOCALE_FONTSIZE_GAMELIST_ITEMSMALL ,  16, FONT_STYLE_REGULAR, 1},
        {LOCALE_FONTSIZE_CHANNELLIST        ,  20, FONT_STYLE_BOLD   , 1},
        {LOCALE_FONTSIZE_CHANNELLIST_DESCR  ,  20, FONT_STYLE_REGULAR, 1},
        {LOCALE_FONTSIZE_CHANNELLIST_NUMBER ,  14, FONT_STYLE_BOLD   , 2},
        {LOCALE_FONTSIZE_CHANNEL_NUM_ZAP    ,  40, FONT_STYLE_BOLD   , 0},
        {LOCALE_FONTSIZE_INFOBAR_NUMBER     ,  30, FONT_STYLE_BOLD   , 0},
        {LOCALE_FONTSIZE_INFOBAR_CHANNAME   ,  30, FONT_STYLE_BOLD   , 0},
        {LOCALE_FONTSIZE_INFOBAR_INFO       ,  20, FONT_STYLE_REGULAR, 1},
        {LOCALE_FONTSIZE_INFOBAR_SMALL      ,  14, FONT_STYLE_REGULAR, 1},
        {LOCALE_FONTSIZE_FILEBROWSER_ITEM   ,  16, FONT_STYLE_BOLD   , 1}
};

/* signal font */
const font_sizes_struct signal_font = {LOCALE_FONTSIZE_INFOBAR_SMALL,  14, FONT_STYLE_REGULAR, 1};

// LCD settings
typedef struct lcd_setting_t
{
	const char * const name;
	const unsigned int default_value;
} lcd_setting_struct_t;

const lcd_setting_struct_t lcd_setting[LCD_SETTING_COUNT] =
{
	{"lcd_brightness"       , DEFAULT_LCD_BRIGHTNESS       },
	{"lcd_standbybrightness", DEFAULT_LCD_STANDBYBRIGHTNESS},
	{"lcd_contrast"         , DEFAULT_LCD_CONTRAST         },
	{"lcd_power"            , DEFAULT_LCD_POWER            },
	{"lcd_inverse"          , DEFAULT_LCD_INVERSE          },
	{"lcd_show_volume"      , DEFAULT_LCD_SHOW_VOLUME      },
	{"lcd_autodimm"         , DEFAULT_LCD_AUTODIMM         },
	{"lcd_scroll_text"      , DEFAULT_LCD_SCROLL_TEXT      }
};

// loadSetup, load the application-settings
int CNeutrinoApp::loadSetup(const char * fname)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::loadSetup\n");
	
	char cfg_key[81];
	int erg = 0;

	configfile.clear();

	// settings laden - und dabei Defaults setzen!
	if(!configfile.loadConfig(fname)) 
	{
		//file existiert nicht
		erg = 1;
	}

	// parentallock check
        std::ifstream checkParentallocked(NEUTRINO_PARENTALLOCKED_FILE);
	if(checkParentallocked) 
	{
	        parentallocked = true;
	        checkParentallocked.close();
	}

	// video
#ifdef __sh__	
	g_settings.video_Mode = configfile.getInt32("video_Mode", VIDEO_STD_PAL);
#else
	g_settings.video_Mode = configfile.getInt32("video_Mode", VIDEO_STD_720P50);
#endif
	prev_video_Mode = g_settings.video_Mode;
	
	//analog mode
	g_settings.analog_mode = configfile.getInt32("analog_mode", ANALOG_YUV); 	// default yuv
	
	g_settings.hdmi_color_space = configfile.getInt32("hdmi_color_space", HDMI_RGB); //default RGB

	//aspect ratio
	g_settings.video_Ratio = configfile.getInt32("video_Ratio", ASPECTRATIO_169);		// 16:9
	 
	// policy	
	g_settings.video_Format = configfile.getInt32("video_Format", VIDEOFORMAT_PANSCAN2);

	//wss	
	g_settings.wss_mode = configfile.getInt32("wss_mode", WSS_OFF);
	
	g_settings.contrast = configfile.getInt32( "contrast", 130);
	g_settings.saturation = configfile.getInt32( "saturation", 130);
	g_settings.brightness = configfile.getInt32( "brightness", 130);
	g_settings.tint = configfile.getInt32( "tint", 130);
	// end video

	// audio
	g_settings.audio_AnalogMode = configfile.getInt32( "audio_AnalogMode", 0 );		//default=stereo
	g_settings.audio_DolbyDigital    = configfile.getBool("audio_DolbyDigital", false);
	
	// ac3
	g_settings.hdmi_dd = configfile.getInt32( "hdmi_dd", AC3_DOWNMIX);	// downmix
	
	// avsync
	g_settings.avsync = configfile.getInt32( "avsync", AVSYNC_ON);
	
	// ac3 delay
	g_settings.ac3_delay = configfile.getInt32( "ac3_delay", 0);
	
	// pcm delay
	g_settings.pcm_delay = configfile.getInt32( "pcm_delay", 0);
	
	g_settings.auto_lang = configfile.getInt32( "auto_lang", 0 );
	g_settings.auto_subs = configfile.getInt32( "auto_subs", 0 );

	for(int i = 0; i < 3; i++) 
	{
		sprintf(cfg_key, "pref_lang_%d", i);
		strncpy(g_settings.pref_lang[i], configfile.getString(cfg_key, "").c_str(), 30);
		sprintf(cfg_key, "pref_subs_%d", i);
		strncpy(g_settings.pref_subs[i], configfile.getString(cfg_key, "").c_str(), 30);
	}
	// end audio

	// parentallock
	if (!parentallocked) 
	{
	  	g_settings.parentallock_prompt = configfile.getInt32( "parentallock_prompt", 0 );
		g_settings.parentallock_lockage = configfile.getInt32( "parentallock_lockage", 12 );
	} 
	else 
	{
	        g_settings.parentallock_prompt = 3;
	        g_settings.parentallock_lockage = 18;
	}
	strcpy( g_settings.parentallock_pincode, configfile.getString( "parentallock_pincode", "0000" ).c_str() );
	// end parentallock

	// network
	g_settings.network_ntpserver    = configfile.getString("network_ntpserver", "time.fu-berlin.de");
        g_settings.network_ntprefresh   = configfile.getString("network_ntprefresh", "30" );
        g_settings.network_ntpenable    = configfile.getBool("network_ntpenable", false);
	
	snprintf(g_settings.ifname, sizeof(g_settings.ifname), "%s", configfile.getString("ifname", "eth0").c_str());

	// nfs entries
	for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++) 
	{
		sprintf(cfg_key, "network_nfs_ip_%d", i);
		g_settings.network_nfs_ip[i] = configfile.getString(cfg_key, "");
		sprintf(cfg_key, "network_nfs_dir_%d", i);
		strcpy( g_settings.network_nfs_dir[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_local_dir_%d", i);
		strcpy( g_settings.network_nfs_local_dir[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_automount_%d", i);
		g_settings.network_nfs_automount[i] = configfile.getInt32( cfg_key, 0);
		sprintf(cfg_key, "network_nfs_type_%d", i);
		g_settings.network_nfs_type[i] = configfile.getInt32( cfg_key, 0);
		sprintf(cfg_key,"network_nfs_username_%d", i);
		strcpy( g_settings.network_nfs_username[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_password_%d", i);
		strcpy( g_settings.network_nfs_password[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_mount_options1_%d", i);
		strcpy( g_settings.network_nfs_mount_options1[i], configfile.getString( cfg_key, "ro,soft,udp" ).c_str() );
		sprintf(cfg_key, "network_nfs_mount_options2_%d", i);
		strcpy( g_settings.network_nfs_mount_options2[i], configfile.getString( cfg_key, "nolock,rsize=8192,wsize=8192" ).c_str() );
		sprintf(cfg_key, "network_nfs_mac_%d", i);
		strcpy( g_settings.network_nfs_mac[i], configfile.getString( cfg_key, "11:22:33:44:55:66").c_str() );
	}
	// end network

	// recording
	g_settings.recording_type = configfile.getInt32("recording_type", RECORDING_FILE);
	strcpy( g_settings.network_nfs_recordingdir, configfile.getString( "network_nfs_recordingdir", "/media/sda1/record" ).c_str() );
	g_settings.temp_timeshift = configfile.getInt32( "temp_timeshift", 0 );
	g_settings.auto_timeshift = configfile.getInt32( "auto_timeshift", 0 );
	g_settings.auto_delete = configfile.getInt32( "auto_delete", 0);

	sprintf(timeshiftDir, "%s/.timeshift", g_settings.network_nfs_recordingdir);
	safe_mkdir(timeshiftDir);
	
	dprintf(DEBUG_INFO, "CNeutrinoApp::loadSetup: rec dir %s timeshift dir %s\n", g_settings.network_nfs_recordingdir, timeshiftDir);

	if(g_settings.auto_delete) 
	{
		char buf[512];
		sprintf(buf, "rm -f %s/*_temp.ts %s/*_temp.xml &", timeshiftDir, timeshiftDir);
		system(buf);
	}

	g_settings.record_hours = configfile.getInt32( "record_hours", 4 );
	g_settings.recording_audio_pids_default    = configfile.getInt32("recording_audio_pids_default", TIMERD_APIDS_STD | TIMERD_APIDS_AC3);
	g_settings.recording_epg_for_filename      = configfile.getBool("recording_epg_for_filename", false);
	g_settings.recording_save_in_channeldir      = configfile.getBool("recording_save_in_channeldir", false);
	
	g_settings.recording_zap_on_announce       = configfile.getBool("recording_zap_on_announce"      , false);
	// end recording

	// movieplayer
	strcpy( g_settings.network_nfs_moviedir, configfile.getString( "network_nfs_moviedir", "/media/sda1/movie" ).c_str() );
	
	// not in GUI
	//streaming (server)
	g_settings.streaming_type = configfile.getInt32( "streaming_type", 0 );
	g_settings.streaming_server_ip = configfile.getString("streaming_server_ip", "192.168.1.234");
	strcpy( g_settings.streaming_server_port, configfile.getString( "streaming_server_port", "8080").c_str() );
	strcpy( g_settings.streaming_server_cddrive, configfile.getString("streaming_server_cddrive", "D:").c_str() );
	strcpy( g_settings.streaming_videorate,  configfile.getString("streaming_videorate", "1000").c_str() );
	strcpy( g_settings.streaming_audiorate, configfile.getString("streaming_audiorate", "192").c_str() );
	strcpy( g_settings.streaming_server_startdir, configfile.getString("streaming_server_startdir", "C:/Movies").c_str() );
	g_settings.streaming_transcode_audio = configfile.getInt32( "streaming_transcode_audio", 0 );
	g_settings.streaming_force_transcode_video = configfile.getInt32( "streaming_force_transcode_video", 0 );
	g_settings.streaming_transcode_video_codec = configfile.getInt32( "streaming_transcode_video_codec", 0 );
	g_settings.streaming_force_avi_rawaudio = configfile.getInt32( "streaming_force_avi_rawaudio", 0 );
	g_settings.streaming_resolution = configfile.getInt32( "streaming_resolution", 0 );
	g_settings.streaming_vlc10 = configfile.getInt32( "streaming_vlc10", 0);
	g_settings.streaming_show_tv_in_browser = configfile.getInt32("streaming_show_tv_in_browser", 0);
	g_settings.streaming_allow_multiselect = configfile.getBool("streaming_allow_multiselect", false);
	// end movieplayer

	// OSD
	g_settings.gtx_alpha = configfile.getInt32( "gtx_alpha", 255);
	
	strcpy(g_settings.language, configfile.getString("language", "english").c_str());
	g_settings.help_txt = configfile.getInt32( "help_txt", 1);		//on
	g_settings.help_bar = configfile.getInt32( "help_bar", 0);		//off
	g_settings.menutitle_vfd = configfile.getInt32( "menutitle_vfd", 0);	// off

	// themes
	// nhd2	
	g_settings.menu_Head_alpha = configfile.getInt32( "menu_Head_alpha", 0);
	g_settings.menu_Head_red   = configfile.getInt32( "menu_Head_red", 35);
	g_settings.menu_Head_green = configfile.getInt32( "menu_Head_green", 35);
	g_settings.menu_Head_blue  = configfile.getInt32( "menu_Head_blue", 35);

	g_settings.menu_Head_Text_alpha = configfile.getInt32( "menu_Head_Text_alpha", 0);
	g_settings.menu_Head_Text_red   = configfile.getInt32( "menu_Head_Text_red", 100);
	g_settings.menu_Head_Text_green = configfile.getInt32( "menu_Head_Text_green", 100);
	g_settings.menu_Head_Text_blue  = configfile.getInt32( "menu_Head_Text_blue", 100);

	g_settings.menu_Content_alpha = configfile.getInt32( "menu_Content_alpha", 20);
	g_settings.menu_Content_red   = configfile.getInt32( "menu_Content_red", 50);
	g_settings.menu_Content_green = configfile.getInt32( "menu_Content_green", 50);
	g_settings.menu_Content_blue  = configfile.getInt32( "menu_Content_blue", 50);

	g_settings.menu_Content_Text_alpha = configfile.getInt32( "menu_Content_Text_alpha", 0);
	g_settings.menu_Content_Text_red   = configfile.getInt32( "menu_Content_Text_red", 100);
	g_settings.menu_Content_Text_green = configfile.getInt32( "menu_Content_Text_green", 100);
	g_settings.menu_Content_Text_blue  = configfile.getInt32( "menu_Content_Text_blue", 100);

	g_settings.menu_Content_Selected_alpha = configfile.getInt32( "menu_Content_Selected_alpha", 15);
	g_settings.menu_Content_Selected_red   = configfile.getInt32( "menu_Content_Selected_red", 20);
	g_settings.menu_Content_Selected_green = configfile.getInt32( "menu_Content_Selected_green", 20);
	g_settings.menu_Content_Selected_blue  = configfile.getInt32( "menu_Content_Selected_blue", 20);

	g_settings.menu_Content_Selected_Text_alpha  = configfile.getInt32( "menu_Content_Selected_Text_alpha", 0);
	g_settings.menu_Content_Selected_Text_red    = configfile.getInt32( "menu_Content_Selected_Text_red", 100);
	g_settings.menu_Content_Selected_Text_green  = configfile.getInt32( "menu_Content_Selected_Text_green", 100);
	g_settings.menu_Content_Selected_Text_blue   = configfile.getInt32( "menu_Content_Selected_Text_blue", 100);

	g_settings.menu_Content_inactive_alpha = configfile.getInt32( "menu_Content_inactive_alpha", 20);
	g_settings.menu_Content_inactive_red   = configfile.getInt32( "menu_Content_inactive_red", 50);
	g_settings.menu_Content_inactive_green = configfile.getInt32( "menu_Content_inactive_green", 50);
	g_settings.menu_Content_inactive_blue  = configfile.getInt32( "menu_Content_inactive_blue", 50);

	g_settings.menu_Content_inactive_Text_alpha  = configfile.getInt32( "menu_Content_inactive_Text_alpha", 0);
	g_settings.menu_Content_inactive_Text_red    = configfile.getInt32( "menu_Content_inactive_Text_red", 70);
	g_settings.menu_Content_inactive_Text_green  = configfile.getInt32( "menu_Content_inactive_Text_green", 70);
	g_settings.menu_Content_inactive_Text_blue   = configfile.getInt32( "menu_Content_inactive_Text_blue", 70);

	g_settings.infobar_alpha = configfile.getInt32( "infobar_alpha", 20);
	g_settings.infobar_red   = configfile.getInt32( "infobar_red", 50);
	g_settings.infobar_green = configfile.getInt32( "infobar_green", 50);
	g_settings.infobar_blue  = configfile.getInt32( "infobar_blue", 50);

	g_settings.infobar_Text_alpha = configfile.getInt32( "infobar_Text_alpha", 0);
	g_settings.infobar_Text_red   = configfile.getInt32( "infobar_Text_red", 100);
	g_settings.infobar_Text_green = configfile.getInt32( "infobar_Text_green", 100);
	g_settings.infobar_Text_blue  = configfile.getInt32( "infobar_Text_blue", 100);

	strcpy( g_settings.font_file, configfile.getString( "font_file", FONTDIR"/neutrino.ttf" ).c_str() );

	// menue timing
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		g_settings.timing[i] = configfile.getInt32(locale_real_names[timing_setting_name[i]], default_timing[i]);

	// screen setup
	g_settings.screen_StartX = configfile.getInt32( "screen_StartX", 35 );
	g_settings.screen_StartY = configfile.getInt32( "screen_StartY", 35 );
	g_settings.screen_EndX = configfile.getInt32( "screen_EndX", frameBuffer->getScreenWidth(true) - 35 );
	g_settings.screen_EndY = configfile.getInt32( "screen_EndY", frameBuffer->getScreenHeight(true) - 35 );
	g_settings.screen_width = configfile.getInt32("screen_width", frameBuffer->getScreenWidth(true) );
	g_settings.screen_height = configfile.getInt32("screen_height", frameBuffer->getScreenHeight(true) );	
	// END OSD

	// keysbinding
	strcpy(g_settings.repeat_blocker, configfile.getString("repeat_blocker", "250").c_str());
	strcpy(g_settings.repeat_genericblocker, configfile.getString("repeat_genericblocker", "25").c_str());

	g_settings.key_tvradio_mode = configfile.getInt32( "key_tvradio_mode", CRCInput::RC_mode );
	g_settings.key_channelList_pageup = configfile.getInt32( "key_channelList_pageup",  CRCInput::RC_page_up );
	g_settings.key_channelList_pagedown = configfile.getInt32( "key_channelList_pagedown", CRCInput::RC_page_down );
	g_settings.key_channelList_cancel = configfile.getInt32( "key_channelList_cancel",  CRCInput::RC_home );
	g_settings.key_channelList_reload = configfile.getInt32( "key_channelList_reload",  CRCInput::RC_blue );
	g_settings.key_channelList_addrecord = configfile.getInt32( "key_channelList_addrecord", CRCInput::RC_red );
	g_settings.key_channelList_addremind = configfile.getInt32( "key_channelList_addremind", CRCInput::RC_yellow );
	g_settings.key_list_start = configfile.getInt32( "key_list_start", CRCInput::RC_nokey );
	g_settings.key_list_end = configfile.getInt32( "key_list_end", CRCInput::RC_nokey );
	
	g_settings.key_bouquet_up = configfile.getInt32( "key_bouquet_up",  CRCInput::RC_right);
	g_settings.key_bouquet_down = configfile.getInt32( "key_bouquet_down",  CRCInput::RC_left);

	g_settings.key_quickzap_up = configfile.getInt32( "key_quickzap_up",  CRCInput::RC_up );
	g_settings.key_quickzap_down = configfile.getInt32( "key_quickzap_down",  CRCInput::RC_down );
	g_settings.key_subchannel_up = configfile.getInt32( "key_subchannel_up",  CRCInput::RC_right );
	g_settings.key_subchannel_down = configfile.getInt32( "key_subchannel_down",  CRCInput::RC_left );
	g_settings.key_zaphistory = configfile.getInt32( "key_zaphistory",  CRCInput::RC_home );	
	g_settings.key_lastchannel = configfile.getInt32( "key_lastchannel",  CRCInput::RC_recall );
	
	// pip keys
	g_settings.key_pip = configfile.getInt32("key_pip", CRCInput::RC_pip);
	g_settings.key_pip_subchannel = configfile.getInt32("key_pip_subchannel", CRCInput::RC_pipsubch);

	// mpkeys
	g_settings.mpkey_rewind = configfile.getInt32( "mpkey.rewind", CRCInput::RC_rewind );
	g_settings.mpkey_forward = configfile.getInt32( "mpkey.forward", CRCInput::RC_forward );
	g_settings.mpkey_pause = configfile.getInt32( "mpkey.pause", CRCInput::RC_pause );
	g_settings.mpkey_stop = configfile.getInt32( "mpkey.stop", CRCInput::RC_stop );
	g_settings.mpkey_play = configfile.getInt32( "mpkey.play", CRCInput::RC_play );
	g_settings.mpkey_audio = configfile.getInt32( "mpkey.audio", CRCInput::RC_green );
	g_settings.mpkey_time = configfile.getInt32( "mpkey.time", CRCInput::RC_setup );
	g_settings.mpkey_bookmark = configfile.getInt32( "mpkey.bookmark", CRCInput::RC_blue );
	g_settings.key_timeshift = configfile.getInt32( "key_timeshift", CRCInput::RC_pause );	
	
	// misc keys
	g_settings.key_unlock = configfile.getInt32( "key_unlock", CRCInput::RC_setup );
	g_settings.menu_left_exit = configfile.getInt32( "menu_left_exit", 0 );

	// media keys
	g_settings.key_video = configfile.getInt32( "key_video", CRCInput::RC_video );
	g_settings.key_music = configfile.getInt32( "key_music", CRCInput::RC_music );
	g_settings.key_picture = configfile.getInt32( "key_picture", CRCInput::RC_picture );
	g_settings.key_timelist = configfile.getInt32( "key_timelist", CRCInput::RC_bookmark );
	g_settings.key_net = configfile.getInt32( "key_net", CRCInput::RC_net );
	g_settings.key_video_player = configfile.getInt32( "key_video_player", CRCInput::RC_nokey );
	
        // USERMENU -> in system/settings.h
        //-------------------------------------------
        // this is as the current neutrino usermen
        const char* usermenu_default[SNeutrinoSettings::BUTTON_MAX]={
                "2,3,4,13",                     // RED
                "6",                            // GREEN
                "7",                            // YELLOW
                "12,10,11,14,15",   		// BLUE
#if defined (PLATFORM_GIGABLUE)
		"2",				// F1
		"3",				// F2
		"4",				// F3
		"5",				// F4
#endif
        };
        char txt1[81];
        std::string txt2;
        const char* txt2ptr;

        for(int button = 0; button < SNeutrinoSettings::BUTTON_MAX; button++)
        {
                snprintf(txt1,80,"usermenu_tv_%s_text",usermenu_button_def[button]);
                txt1[80] = 0; // terminate for sure
                g_settings.usermenu_text[button] = configfile.getString(txt1, "" );

                snprintf(txt1,80,"usermenu_tv_%s",usermenu_button_def[button]);
                txt2 = configfile.getString(txt1,usermenu_default[button]);
                txt2ptr = txt2.c_str();

                for( int pos = 0; pos < SNeutrinoSettings::ITEM_MAX; pos++)
                {
                        // find next comma or end of string - if it's not the first round
                        if(pos != 0)
                        {
                                while(*txt2ptr != 0 && *txt2ptr != ',')
                                        txt2ptr++;
                                if(*txt2ptr != 0)
                                        txt2ptr++;
                        }

                        if(*txt2ptr != 0)
                        {
                                g_settings.usermenu[button][pos] = atoi(txt2ptr);  // there is still a string
                                if(g_settings.usermenu[button][pos] >= SNeutrinoSettings::ITEM_MAX)
                                        g_settings.usermenu[button][pos] = 0;
                        }
                        else
                                g_settings.usermenu[button][pos] = 0;     // string empty, fill up with 0

                }
        }
	// end keysbinding

	// audioplayer
	strcpy( g_settings.network_nfs_audioplayerdir, configfile.getString( "network_nfs_audioplayerdir", "/media/sda1/music" ).c_str() );

	g_settings.audioplayer_display = configfile.getInt32("audioplayer_display",(int)CAudioPlayerGui::ARTIST_TITLE);
	g_settings.audioplayer_follow  = configfile.getInt32("audioplayer_follow",0);
	strcpy( g_settings.audioplayer_screensaver, configfile.getString( "audioplayer_screensaver", "1" ).c_str() );
	g_settings.audioplayer_highprio  = configfile.getInt32("audioplayer_highprio",0);
	g_settings.audioplayer_select_title_by_name = configfile.getInt32("audioplayer_select_title_by_name",0);
	g_settings.audioplayer_repeat_on = configfile.getInt32("audioplayer_repeat_on",0);
	g_settings.audioplayer_show_playlist = configfile.getInt32("audioplayer_show_playlist",1);
	g_settings.audioplayer_enable_sc_metadata = configfile.getInt32("audioplayer_enable_sc_metadata",1);
	
	// shoutcast --- not in GUI
	g_settings.shoutcast_dev_id = configfile.getString("shoutcast_dev_id","XXXXXXXXXXXXXXXX");
	// end audioplayer

	// pictureviewer
	strcpy( g_settings.network_nfs_picturedir, configfile.getString( "network_nfs_picturedir", "/media/sda1/picture" ).c_str() );

	strcpy( g_settings.picviewer_slide_time, configfile.getString( "picviewer_slide_time", "10" ).c_str() );
	g_settings.picviewer_scaling = configfile.getInt32("picviewer_scaling", 1 /*(int)CPictureViewer::SIMPLE*/);
	// end pictureviewer

	// misc opts
	g_settings.channel_mode = configfile.getInt32("channel_mode", LIST_MODE_ALL);

	//misc
	g_settings.power_standby = configfile.getInt32( "power_standby", 0);
	g_settings.rotor_swap = configfile.getInt32( "rotor_swap", 0);
	
	// mb preview
	g_settings.mb_preview = configfile.getBool("mb_preview", false);

	g_settings.shutdown_real = configfile.getBool("shutdown_real", true );
	g_settings.shutdown_real_rcdelay = configfile.getBool("shutdown_real_rcdelay", false );
        strcpy(g_settings.shutdown_count, configfile.getString("shutdown_count","0").c_str());
	g_settings.infobar_sat_display   = configfile.getBool("infobar_sat_display"  , true );
	g_settings.infobar_subchan_disp_pos = configfile.getInt32("infobar_subchan_disp_pos"  , 0 );

	g_settings.zap_cycle = configfile.getInt32( "zap_cycle", 1 );
	g_settings.sms_channel = configfile.getInt32( "sms_channel", 0 );

	//timezone
	strcpy(g_settings.timezone, configfile.getString("timezone", "(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Vienna").c_str());
	
	//zapit setup
	g_settings.lastChannelMode = configfile.getInt32("lastChannelMode", 1);		//TV mode
	g_settings.StartChannelTV = configfile.getString("startchanneltv","");
	g_settings.StartChannelRadio = configfile.getString("startchannelradio","");
	g_settings.startchanneltv_id =  configfile.getInt64("startchanneltv_id", 0);
	g_settings.startchannelradio_id =  configfile.getInt64("startchannelradio_id", 0);
	g_settings.startchanneltv_nr =  configfile.getInt32("startchanneltv_nr", 0);
	g_settings.startchannelradio_nr =  configfile.getInt32("startchannelradio_nr", 0);
	g_settings.uselastchannel = configfile.getInt32("uselastchannel" , 1);

	// epg
        g_settings.epg_cache            = configfile.getString("epg_cache_time", "14");
        g_settings.epg_extendedcache    = configfile.getString("epg_extendedcache_time", "360");
        g_settings.epg_old_events       = configfile.getString("epg_old_events", "1");
        g_settings.epg_max_events       = configfile.getString("epg_max_events", "50000");
        g_settings.epg_dir              = configfile.getString("epg_dir", "/media/sda1/epg");
	g_settings.epg_save = configfile.getBool("epg_save", false);

	g_settings.virtual_zap_mode = configfile.getBool("virtual_zap_mode", false);

	g_settings.channellist_epgtext_align_right	= configfile.getBool("channellist_epgtext_align_right", false);
	g_settings.channellist_extended		= configfile.getBool("channellist_extended", true);
	g_settings.make_hd_list = configfile.getInt32("make_hd_list", 0);
	
	//crypticon on channellist
	g_settings.channellist_ca = configfile.getInt32("channellist_ca", 1);

	//Filebrowser
	g_settings.filesystem_is_utf8 = configfile.getBool("filesystem_is_utf8", true );
	g_settings.filebrowser_showrights = configfile.getInt32("filebrowser_showrights", 1);
	g_settings.filebrowser_sortmethod = configfile.getInt32("filebrowser_sortmethod", 0);
	if ((g_settings.filebrowser_sortmethod < 0) || (g_settings.filebrowser_sortmethod >= FILEBROWSER_NUMBER_OF_SORT_VARIANTS))
		g_settings.filebrowser_sortmethod = 0;
	g_settings.filebrowser_denydirectoryleave = configfile.getBool("filebrowser_denydirectoryleave", false);
	
	// radiotext
#if ENABLE_RADIOTEXT	
	g_settings.radiotext_enable = configfile.getBool("radiotext_enable", false);
#endif	
	
	// logos_dir
	g_settings.logos_dir = configfile.getString("logos_dir", "/var/tuxbox/icons/logos");
	
	// vol
	g_settings.volume_pos = configfile.getInt32( "volume_pos", 1);		//top_left
	g_settings.current_volume = configfile.getInt32("current_volume", 100);
	strcpy( g_settings.audio_step,		configfile.getString( "audio_step" , "2" ).c_str() );
	// END MISC OPTS

	// HDD
	g_settings.hdd_sleep = configfile.getInt32( "hdd_sleep", 120);
	g_settings.hdd_noise = configfile.getInt32( "hdd_noise", 254);
	// END HDD

	// SOFT UPDATE
	g_settings.softupdate_mode = configfile.getInt32( "softupdate_mode", 1 );
	strcpy(g_settings.softupdate_url_file, configfile.getString("softupdate_url_file", "/var/etc/update.urls").c_str());
	strcpy(g_settings.softupdate_proxyserver, configfile.getString("softupdate_proxyserver", "" ).c_str());
	strcpy(g_settings.softupdate_proxyusername, configfile.getString("softupdate_proxyusername", "" ).c_str());
	strcpy(g_settings.softupdate_proxypassword, configfile.getString("softupdate_proxypassword", "" ).c_str());
	strcpy( g_settings.update_dir, configfile.getString( "update_dir", "/tmp" ).c_str() );
	// END UPDATE

	// VFD
	for (int i = 0; i < LCD_SETTING_COUNT; i++)
		g_settings.lcd_setting[i] = configfile.getInt32(lcd_setting[i].name, lcd_setting[i].default_value);
	
	strcpy(g_settings.lcd_setting_dim_time, configfile.getString("lcd_dim_time","0").c_str());
	g_settings.lcd_setting_dim_brightness = configfile.getInt32("lcd_dim_brightness", 0);
	// END VFD
	
#if ENABLE_GRAPHLCD
	g_settings.glcd_enable = configfile.getInt32("glcd_enable", 0);
	g_settings.glcd_color_fg = configfile.getInt32("glcd_color_fg", GLCD::cColor::White);
	g_settings.glcd_color_bg = configfile.getInt32("glcd_color_bg", GLCD::cColor::Blue);
	g_settings.glcd_color_bar = configfile.getInt32("glcd_color_bar", GLCD::cColor::Red);
	g_settings.glcd_percent_channel = configfile.getInt32("glcd_percent_channel", 18);
	g_settings.glcd_percent_epg = configfile.getInt32("glcd_percent_epg", 8);
	g_settings.glcd_percent_bar = configfile.getInt32("glcd_percent_bar", 6);
	g_settings.glcd_percent_time = configfile.getInt32("glcd_percent_time", 22);
	g_settings.glcd_mirror_osd = configfile.getInt32("glcd_mirror_osd", 0);
	g_settings.glcd_time_in_standby = configfile.getInt32("glcd_time_in_standby", 0);
	g_settings.glcd_font = configfile.getString("glcd_font", FONTDIR "/neutrino.ttf");
#endif	
	
	//set OSD resolution
#define DEFAULT_X_OFF 35
#define DEFAULT_Y_OFF 35
	if((g_settings.screen_width != (int) frameBuffer->getScreenWidth(true)) || (g_settings.screen_height != (int) frameBuffer->getScreenHeight(true))) 
	{
		g_settings.screen_StartX = DEFAULT_X_OFF;
		g_settings.screen_StartY = DEFAULT_Y_OFF;
		g_settings.screen_EndX = frameBuffer->getScreenWidth(true) - DEFAULT_X_OFF;
		g_settings.screen_EndY = frameBuffer->getScreenHeight(true) - DEFAULT_Y_OFF;
		g_settings.screen_width = frameBuffer->getScreenWidth(true);
		g_settings.screen_height = frameBuffer->getScreenHeight(true);
	}	

	if(configfile.getUnknownKeyQueryedFlag() && (erg==0)) 
	{
		erg = 2;
	}

	if(erg)
		configfile.setModifiedFlag(true);

	return erg;
}

// saveSetup, save the application-settings
void CNeutrinoApp::saveSetup(const char * fname)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::saveSetup\n");
	
	char cfg_key[81];

	// VIDEO
	configfile.setInt32( "video_Mode", g_settings.video_Mode );
	configfile.setInt32( "analog_mode", g_settings.analog_mode );
	
	// hdmi space colour
	configfile.setInt32( "hdmi_color_space", g_settings.hdmi_color_space );
	
	//aspect ratio
	configfile.setInt32( "video_Ratio", g_settings.video_Ratio );
	
	//display format
	configfile.setInt32( "video_Format", g_settings.video_Format );

	// wss
	configfile.setInt32("wss_mode", g_settings.wss_mode);
	
	configfile.setInt32( "contrast", g_settings.contrast);
	configfile.setInt32( "saturation", g_settings.saturation);
	configfile.setInt32( "brightness", g_settings.brightness);
	configfile.setInt32( "tint", g_settings.tint);
	// END VIDEO

	// AUDIO
	configfile.setInt32( "audio_AnalogMode", g_settings.audio_AnalogMode );
	configfile.setBool("audio_DolbyDigital", g_settings.audio_DolbyDigital   );
	configfile.setInt32( "hdmi_dd", g_settings.hdmi_dd);
	
	configfile.setInt32( "avsync", g_settings.avsync);
	
	// ac3 delay
	configfile.setInt32( "ac3_delay", g_settings.ac3_delay);
	
	// pcm delay
	configfile.setInt32( "pcm_delay", g_settings.pcm_delay);
	
	// auto langs/subs/ac3
	configfile.setInt32( "auto_lang", g_settings.auto_lang );
	configfile.setInt32( "auto_subs", g_settings.auto_subs );
	for(int i = 0; i < 3; i++) {
		sprintf(cfg_key, "pref_lang_%d", i);
		configfile.setString(cfg_key, g_settings.pref_lang[i]);
		sprintf(cfg_key, "pref_subs_%d", i);
		configfile.setString(cfg_key, g_settings.pref_subs[i]);
	}
	// END AUDIO

	// PARENTALLOCK
	configfile.setInt32( "parentallock_prompt", g_settings.parentallock_prompt );
	configfile.setInt32( "parentallock_lockage", g_settings.parentallock_lockage );
	configfile.setString( "parentallock_pincode", g_settings.parentallock_pincode );
	// END PARENTALLOCK

	// NETWORK
        configfile.setString( "network_ntpserver", g_settings.network_ntpserver);
        configfile.setString( "network_ntprefresh", g_settings.network_ntprefresh);
        configfile.setBool( "network_ntpenable", g_settings.network_ntpenable);
	
	configfile.setString("ifname", g_settings.ifname);

	for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++) 
	{
		sprintf(cfg_key, "network_nfs_ip_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_ip[i] );
		sprintf(cfg_key, "network_nfs_dir_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_dir[i] );
		sprintf(cfg_key, "network_nfs_local_dir_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_local_dir[i] );
		sprintf(cfg_key, "network_nfs_automount_%d", i);
		configfile.setInt32( cfg_key, g_settings.network_nfs_automount[i]);
		sprintf(cfg_key, "network_nfs_type_%d", i);
		configfile.setInt32( cfg_key, g_settings.network_nfs_type[i]);
		sprintf(cfg_key,"network_nfs_username_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_username[i] );
		sprintf(cfg_key, "network_nfs_password_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_password[i] );
		sprintf(cfg_key, "network_nfs_mount_options1_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_mount_options1[i]);
		sprintf(cfg_key, "network_nfs_mount_options2_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_mount_options2[i]);
		sprintf(cfg_key, "network_nfs_mac_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_mac[i]);
	}
	// END NETWORK

	// RECORDING
	configfile.setString( "network_nfs_recordingdir", g_settings.network_nfs_recordingdir);
	configfile.setInt32( "temp_timeshift", g_settings.temp_timeshift );
	configfile.setInt32( "auto_timeshift", g_settings.auto_timeshift );
	configfile.setInt32( "auto_delete", g_settings.auto_delete );
	configfile.setInt32( "record_hours", g_settings.record_hours );
	configfile.setInt32 ("recording_audio_pids_default"       , g_settings.recording_audio_pids_default);
	configfile.setBool  ("recording_epg_for_filename"         , g_settings.recording_epg_for_filename);
	configfile.setBool  ("recording_save_in_channeldir"       , g_settings.recording_save_in_channeldir);
	
	configfile.setBool  ("recording_zap_on_announce"          , g_settings.recording_zap_on_announce );
	// END RECORDING

	// MOVIEPLAYER
	configfile.setString( "network_nfs_moviedir", g_settings.network_nfs_moviedir);
	
	//streaming
	configfile.setInt32 ( "streaming_type", g_settings.streaming_type );
	configfile.setString( "streaming_server_ip", g_settings.streaming_server_ip );
	configfile.setString( "streaming_server_port", g_settings.streaming_server_port );
	configfile.setString( "streaming_server_cddrive", g_settings.streaming_server_cddrive );
	configfile.setString( "streaming_videorate", g_settings.streaming_videorate );
	configfile.setString( "streaming_audiorate", g_settings.streaming_audiorate );
	configfile.setString( "streaming_server_startdir", g_settings.streaming_server_startdir );
	configfile.setInt32 ( "streaming_transcode_audio", g_settings.streaming_transcode_audio );
	configfile.setInt32 ( "streaming_force_avi_rawaudio", g_settings.streaming_force_avi_rawaudio );
	configfile.setInt32 ( "streaming_force_transcode_video", g_settings.streaming_force_transcode_video );
	configfile.setInt32 ( "streaming_transcode_video_codec", g_settings.streaming_transcode_video_codec );
	configfile.setInt32 ( "streaming_resolution", g_settings.streaming_resolution );
	configfile.setInt32 ( "streaming_vlc10", g_settings.streaming_vlc10 );
	configfile.setInt32 ( "streaming_show_tv_in_browser", g_settings.streaming_show_tv_in_browser);
	configfile.setBool ("streaming_allow_multiselect", g_settings.streaming_allow_multiselect);
	// END MOVIEPLAYER

	// OSD
	configfile.setInt32( "gtx_alpha", g_settings.gtx_alpha);

	configfile.setString("language", g_settings.language);

	// theme
	configfile.setInt32( "menu_Head_alpha", g_settings.menu_Head_alpha );
	configfile.setInt32( "menu_Head_red", g_settings.menu_Head_red );
	configfile.setInt32( "menu_Head_green", g_settings.menu_Head_green );
	configfile.setInt32( "menu_Head_blue", g_settings.menu_Head_blue );

	configfile.setInt32( "menu_Head_Text_alpha", g_settings.menu_Head_Text_alpha );
	configfile.setInt32( "menu_Head_Text_red", g_settings.menu_Head_Text_red );
	configfile.setInt32( "menu_Head_Text_green", g_settings.menu_Head_Text_green );
	configfile.setInt32( "menu_Head_Text_blue", g_settings.menu_Head_Text_blue );

	configfile.setInt32( "menu_Content_alpha", g_settings.menu_Content_alpha );
	configfile.setInt32( "menu_Content_red", g_settings.menu_Content_red );
	configfile.setInt32( "menu_Content_green", g_settings.menu_Content_green );
	configfile.setInt32( "menu_Content_blue", g_settings.menu_Content_blue );

	configfile.setInt32( "menu_Content_Text_alpha", g_settings.menu_Content_Text_alpha );
	configfile.setInt32( "menu_Content_Text_red", g_settings.menu_Content_Text_red );
	configfile.setInt32( "menu_Content_Text_green", g_settings.menu_Content_Text_green );
	configfile.setInt32( "menu_Content_Text_blue", g_settings.menu_Content_Text_blue );

	configfile.setInt32( "menu_Content_Selected_alpha", g_settings.menu_Content_Selected_alpha );
	configfile.setInt32( "menu_Content_Selected_red", g_settings.menu_Content_Selected_red );
	configfile.setInt32( "menu_Content_Selected_green", g_settings.menu_Content_Selected_green );
	configfile.setInt32( "menu_Content_Selected_blue", g_settings.menu_Content_Selected_blue );

	configfile.setInt32( "menu_Content_Selected_Text_alpha", g_settings.menu_Content_Selected_Text_alpha );
	configfile.setInt32( "menu_Content_Selected_Text_red", g_settings.menu_Content_Selected_Text_red );
	configfile.setInt32( "menu_Content_Selected_Text_green", g_settings.menu_Content_Selected_Text_green );
	configfile.setInt32( "menu_Content_Selected_Text_blue", g_settings.menu_Content_Selected_Text_blue );

	configfile.setInt32( "menu_Content_inactive_alpha", g_settings.menu_Content_inactive_alpha );
	configfile.setInt32( "menu_Content_inactive_red", g_settings.menu_Content_inactive_red );
	configfile.setInt32( "menu_Content_inactive_green", g_settings.menu_Content_inactive_green );
	configfile.setInt32( "menu_Content_inactive_blue", g_settings.menu_Content_inactive_blue );

	configfile.setInt32( "menu_Content_inactive_Text_alpha", g_settings.menu_Content_inactive_Text_alpha );
	configfile.setInt32( "menu_Content_inactive_Text_red", g_settings.menu_Content_inactive_Text_red );
	configfile.setInt32( "menu_Content_inactive_Text_green", g_settings.menu_Content_inactive_Text_green );
	configfile.setInt32( "menu_Content_inactive_Text_blue", g_settings.menu_Content_inactive_Text_blue );

	configfile.setInt32( "infobar_alpha", g_settings.infobar_alpha );
	configfile.setInt32( "infobar_red", g_settings.infobar_red );
	configfile.setInt32( "infobar_green", g_settings.infobar_green );
	configfile.setInt32( "infobar_blue", g_settings.infobar_blue );

	configfile.setInt32( "infobar_Text_alpha", g_settings.infobar_Text_alpha );
	configfile.setInt32( "infobar_Text_red", g_settings.infobar_Text_red );
	configfile.setInt32( "infobar_Text_green", g_settings.infobar_Text_green );
	configfile.setInt32( "infobar_Text_blue", g_settings.infobar_Text_blue );

	configfile.setInt32( "screen_StartX", g_settings.screen_StartX );
	configfile.setInt32( "screen_StartY", g_settings.screen_StartY );
	configfile.setInt32( "screen_EndX", g_settings.screen_EndX );
	configfile.setInt32( "screen_EndY", g_settings.screen_EndY );
	configfile.setInt32( "screen_width", g_settings.screen_width);
	configfile.setInt32( "screen_height", g_settings.screen_height);

	configfile.setString("font_file", g_settings.font_file);

	// menue timing
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		configfile.setInt32(locale_real_names[timing_setting_name[i]], g_settings.timing[i]);
	
	configfile.setInt32( "help_txt", g_settings.help_txt);
	configfile.setInt32( "help_bar", g_settings.help_bar);
	configfile.setInt32( "menutitle_vfd", g_settings.menutitle_vfd);
	// END OSD

	// KEYS
	configfile.setString( "repeat_blocker", g_settings.repeat_blocker );
	configfile.setString( "repeat_genericblocker", g_settings.repeat_genericblocker );

	configfile.setInt32( "key_tvradio_mode", g_settings.key_tvradio_mode );

	configfile.setInt32( "key_channelList_pageup", g_settings.key_channelList_pageup );
	configfile.setInt32( "key_channelList_pagedown", g_settings.key_channelList_pagedown );
	configfile.setInt32( "key_channelList_cancel", g_settings.key_channelList_cancel );
	configfile.setInt32( "key_channelList_reload", g_settings.key_channelList_reload );
	configfile.setInt32( "key_channelList_addrecord", g_settings.key_channelList_addrecord );
	configfile.setInt32( "key_channelList_addremind", g_settings.key_channelList_addremind );

	configfile.setInt32( "key_quickzap_up", g_settings.key_quickzap_up );
	configfile.setInt32( "key_quickzap_down", g_settings.key_quickzap_down );
	configfile.setInt32( "key_bouquet_up", g_settings.key_bouquet_up );
	configfile.setInt32( "key_bouquet_down", g_settings.key_bouquet_down );
	configfile.setInt32( "key_subchannel_up", g_settings.key_subchannel_up );
	configfile.setInt32( "key_subchannel_down", g_settings.key_subchannel_down );
	configfile.setInt32( "key_zaphistory", g_settings.key_zaphistory );
	configfile.setInt32( "key_lastchannel", g_settings.key_lastchannel );

	configfile.setInt32( "key_list_start", g_settings.key_list_start );
	configfile.setInt32( "key_list_end", g_settings.key_list_end );
	
	// mp keys
	configfile.setInt32( "mpkey.rewind", g_settings.mpkey_rewind );
	configfile.setInt32( "mpkey.forward", g_settings.mpkey_forward );
	configfile.setInt32( "mpkey.pause", g_settings.mpkey_pause );
	configfile.setInt32( "mpkey.stop", g_settings.mpkey_stop );
	configfile.setInt32( "mpkey.play", g_settings.mpkey_play );
	configfile.setInt32( "mpkey.audio", g_settings.mpkey_audio );
	configfile.setInt32( "mpkey.time", g_settings.mpkey_time );
	configfile.setInt32( "mpkey.bookmark", g_settings.mpkey_bookmark );
	configfile.setInt32( "key_timeshift", g_settings.key_timeshift );
	
	// misc keys
	configfile.setInt32( "menu_left_exit", g_settings.menu_left_exit );
	configfile.setInt32( "key_unlock", g_settings.key_unlock );
	
	// media keys
	configfile.setInt32( "key_video", g_settings.key_video );
	configfile.setInt32( "key_music", g_settings.key_music );
	configfile.setInt32( "key_picture", g_settings.key_picture );
	configfile.setInt32( "key_timelist", g_settings.key_timelist );
	configfile.setInt32( "key_net", g_settings.key_net );
	configfile.setInt32( "key_video_player", g_settings.key_video_player );

        // USERMENU
        char txt1[81];
        char txt2[81];
        for(int button = 0; button < SNeutrinoSettings::BUTTON_MAX; button++) 
	{
                snprintf(txt1,80,"usermenu_tv_%s_text",usermenu_button_def[button]);
                txt1[80] = 0; // terminate for sure
                configfile.setString(txt1,g_settings.usermenu_text[button]);

                char* txt2ptr = txt2;
                snprintf(txt1,80,"usermenu_tv_%s",usermenu_button_def[button]);

                for(int pos = 0; pos < SNeutrinoSettings::ITEM_MAX; pos++) 
		{
                        if( g_settings.usermenu[button][pos] != 0) 
			{
                                if(pos != 0)
                                        *txt2ptr++ = ',';
                                txt2ptr += snprintf(txt2ptr,80,"%d",g_settings.usermenu[button][pos]);
                        }
                }
                configfile.setString(txt1,txt2);
        }
	// END KEYS

	// AUDIOPLAYER
	configfile.setString( "network_nfs_audioplayerdir", g_settings.network_nfs_audioplayerdir);

	configfile.setInt32( "audioplayer_display", g_settings.audioplayer_display );
	configfile.setInt32( "audioplayer_follow", g_settings.audioplayer_follow );
	configfile.setString( "audioplayer_screensaver", g_settings.audioplayer_screensaver );
	configfile.setInt32( "audioplayer_highprio", g_settings.audioplayer_highprio );
	configfile.setInt32( "audioplayer_select_title_by_name", g_settings.audioplayer_select_title_by_name );
	configfile.setInt32( "audioplayer_repeat_on", g_settings.audioplayer_repeat_on );
	configfile.setInt32( "audioplayer_show_playlist", g_settings.audioplayer_show_playlist );
	configfile.setInt32( "audioplayer_enable_sc_metadata", g_settings.audioplayer_enable_sc_metadata );
	
	//shoutcast
	configfile.setString( "shoutcast_dev_id", g_settings.shoutcast_dev_id );
	// END AUDIOPLAYER

	// PICVIEWER
	configfile.setString( "network_nfs_picturedir", g_settings.network_nfs_picturedir);

	configfile.setString( "picviewer_slide_time", g_settings.picviewer_slide_time );
	configfile.setInt32( "picviewer_scaling", g_settings.picviewer_scaling );
	// END PICVIEWER

	// MISC OPTS
	configfile.setBool("shutdown_real", g_settings.shutdown_real        );
	configfile.setBool("shutdown_real_rcdelay", g_settings.shutdown_real_rcdelay);
	configfile.setString("shutdown_count"           , g_settings.shutdown_count);

	configfile.setBool("infobar_sat_display", g_settings.infobar_sat_display  );
	configfile.setInt32("infobar_subchan_disp_pos", g_settings.infobar_subchan_disp_pos  );
	
	//crypticon channellist
	configfile.setInt32("channellist_ca", g_settings.channellist_ca);

	configfile.setBool("channellist_epgtext_align_right", g_settings.channellist_epgtext_align_right);
	configfile.setBool("channellist_extended", g_settings.channellist_extended);
	configfile.setInt32("make_hd_list", g_settings.make_hd_list);

	configfile.setString("timezone", g_settings.timezone);

	// epg
	configfile.setBool("epg_save", g_settings.epg_save);
        configfile.setString("epg_cache_time", g_settings.epg_cache );
        configfile.setString("epg_extendedcache_time", g_settings.epg_extendedcache);
        configfile.setString("epg_old_events", g_settings.epg_old_events );
        configfile.setString("epg_max_events", g_settings.epg_max_events );
        configfile.setString("epg_dir", g_settings.epg_dir);

	//filebrowser
	configfile.setBool  ("filesystem_is_utf8", g_settings.filesystem_is_utf8);
	configfile.setInt32("filebrowser_showrights", g_settings.filebrowser_showrights);
	configfile.setInt32("filebrowser_sortmethod", g_settings.filebrowser_sortmethod);
	configfile.setBool("filebrowser_denydirectoryleave", g_settings.filebrowser_denydirectoryleave);

	//
	configfile.setInt32( "channel_mode", g_settings.channel_mode );

	//misc
	configfile.setInt32( "power_standby", g_settings.power_standby);

	configfile.setInt32( "rotor_swap", g_settings.rotor_swap);
	
	configfile.setBool("mb_preview", g_settings.mb_preview);

	configfile.setInt32( "zap_cycle", g_settings.zap_cycle );
	configfile.setInt32( "sms_channel", g_settings.sms_channel );

	configfile.setBool("virtual_zap_mode", g_settings.virtual_zap_mode);
	
	//zapit setup
	configfile.setInt32("lastChannelMode", g_settings.lastChannelMode);
	configfile.setString("startchanneltv", g_settings.StartChannelTV );
	configfile.setString("startchannelradio", g_settings.StartChannelRadio );
	configfile.setInt64("startchanneltv_id", g_settings.startchanneltv_id);
	configfile.setInt64("startchannelradio_id", g_settings.startchannelradio_id);
	configfile.setInt32("startchanneltv_nr", g_settings.startchanneltv_nr);
	configfile.setInt32("startchannelradio_nr", g_settings.startchannelradio_nr);
	configfile.setInt32("uselastchannel", g_settings.uselastchannel);
	
	// radiotext
#if ENABLE_RADIOTEXT	
	configfile.setBool("radiotext_enable", g_settings.radiotext_enable);
#endif	
	
	// logos_dir
	configfile.setString("logos_dir", g_settings.logos_dir);
	
	// vol
	configfile.setInt32( "volume_pos", g_settings.volume_pos);
	configfile.setInt32( "current_volume", g_settings.current_volume );
	configfile.setString( "audio_step"	, g_settings.audio_step);
	// END MISC OPTS

	// HDD
	configfile.setInt32( "hdd_sleep", g_settings.hdd_sleep);
	configfile.setInt32( "hdd_noise", g_settings.hdd_noise);
	// END HDD

	// SOFT UPDATE
	configfile.setString("update_dir", g_settings.update_dir);
	configfile.setInt32 ("softupdate_mode", g_settings.softupdate_mode );
	configfile.setString("softupdate_url_file", g_settings.softupdate_url_file );
	configfile.setString("softupdate_proxyserver", g_settings.softupdate_proxyserver );
	configfile.setString("softupdate_proxyusername", g_settings.softupdate_proxyusername );
	configfile.setString("softupdate_proxypassword", g_settings.softupdate_proxypassword );
	// END UPDATE

	// VFD 
	for (int i = 0; i < LCD_SETTING_COUNT; i++)
		configfile.setInt32(lcd_setting[i].name, g_settings.lcd_setting[i]);
	
	configfile.setString("lcd_dim_time", g_settings.lcd_setting_dim_time);
	configfile.setInt32("lcd_dim_brightness", g_settings.lcd_setting_dim_brightness);
	// END VFD
	
#if ENABLE_GRAPHLCD
	configfile.setInt32("glcd_enable", g_settings.glcd_enable);
	configfile.setInt32("glcd_color_fg", g_settings.glcd_color_fg);
	configfile.setInt32("glcd_color_bg", g_settings.glcd_color_bg);
	configfile.setInt32("glcd_color_bar", g_settings.glcd_color_bar);
	configfile.setInt32("glcd_percent_channel", g_settings.glcd_percent_channel);
	configfile.setInt32("glcd_percent_epg", g_settings.glcd_percent_epg);
	configfile.setInt32("glcd_percent_bar", g_settings.glcd_percent_bar);
	configfile.setInt32("glcd_percent_time", g_settings.glcd_percent_time);
	configfile.setInt32("glcd_mirror_osd", g_settings.glcd_mirror_osd);
	configfile.setInt32("glcd_time_in_standby", g_settings.glcd_time_in_standby);
	configfile.setString("glcd_font", g_settings.glcd_font);
#endif	

	if(strcmp(fname, NEUTRINO_SETTINGS_FILE))
		configfile.saveConfig(fname);

	else if (configfile.getModifiedFlag())
	{
		configfile.saveConfig(fname);
		configfile.setModifiedFlag(false);
	}
}

// firstChannel, get the initial channe
void CNeutrinoApp::firstChannel()
{
	g_Zapit->getLastChannel(firstchannel.channelNumber, firstchannel.mode);
}

// CNeutrinoApp -  channelsInit, get the Channellist from zapit
void CNeutrinoApp::channelsInit(bool bOnly)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::channelsInit: Creating channels lists...\n");

	const char * fav_bouquetname = g_Locale->getText(LOCALE_FAVORITES_BOUQUETNAME);

	if(g_bouquetManager->existsUBouquet(fav_bouquetname, true) == -1)
		g_bouquetManager->addBouquet(fav_bouquetname, true, true);

	if(TVallList) 
		delete TVallList;

	if(RADIOallList) 
		delete RADIOallList;

	if(TVbouquetList) 
		delete TVbouquetList;

	if(TVsatList) 
		delete TVsatList;

	if(TVfavList) 
		delete TVfavList;

	if(RADIObouquetList) 
		delete RADIObouquetList;

	if(RADIOsatList) 
		delete RADIOsatList;

	if(RADIOfavList) 
		delete RADIOfavList;

	if(TVchannelList) 
		delete TVchannelList;

	if(RADIOchannelList) 
		delete RADIOchannelList;

	TVchannelList = new CChannelList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));
	RADIOchannelList = new CChannelList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));

	TVbouquetList = new CBouquetList(g_Locale->getText(LOCALE_CHANNELLIST_PROVS));
	TVbouquetList->orgChannelList = TVchannelList;

	TVsatList = new CBouquetList(g_Locale->getText(LOCALE_CHANNELLIST_SATS));
	TVsatList->orgChannelList = TVchannelList;

	TVfavList = new CBouquetList(g_Locale->getText(LOCALE_CHANNELLIST_FAVS));
	TVfavList->orgChannelList = TVchannelList;

	RADIObouquetList = new CBouquetList(g_Locale->getText(LOCALE_CHANNELLIST_PROVS));
	RADIObouquetList->orgChannelList = RADIOchannelList;

	RADIOsatList = new CBouquetList(g_Locale->getText(LOCALE_CHANNELLIST_SATS));
	RADIOsatList->orgChannelList = RADIOchannelList;

	RADIOfavList = new CBouquetList(g_Locale->getText(LOCALE_CHANNELLIST_FAVS));
	RADIOfavList->orgChannelList = RADIOchannelList;

	uint32_t i;
	i = 1;

	int tvi = 0, ri = 0, hi = 0;
	
	// hd bouquet
	CBouquet * hdBouquet;
	if(g_settings.make_hd_list)
		hdBouquet = new CBouquet(0, (char *) "HD", 0);

	for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
	{
		if (it->second.getServiceType() == ST_DIGITAL_TELEVISION_SERVICE) 
		{
			TVchannelList->putChannel(&(it->second));
			tvi++;

			if(it->second.isHD()) 
			{
				if(g_settings.make_hd_list)
					hdBouquet->channelList->addChannel(&(it->second));
				hi++;
			}
		}
		else if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE) 
		{
			RADIOchannelList->putChannel(&(it->second));
			ri++;
		}
	}
	
	if(g_settings.make_hd_list)
		hdBouquet->channelList->SortSat();

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::channelsInit: got %d TV (%d is HD) and %d RADIO channels\n", tvi, hi, ri); fflush(stdout);

	CBouquet * tmp;

	// tv all list
	TVallList = new CBouquetList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));
	tmp = TVallList->addBouquet(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));
	*(tmp->channelList) = *TVchannelList;
	tmp->channelList->SortAlpha();
	TVallList->orgChannelList = TVchannelList;

	// radio all list
	RADIOallList = new CBouquetList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));
	tmp = RADIOallList->addBouquet(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));
	*(tmp->channelList) = *RADIOchannelList;
	tmp->channelList->SortAlpha();
	RADIOallList->orgChannelList = RADIOchannelList;

	int bnum;
	sat_iterator_t sit;
	for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		tvi = 0, ri = 0;
		CBouquet * tmp1 = TVsatList->addBouquet(sit->second.name.c_str());
		CBouquet * tmp2 = RADIOsatList->addBouquet(sit->second.name.c_str());

		for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
		{
			if(it->second.getSatellitePosition() == sit->first) 
			{
				if (it->second.getServiceType() == ST_DIGITAL_TELEVISION_SERVICE) 
				{
					tmp1->channelList->addChannel(&(it->second));
					tvi++;
				}
				else if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE) 
				{
					tmp2->channelList->addChannel(&(it->second));
					ri++;
				}
			}
		}

		if(tvi)
			tmp1->channelList->SortAlpha();
		else
			TVsatList->deleteBouquet(tmp1);

		if(ri)
			tmp2->channelList->SortAlpha();
		else
			RADIOsatList->deleteBouquet(tmp2);

		if(tvi || ri)
			printf("CNeutrinoApp::channelsInit: created %s with %d TV and %d RADIO channels\n", sit->second.name.c_str(), tvi, ri);
	}

	// tv fav list
	bnum = 0;
	for (i = 0; i < g_bouquetManager->Bouquets.size(); i++) 
	{
		if (!g_bouquetManager->Bouquets[i]->bHidden && !g_bouquetManager->Bouquets[i]->tvChannels.empty())
		{
			CBouquet* tmp;
			if(g_bouquetManager->Bouquets[i]->bUser) 
				tmp = TVfavList->addBouquet(g_bouquetManager->Bouquets[i]);
			else
				tmp = TVbouquetList->addBouquet(g_bouquetManager->Bouquets[i]);

			ZapitChannelList * channels = &(g_bouquetManager->Bouquets[i]->tvChannels);
			tmp->channelList->setSize(channels->size());
			for(int j = 0; j < (int) channels->size(); j++) 
			{
				tmp->channelList->addChannel((*channels)[j]);
			}
			bnum++;
		}
	}
	
	if(g_settings.make_hd_list)
		TVfavList->Bouquets.push_back(hdBouquet);
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::channelsInit: got %d TV bouquets\n", bnum); fflush(stdout);

	// radio fav list
	bnum = 0;
	for (i = 0; i < g_bouquetManager->Bouquets.size(); i++) 
	{	
		if (!g_bouquetManager->Bouquets[i]->bHidden && !g_bouquetManager->Bouquets[i]->radioChannels.empty() )
		{
			CBouquet * tmp;
			if(g_bouquetManager->Bouquets[i]->bUser) 
				tmp = RADIOfavList->addBouquet(g_bouquetManager->Bouquets[i]->Name.c_str(), i, g_bouquetManager->Bouquets[i]->bLocked);
			else
				tmp = RADIObouquetList->addBouquet(g_bouquetManager->Bouquets[i]->Name.c_str(), i, g_bouquetManager->Bouquets[i]->bLocked);

			ZapitChannelList* channels = &(g_bouquetManager->Bouquets[i]->radioChannels);
			tmp->channelList->setSize(channels->size());
			for(int j = 0; j < (int) channels->size(); j++) 
			{
				tmp->channelList->addChannel((*channels)[j]);
			}
			bnum++;
		}
	}
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::channelsInit: got %d RADIO bouquets\n", bnum); fflush(stdout);

	SetChannelMode( g_settings.channel_mode);

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::channelsInit: All bouquets-channels received\n");
}

void CNeutrinoApp::SetChannelMode(int newmode)
{
	const char *aLISTMODE[] = {
		"LIST_MODE_FAV",
		"LIST_MODE_PROV",
		"LIST_MODE_SAT",
		"LIST_MODE_ALL"
	};
	
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::SetChannelMode %s\n", aLISTMODE[newmode]);

	if(mode == mode_radio)
		channelList = RADIOchannelList;
	else
		channelList = TVchannelList;

	switch(newmode) 
	{
		case LIST_MODE_FAV:
			if(mode == mode_radio) 
			{
				bouquetList = RADIOfavList;
			} 
			else 
			{
				bouquetList = TVfavList;
			}
			break;

		case LIST_MODE_SAT:
			if(mode == mode_radio) 
			{
				bouquetList = RADIOsatList;
			} 
			else 
			{
				bouquetList = TVsatList;
			}
			break;

		case LIST_MODE_ALL:
			if(mode == mode_radio) 
			{
				bouquetList = RADIOallList;
			} 
			else 
			{
				bouquetList = TVallList;
			}
			break;

		default:
		case LIST_MODE_PROV:
			if(mode == mode_radio) 
			{
				bouquetList = RADIObouquetList;
			} 
			else 
			{
				bouquetList = TVbouquetList;
			}
			break;
	}

	g_settings.channel_mode = newmode;
}

// CNeutrinoApp -  run, the main runloop
void CNeutrinoApp::CmdParser(int argc, char **argv)
{
        global_argv = new char *[argc + 1];
	
        for (int i = 0; i < argc; i++)
                global_argv[i] = argv[i];
	
        global_argv[argc] = NULL;

	for(int x = 1; x < argc; x++) 
	{
		if (((!strcmp(argv[x], "-v")) || (!strcmp(argv[x], "--verbose"))) && (x + 1 < argc)) 
		{
			int dl = atoi(argv[x + 1]);
			dprintf(DEBUG_NORMAL, "set debuglevel: %d\n", dl);
			setDebugLevel(dl);
			x++;
		}
		else if ((!strcmp(argv[x], "-sd"))) 
		{
			dprintf(DEBUG_NORMAL, "Sectionsd debug enabled\n");
			sections_debug = 1;
			x++;
		}
		else 
		{
			dprintf(DEBUG_NORMAL, "Usage: neutrino [-sd] [-v | --verbose 0..3]\n");
			exit(1);
		}
	}
}

// setup the framebuffer
void CNeutrinoApp::SetupFrameBuffer()
{
	frameBuffer->init();
	
	if(frameBuffer->setMode() ) 
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::SetupFrameBuffer: Error while setting framebuffer mode\n");
		exit(-1);
	}	
}

// setup fonts
void CNeutrinoApp::SetupFonts()
{
	const char * style[3];

	if (g_fontRenderer != NULL)
		delete g_fontRenderer;

	g_fontRenderer = new FBFontRenderClass();

	if(font.filename != NULL)
		free((void *)font.filename);

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::SetupFonts: settings font file %s\n", g_settings.font_file);

	if(access(g_settings.font_file, F_OK)) 
	{
		if(!access(FONTDIR"/neutrino.ttf", F_OK))
		{
			font.filename = strdup(FONTDIR"/neutrino.ttf");
			strcpy(g_settings.font_file, font.filename);
		}
		else
		{
			  fprintf( stderr,"CNeutrinoApp::SetupFonts: font file [%s] not found\n neutrino exit\n", FONTDIR"/neutrino.ttf");
			  _exit(0);
		}
	}
	else
	{
		font.filename = strdup(g_settings.font_file);
	}

	style[0] = g_fontRenderer->AddFont(font.filename);

	if(font.name != NULL)
		free((void *)font.name);

	font.name = strdup(g_fontRenderer->getFamily(font.filename).c_str());

	dprintf(DEBUG_DEBUG, "CNeutrinoApp::SetupFonts: font family %s\n", font.name);

	style[1] = "Bold Regular";

	g_fontRenderer->AddFont(font.filename, true);  // make italics
	style[2] = "Italic";

	// set neutrino font
	for (int i = 0; i < FONT_TYPE_COUNT; i++)
	{
		if(g_Font[i] != NULL) 
			delete g_Font[i];

		g_Font[i] = g_fontRenderer->getFont(font.name, style[neutrino_font[i].style], configfile.getInt32(locale_real_names[neutrino_font[i].name], neutrino_font[i].defaultsize) + neutrino_font[i].size_offset * font.size_offset);
	}

	// set signal font
	g_SignalFont = g_fontRenderer->getFont(font.name, style[signal_font.style], signal_font.defaultsize + signal_font.size_offset * font.size_offset);

	// recalculate infobar position
	if (g_InfoViewer)
		g_InfoViewer->start();
}

// setup the menu timeouts
void CNeutrinoApp::SetupTiming()
{
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		sprintf(g_settings.timing_string[i], "%d", g_settings.timing[i]);
};

bool sectionsd_getActualEPGServiceKey(const t_channel_id uniqueServiceKey, CEPGData * epgdata);
bool sectionsd_getEPGid(const event_id_t epgID, const time_t startzeit, CEPGData * epgdata);

// start auto record (permanent/temp timeshift)
int startAutoRecord(bool addTimer)
{
	CTimerd::RecordingInfo eventinfo;

	if(CNeutrinoApp::getInstance()->recordingstatus || !CVCRControl::getInstance()->isDeviceRegistered() || (g_settings.recording_type != RECORDING_FILE))
		return 0;

	eventinfo.channel_id = live_channel_id;
	CEPGData epgData;
	
	if (sectionsd_getActualEPGServiceKey(live_channel_id&0xFFFFFFFFFFFFULL, &epgData ))
	{
		eventinfo.epgID = epgData.eventID;
		eventinfo.epg_starttime = epgData.epg_times.startzeit;
		strncpy(eventinfo.epgTitle, epgData.title.c_str(), EPG_TITLE_MAXLEN-1);
		eventinfo.epgTitle[EPG_TITLE_MAXLEN-1]=0;
	}
	else 
	{
		eventinfo.epgID = 0;
		eventinfo.epg_starttime = 0;
		strcpy(eventinfo.epgTitle, "");
	}

	eventinfo.apids = TIMERD_APIDS_CONF;
	
	dprintf(DEBUG_NORMAL, "startAutoRecord: dir %s\n", timeshiftDir);

	(static_cast<CVCRControl::CFileDevice*>(recordingdevice))->Directory = timeshiftDir;

	autoshift = 1;
	CNeutrinoApp::getInstance()->recordingstatus = 1;
	CNeutrinoApp::getInstance()->timeshiftstatus = 1;

	if( CVCRControl::getInstance()->Record(&eventinfo) == false ) 
	{
		CNeutrinoApp::getInstance()->recordingstatus = 0;
		autoshift = 0;
		
		CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false );	
	}
	else if (addTimer) 
	{
		time_t now = time(NULL);
		CNeutrinoApp::getInstance()->recording_id = g_Timerd->addImmediateRecordTimerEvent(eventinfo.channel_id, now, now+g_settings.record_hours*60*60, eventinfo.epgID, eventinfo.epg_starttime, eventinfo.apids);
	}

	CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);

	return 0;
}

// stop auto record
void stopAutoRecord()
{
	if(autoshift && CNeutrinoApp::getInstance()->recordingstatus) 
	{
		dprintf(DEBUG_NORMAL, "stopAutoRecord: autoshift, recordingstatus %d, stopping ...\n", CNeutrinoApp::getInstance()->recordingstatus);

		CVCRControl::getInstance()->Stop();
		autoshift = false;
		
		if(CNeutrinoApp::getInstance()->recording_id) 
		{
			g_Timerd->stopTimerEvent(CNeutrinoApp::getInstance()->recording_id);
			CNeutrinoApp::getInstance()->recording_id = 0;
		}
	} 
	else if(shift_timer)  
	{
		g_RCInput->killTimer (shift_timer);
		shift_timer = 0;
	}
	
	CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false);	
}

// do gui-record
bool CNeutrinoApp::doGuiRecord(char * preselectedDir, bool addTimer)
{
	CTimerd::RecordingInfo eventinfo;
	bool refreshGui = false;

	if(CVCRControl::getInstance()->isDeviceRegistered()) 
	{
		// stop auto record
		if(autoshift) 
		{
			stopAutoRecord();
		}

		//
		if(recordingstatus == 1) 
		{
			puts("CNeutrinoApp::doGuiRecord: executing " NEUTRINO_RECORDING_START_SCRIPT ".");

			if (system(NEUTRINO_RECORDING_START_SCRIPT) != 0)
				perror(NEUTRINO_RECORDING_START_SCRIPT " failed");

			//CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);

			// get EPG info
			eventinfo.channel_id = live_channel_id;
			CEPGData epgData;

			if (sectionsd_getActualEPGServiceKey(live_channel_id&0xFFFFFFFFFFFFULL, &epgData ))
			{
				eventinfo.epgID = epgData.eventID;
				eventinfo.epg_starttime = epgData.epg_times.startzeit;
				strncpy(eventinfo.epgTitle, epgData.title.c_str(), EPG_TITLE_MAXLEN-1);
				eventinfo.epgTitle[EPG_TITLE_MAXLEN-1]=0;
			}
			else 
			{
				eventinfo.epgID = 0;
				eventinfo.epg_starttime = 0;
				strcpy(eventinfo.epgTitle, "");
			}

			eventinfo.apids = TIMERD_APIDS_CONF;

			bool doRecord = true;

			// rec dir
			if (g_settings.recording_type == RECORDING_FILE) //FILE MODE
			{
				strcpy(recDir, (preselectedDir != NULL) ? preselectedDir : g_settings.network_nfs_recordingdir);
				
				(static_cast<CVCRControl::CFileDevice*>(recordingdevice))->Directory = recDir;
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::doGuiRecord: start record to dir %s\n", recDir);
			}

			// start to record baby
			if( !doRecord || (CVCRControl::getInstance()->Record(&eventinfo) == false ) ) 
			{
				recordingstatus = 0;

				if(doRecord)
					return true;// try to refresh gui if record was not ok ?

				return refreshGui;
			}
			else if (addTimer) // add timer
			{
				time_t now = time(NULL);
				recording_id = g_Timerd->addImmediateRecordTimerEvent(eventinfo.channel_id, now, now+g_settings.record_hours*60*60, eventinfo.epgID, eventinfo.epg_starttime, eventinfo.apids);
			}
		} 
		else 
		{
			g_Timerd->stopTimerEvent(recording_id);
			
			//CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);
			
			startNextRecording();
		}
		
		CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);

		return refreshGui;
	}
	else
		puts("CNeutrinoApp::doGuiRecord: no recording devices");


	return false;
}

#define LCD_UPDATE_TIME_RADIO_MODE (6 * 1000 * 1000)
#define LCD_UPDATE_TIME_TV_MODE (60 * 1000 * 1000)

// send sectionsd config
void CNeutrinoApp::SendSectionsdConfig(void)
{
        CSectionsdClient::epg_config config;
	
        config.scanMode                 = scanSettings->scanSectionsd;
        config.epg_cache                = atoi(g_settings.epg_cache.c_str());
        config.epg_old_events           = atoi(g_settings.epg_old_events.c_str());
        config.epg_max_events           = atoi(g_settings.epg_max_events.c_str());
        config.epg_extendedcache        = atoi(g_settings.epg_extendedcache.c_str());
        config.epg_dir                  = g_settings.epg_dir;
        config.network_ntpserver        = g_settings.network_ntpserver;
        config.network_ntprefresh       = atoi(g_settings.network_ntprefresh.c_str());
        config.network_ntpenable        = g_settings.network_ntpenable;
	
        g_Sectionsd->setConfig(config);
}

// init zapper
void CNeutrinoApp::InitZapper()
{
	// start infobar
	g_InfoViewer->start();
	
	// send sectionsd config
	SendSectionsdConfig();

	// read saved epg
	struct stat my_stat;
	
	if (g_settings.epg_save)
	{
		if(stat(g_settings.epg_dir.c_str(), &my_stat) == 0)
			g_Sectionsd->readSIfromXML(g_settings.epg_dir.c_str());
	}

	// first channel
	firstChannel();
	
	// init channel
	channelsInit();

	// mode ? tv:radio
	if(firstchannel.mode == 't') 
	{
		tvMode(false);
	} 
	else 
	{
		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_RADIO_MODE, false );
		
		radioMode(false);
	}

	if(channelList->getSize() && live_channel_id) 
	{
		channelList->adjustToChannelID(live_channel_id);

		// show service name in vfd (250hd has only 4 digit so we show service number)
#if defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_GIGABLUE)
		char tmp[5];
		
		sprintf(tmp, "%04d", channelList->getActiveChannelNumber() );
		
		CVFD::getInstance()->ShowText(tmp); // UTF-8
#else
		CVFD::getInstance()->showServicename(channelList->getActiveChannelName());
#endif	

		// start epg scanning
		g_Sectionsd->setPauseScanning(false);
		g_Sectionsd->setServiceChanged(live_channel_id&0xFFFFFFFFFFFFULL, true );
		
		// process apids
		g_Zapit->getPIDS(g_RemoteControl->current_PIDs);
		g_RemoteControl->processAPIDnames();
		
		// show info bar
		g_RCInput->postMsg(NeutrinoMessages::SHOW_INFOBAR, 0);
		
		SelectSubtitles();
		
		StartSubtitles();
	}
}

// setup recording device
void CNeutrinoApp::setupRecordingDevice(void)
{
	if (g_settings.recording_type == RECORDING_FILE)
	{
		recordingdevice = new CVCRControl::CFileDevice(g_settings.network_nfs_recordingdir );

		CVCRControl::getInstance()->registerDevice(recordingdevice);
	}
	else
	{
		if (CVCRControl::getInstance()->isDeviceRegistered())
			CVCRControl::getInstance()->unregisterDevice();
	}
}

extern CMenuOptionStringChooser * tzSelect;

void CISendMessage(uint32_t msg, uint32_t data)
{
	g_RCInput->postMsg(msg, data);
}

int CNeutrinoApp::run(int argc, char **argv)
{
	CmdParser(argc, argv);
	
	// font
	font.name = NULL;

	// load settings
	int loadSettingsErg = loadSetup(NEUTRINO_SETTINGS_FILE);
	
	// init isomap
	initialize_iso639_map();

	// check locale language
	bool display_language_selection;
	CLocaleManager::loadLocale_ret_t loadLocale_ret = g_Locale->loadLocale(g_settings.language);

	if (loadLocale_ret == CLocaleManager::NO_SUCH_LOCALE)
	{
		strcpy(g_settings.language, "english");
		loadLocale_ret = g_Locale->loadLocale(g_settings.language);
		display_language_selection = true;
	}
	else
		display_language_selection = false;

	// setup fonts
	SetupFonts();
	
	// setup menue timing
	SetupTiming();
	
	// setup color
	colorSetupNotifier = new CColorSetupNotifier;
	colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);

	// init vfd/lcd display
#if ENABLE_LCD
	CVFD::getInstance()->init(font.filename, font.name);
#else	
	CVFD::getInstance()->init();
#endif	

	// VFD clear all symbols
	CVFD::getInstance()->Clear();

	// show msg in vfd
#if defined (PLATFORM_GIGABLUE) || defined (PLATFORM_CUBEREVO_250HD)
	CVFD::getInstance()->ShowText( (char *)"NHD2");
#else
	CVFD::getInstance()->ShowText((char *)"NEUTRINOHD2");
#endif	
	
	// zapit	
	//zapit start parameters
	Z_start_arg ZapStart_arg;
	
	ZapStart_arg.lastchannelmode = g_settings.lastChannelMode;
	ZapStart_arg.startchanneltv_id = g_settings.startchanneltv_id;
	ZapStart_arg.startchannelradio_id = g_settings.startchannelradio_id;
	ZapStart_arg.startchanneltv_nr = g_settings.startchanneltv_nr;
	ZapStart_arg.startchannelradio_nr = g_settings.startchannelradio_nr;
	ZapStart_arg.uselastchannel = g_settings.uselastchannel;
	
	ZapStart_arg.video_mode = g_settings.video_Mode;

	pthread_create (&zapit_thread, NULL, zapit_main_thread, (void *) &ZapStart_arg);	

	// wait until zapit is ready
	while(!zapit_ready)
		usleep(0);
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::run: zapit ready\n\n");
	
	// dvbsub thread
	dvbsub_init();

	// Audio
	if(audioDecoder)
	{
		// set volume
		audioDecoder->setVolume(g_settings.current_volume, g_settings.current_volume);
	}

	// Video
	videoSetupNotifier = new CVideoSetupNotifier;
	
	// video format
	videoSetupNotifier->changeNotify(LOCALE_VIDEOMENU_VIDEOFORMAT, NULL);
	
	// wss
	videoSetupNotifier->changeNotify(LOCALE_VIDEOMENU_WSS, NULL);
	
	// A/V sync */
	audioSetupNotifier = new CAudioSetupNotifier;
	audioSetupNotifier->changeNotify(LOCALE_AUDIOMENU_AVSYNC, NULL);
	audioSetupNotifier->changeNotify(LOCALE_AUDIOMENU_HDMI_DD, NULL);
	audioSetupNotifier->changeNotify(LOCALE_AUDIOMENU_AC3_DELAY, NULL);
	audioSetupNotifier->changeNotify(LOCALE_AUDIOMENU_AC3_DELAY, NULL);
	
#if ENABLE_GRAPHLCD
	nGLCD::getInstance();
#endif	

	// timerd thread
	pthread_create(&timer_thread, NULL, timerd_main_thread, (void *) NULL);

	// nhttpd thread
	pthread_create(&nhttpd_thread, NULL, nhttpd_main_thread, (void *) NULL);	

	// streamts thread: disabled we use streamts	
	pthread_create (&stream_thread, NULL, streamts_main_thread, (void *) NULL);	

	// sectionsd thread
	pthread_create(&sections_thread, NULL, sectionsd_main_thread, (void *) NULL);

	// for boxes with lcd :-)
#if ENABLE_LCD	
	CVFD::getInstance()->showVolume(g_settings.current_volume);
	CVFD::getInstance()->setMuted(current_muted);
#endif

	// rc 
	g_RCInput = new CRCInput;
	
	// sectionsd client
	g_Sectionsd = new CSectionsdClient;
	
	// timed client
	g_Timerd = new CTimerdClient;
	
	// remote control
	g_RemoteControl = new CRemoteControl;
	
	// epg Data
	g_EpgData = new CEpgData;
	
	// channel infoviewer
	g_InfoViewer = new CInfoViewer;
	
	// event list
	g_EventList = new EventList;
	
	// volume bar
	g_volscale = new CProgressBar(200, 15, 50, 100, 80, true);

	// Ci Cam handler
	g_CamHandler = new CCAMMenuHandler();
	g_CamHandler->init();	

	// plugins
	g_PluginList = new CPlugins;
	g_PluginList->setPluginDir(PLUGINDIR);
	
	// picviewer
	g_PicViewer = new CPictureViewer();
	
	// mount shares before scanning for plugins
	CFSMounter::automount();

	// load Pluginlist before main menu (only show script menu if at least one script is available
	g_PluginList->loadPlugins();

	APIDChanger = new CAPIDChangeExec;
	
	// init nvod changer
	NVODChanger = new CNVODChangeExec;
	
	// init stream feature changer
	StreamFeaturesChanger = new CStreamFeaturesChangeExec; // user menu
	
	// init IP changer
	MyIPChanger = new CIPChangeNotifier;
	
	// init rclock
	rcLock = new CRCLock();
	
	// init timerlist
	Timerlist = new CTimerList;	// defined in neutrino.h

	// setup recording device
	if (g_settings.recording_type != RECORDING_OFF)
		setupRecordingDevice();

	dprintf( DEBUG_NORMAL, "CNeutrinoApp::run: menue setup\n");

	// SMS key input
	c_SMSKeyInput = new SMSKeyInput();	//filebrowser.cpp

	// init gui
	CMenuWidget    mainMenu(LOCALE_MAINMENU_HEAD, NEUTRINO_ICON_MAINMENU);
	CMenuWidget    mainSettings(LOCALE_MAINSETTINGS_HEAD, NEUTRINO_ICON_SETTINGS);
	CMenuWidget    videoSettings(LOCALE_VIDEOMENU_HEAD, NEUTRINO_ICON_VIDEO);
	CMenuWidget    audioSettings(LOCALE_AUDIOMENU_HEAD, NEUTRINO_ICON_AUDIO);
	CMenuWidget    parentallockSettings(LOCALE_PARENTALLOCK_PARENTALLOCK, NEUTRINO_ICON_LOCK);
	CMenuWidget    languageSettings(LOCALE_LANGUAGESETUP_HEAD, NEUTRINO_ICON_LANGUAGE );
	CMenuWidget    networkSettings(LOCALE_NETWORKMENU_HEAD, NEUTRINO_ICON_NETWORK);
	CMenuWidget    recordingSettings(LOCALE_RECORDINGMENU_HEAD, NEUTRINO_ICON_RECORDING );
	CMenuWidget    streamingSettings(LOCALE_STREAMINGMENU_HEAD, NEUTRINO_ICON_STREAMING );
	CMenuWidget    colorSettings(LOCALE_MAINSETTINGS_OSD, NEUTRINO_ICON_COLORS );
	CMenuWidget    lcdSettings(LOCALE_LCDMENU_HEAD, NEUTRINO_ICON_LCD );
	CMenuWidget    keySettings(LOCALE_MAINSETTINGS_KEYBINDING, NEUTRINO_ICON_KEYBINDING );
	CMenuWidget    miscSettings(LOCALE_MISCSETTINGS_HEAD, NEUTRINO_ICON_SETTINGS);
	CMenuWidget    audioplayerSettings(LOCALE_AUDIOPLAYERSETTINGS_GENERAL, NEUTRINO_ICON_SETTINGS);
	CMenuWidget    PicViewerSettings(LOCALE_PICTUREVIEWERSETTINGS_GENERAL, NEUTRINO_ICON_SETTINGS );
	CMenuWidget    service(LOCALE_SERVICEMENU_HEAD, NEUTRINO_ICON_SETTINGS);
	CMenuWidget    TunerSetup( LOCALE_SERVICEMENU_SCANTS, NEUTRINO_ICON_SETTINGS);
	CMenuWidget    bindSettings(LOCALE_KEYBINDINGMENU_HEAD, NEUTRINO_ICON_KEYBINDING );
	CMenuWidget    MediaPlayer(LOCALE_MAINMENU_MEDIAPLAYER, NEUTRINO_ICON_MOVIE);

	// main menu
	InitMainMenu(mainMenu, mainSettings, 
		     videoSettings,
		     audioSettings,
		     parentallockSettings,
		     networkSettings, 
		     recordingSettings, 
		     colorSettings, 
		     lcdSettings, 
		     keySettings, 
		     languageSettings, 
		     miscSettings, 
		     service, 
		     audioplayerSettings, 
		     PicViewerSettings, 
		     streamingSettings, 
		     MediaPlayer);

	// service
	InitServiceSettings(service, TunerSetup);
	
	// language
	InitLanguageSettings(languageSettings);
	
	// audioplayer
	InitAudioplayerSettings(audioplayerSettings);
	
	// picviewer
	InitPicViewerSettings(PicViewerSettings);
	
	//miscSettings
	CMenuWidget    miscSettingsGeneral(LOCALE_MISCSETTINGS_GENERAL, NEUTRINO_ICON_SETTINGS);
	CMenuWidget    miscSettingsChannelList(LOCALE_MISCSETTINGS_CHANNELLIST, NEUTRINO_ICON_SETTINGS);
	CMenuWidget    miscSettingsEPG(LOCALE_MISCSETTINGS_EPG_HEAD, NEUTRINO_ICON_SETTINGS);
	CMenuWidget    miscSettingsFileBrowser(LOCALE_FILEBROWSER_HEAD, NEUTRINO_ICON_SETTINGS);
	
	InitMiscSettings(miscSettings, 
			 miscSettingsGeneral, 
			 miscSettingsChannelList, 
			 miscSettingsEPG, 
			 miscSettingsFileBrowser );
	
	// video settings
	InitVideoSettings(videoSettings, videoSetupNotifier);
	
	// audiosettings
	InitAudioSettings(audioSettings, audioSetupNotifier);
	
	// parentallock
	InitParentalLockSettings(parentallockSettings);
	
	//init scan settings
	//CScanSetup scansetup;

	dprintf( DEBUG_NORMAL, "CNeutrinoApp::run: registering as event client\n");
	
	sleep(1);

	// g_sectionsd
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_TIMESET, 222, NEUTRINO_UDS_NAME);
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_GOT_CN_EPG, 222, NEUTRINO_UDS_NAME);
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_SERVICES_UPDATE, 222, NEUTRINO_UDS_NAME);
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_BOUQUETS_UPDATE, 222, NEUTRINO_UDS_NAME);
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_WRITE_SI_FINISHED, 222, NEUTRINO_UDS_NAME);

	// g_ZapitClient
#define ZAPIT_EVENT_COUNT 30
	const CZapitClient::events zapit_event[ZAPIT_EVENT_COUNT] =
	{
		CZapitClient::EVT_ZAP_COMPLETE,
		CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD,
		CZapitClient::EVT_ZAP_FAILED,
		CZapitClient::EVT_ZAP_SUB_COMPLETE,
		CZapitClient::EVT_ZAP_SUB_FAILED,
		CZapitClient::EVT_ZAP_MOTOR,
		CZapitClient::EVT_ZAP_CA_CLEAR,
		CZapitClient::EVT_ZAP_CA_LOCK,
		CZapitClient::EVT_ZAP_CA_FTA,
		CZapitClient::EVT_ZAP_CA_ID,
		CZapitClient::EVT_RECORDMODE_ACTIVATED,
		CZapitClient::EVT_RECORDMODE_DEACTIVATED,
		CZapitClient::EVT_SCAN_COMPLETE,
		CZapitClient::EVT_SCAN_FAILED,
		CZapitClient::EVT_SCAN_NUM_TRANSPONDERS,
		CZapitClient::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS,
		CZapitClient::EVT_SCAN_REPORT_FREQUENCY,
		CZapitClient::EVT_SCAN_REPORT_FREQUENCYP,
		CZapitClient::EVT_SCAN_SATELLITE,
		CZapitClient::EVT_SCAN_NUM_CHANNELS,
		CZapitClient::EVT_SCAN_PROVIDER,
		CZapitClient::EVT_BOUQUETS_CHANGED,
		CZapitClient::EVT_SERVICES_CHANGED,
		CZapitClient::EVT_SCAN_SERVICENAME,
		CZapitClient::EVT_SCAN_FOUND_A_CHAN,
		CZapitClient::EVT_SCAN_FOUND_TV_CHAN,
		CZapitClient::EVT_SCAN_FOUND_RADIO_CHAN,
		CZapitClient::EVT_SCAN_FOUND_DATA_CHAN,
		CZapitClient::EVT_SDT_CHANGED,
		CZapitClient::EVT_PMT_CHANGED
	};

	for (int i = 0; i < ZAPIT_EVENT_COUNT; i++)
		g_Zapit->registerEvent(zapit_event[i], 222, NEUTRINO_UDS_NAME);

	// g_timerd
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_SHUTDOWN, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_SHUTDOWN, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_NEXTPROGRAM, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_NEXTPROGRAM, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_STANDBY_ON, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_STANDBY_OFF, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_RECORD, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_RECORD_START, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_RECORD_STOP, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_ZAPTO, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ZAPTO, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_SLEEPTIMER, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_SLEEPTIMER, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_REMIND, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_EXEC_PLUGIN, 222, NEUTRINO_UDS_NAME);

	/**/
	//FIXME: why only after regestring sectionsd/zapit events???
	InitNetworkSettings(networkSettings);
	InitKeySettings(keySettings, bindSettings);
	InitColorSettings(colorSettings);

	// start assistant
	if(loadSettingsErg) 
	{
		// set help bar
		g_settings.help_bar = 1;
		
		// startup pic
		frameBuffer->loadBackgroundPic("start.jpg");	
#ifdef FB_BLIT
		frameBuffer->blit();
#endif				
		// setup languages
		int ret = languageSettings.exec(NULL, "");

		// video setup wizard
		if(ret != menu_return::RETURN_EXIT_ALL)
			videoSettings.exec(NULL, "");
		
		 // audio setup wizard
		if(ret != menu_return::RETURN_EXIT_ALL)
			audioSettings.exec(NULL, "");

		// setup color
		if(ret != menu_return::RETURN_EXIT_ALL)
			colorSettings.exec(NULL, "");

		// setup timezone
		if(ret != menu_return::RETURN_EXIT_ALL)
			if(tzSelect)
				tzSelect->exec(NULL);

		// setup network
		if(ret != menu_return::RETURN_EXIT_ALL)
			networkSettings.exec(NULL, "");

		// setup scan settings
		//if(ret != menu_return::RETURN_EXIT_ALL)
		//	scansetup.exec(NULL, "");

		dprintf(DEBUG_INFO, "config file or options missing\n");

		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, loadSettingsErg ==  1 ? g_Locale->getText(LOCALE_SETTINGS_NOCONFFILE) : g_Locale->getText(LOCALE_SETTINGS_MISSINGOPTIONSCONFFILE));
		
		configfile.setModifiedFlag(true);
		
		g_settings.help_bar = 0;

		saveSetup(NEUTRINO_SETTINGS_FILE);
	}

	// HDD mount devices
	//assuming that mdev/fstab has mounted devices
	CHDDDestExec * hdd = new CHDDDestExec();
	hdd->exec(NULL, "");
	
	delete hdd;

	// zapper
	InitZapper();

	// recordingsettings
	InitRecordingSettings(recordingSettings);

	// streamungsettings
	InitStreamingSettings(streamingSettings);

	// lcdsettinsg
	InitLcdSettings(lcdSettings);

#if !defined (PLATFORM_GIGABLUE)  && !defined (PLATFORM_DREAMBOX) && !defined (PLATFORM_XTREND) && !defined (PLATFORM_VUPLUS)
	if(g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER] == 1)
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::run: vfd set led on\n");

		CVFD::getInstance()->setPower(1);
		CVFD::getInstance()->setlcdparameter();
	}
	else if(g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER] == 0)
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::run: vfd set led off\n");

		CVFD::getInstance()->setPower(0);
		CVFD::getInstance()->setlcdparameter();
	}
#endif	
	
	// audio mute
	AudioMute(current_muted, true);

	// init shutdown count
	SHTDCNT::getInstance()->init();

	// Cam-Ci
	cDvbCi::getInstance()->SetHook(CISendMessage);	

	// real run ;-)
	RealRun(mainMenu);

	// exitRun
	ExitRun();

	// never reached
	return 0;
}

// quickZap
void CNeutrinoApp::quickZap(int msg)
{
	int res;

	StopSubtitles();
	
	if((bouquetList != NULL) && !(bouquetList->Bouquets.empty()))
		bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->quickZap(msg, g_settings.zap_cycle);
	else
		channelList->quickZap(msg);
}

// showInfo
void CNeutrinoApp::showInfo()
{
	StopSubtitles();
	g_InfoViewer->showTitle(channelList->getActiveChannelNumber(), channelList->getActiveChannelName(), channelList->getActiveSatellitePosition(), channelList->getActiveChannel_ChannelID());
	StartSubtitles();
}

// real run
void CNeutrinoApp::RealRun(CMenuWidget &mainMenu)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::RealRun: initialized everything\n");

	// start plugins
	g_PluginList->startPlugin("startup.cfg");

	// clear msg 
	g_RCInput->clearRCMsg();

	// if start to standby */
	if(g_settings.power_standby)
	{
		standbyMode(true);
	}

	// neutrino run loop
	while( true ) 
	{
		g_RCInput->getMsg(&msg, &data, 100);	// 10 secs..

		// mode TV/Radio
		if( (mode == mode_tv) || (mode == mode_radio) ) 
		{
			if(msg == NeutrinoMessages::SHOW_EPG) 
			{
				StopSubtitles();
				
				g_EpgData->show(live_channel_id);

				StartSubtitles();
			}
			else if( msg == CRCInput::RC_epg ) 
			{
				StopSubtitles();
				
				g_EventList->exec(live_channel_id, channelList->getActiveChannelName());

				StartSubtitles();
			}
			else if( msg == CRCInput::RC_text) 
			{
				g_RCInput->clearRCMsg();

				StopSubtitles();
				
				tuxtx_stop_subtitle();

				tuxtx_main(g_RCInput->getFileHandle(), g_RemoteControl->current_PIDs.PIDs.vtxtpid, 0, live_fe?live_fe->fenumber:0 );

				frameBuffer->paintBackground();

#ifdef FB_BLIT
				frameBuffer->blit();
#endif
				
				g_RCInput->clearRCMsg();
				
				AudioMute(current_muted, true);

				StartSubtitles();
			}			
			else if( msg == (neutrino_msg_t)g_settings.key_timelist ) 
			{
				StopSubtitles();
				
				Timerlist->exec(NULL, "");
				
				StartSubtitles();
			}		
			else if( msg == CRCInput::RC_setup ) 
			{
				StopSubtitles();

				mainMenu.exec(NULL, "");

				// restore mute symbol
				AudioMute(current_muted, true);

				StartSubtitles();
			}
			else if( msg == (neutrino_msg_t) g_settings.key_tvradio_mode ) 
			{
				if( mode == mode_tv )
					radioMode();
				else if( mode == mode_radio )
					tvMode();
			}
			else if( ( msg == (neutrino_msg_t) g_settings.key_quickzap_up ) || ( msg == (neutrino_msg_t) g_settings.key_quickzap_down ) )
			{
				quickZap(msg);
			}
			else if( msg == (neutrino_msg_t) g_settings.key_subchannel_up ) 
			{
			   	if(g_RemoteControl->subChannels.size() > 0) 
				{
					StopSubtitles();
					g_RemoteControl->subChannelUp();
					g_InfoViewer->showSubchan(); 
			    	} 
			    	else if(g_settings.virtual_zap_mode) 
				{
					if(channelList->getSize()) 
						showInfo();	
				}
				else
				{
					quickZap(msg);
				}
			}
			else if( msg == (neutrino_msg_t) g_settings.key_subchannel_down ) 
			{
			   	if(g_RemoteControl->subChannels.size()> 0) 
				{
					StopSubtitles();
					g_RemoteControl->subChannelDown();
					g_InfoViewer->showSubchan();
			    	} 
			    	else if(g_settings.virtual_zap_mode) 
				{
					if(channelList->getSize()) 
						showInfo();	
				}
				else
				{
					quickZap(msg);
				}
			}
			// in case key_subchannel_up/down redefined
			else if( msg == CRCInput::RC_left || msg == CRCInput::RC_right) 
			{
				if(channelList->getSize()) 
				{
					showInfo();
				}
			}
			else if( msg == (neutrino_msg_t) g_settings.key_zaphistory ) 
			{
				StopSubtitles();
				
				// Zap-History "Bouquet"
				int res = channelList->numericZap( msg );

				StartSubtitles(res < 0);
			}
			else if( msg == (neutrino_msg_t) g_settings.key_lastchannel ) 
			{
				StopSubtitles();
				
				// Quick Zap
				int res = channelList->numericZap( msg );

				StartSubtitles(res < 0);
			}
			else if( msg == (neutrino_msg_t) g_settings.key_timeshift ) // start timeshift recording
			{
			   	if( g_settings.recording_type == RECORDING_FILE )
				{
					dprintf(DEBUG_NORMAL, "CNeutrinoApp::RealRun: timeshift try, timeshiftstatus %d, rec dir %s, timeshift dir %s temp timeshift %d ...\n", recordingstatus, g_settings.network_nfs_recordingdir, timeshiftDir, g_settings.temp_timeshift);
					
					if(recordingstatus && msg == (neutrino_msg_t) g_settings.key_timeshift)
						tmode = "ptimeshift"; 	// already recording, pause
					else
						tmode = "timeshift"; 	// record just started

					if(g_RemoteControl->is_video_started) 
					{		
						// ptimeshift
						if(recordingstatus) 
						{
							timeshiftstatus = recordingstatus;
							
							//g_Zapit->lockPlayBack();
							audioDecoder->Stop();
							videoDecoder->Stop(false);
						} 
						else
						{
							//temp timeshift
							if(g_settings.temp_timeshift) 
							{
								startAutoRecord(true);
							} 
							else // timeshift
							{
								recordingstatus = 1;
								
								timeshiftstatus = recordingstatus;

								doGuiRecord(g_settings.network_nfs_recordingdir, true);
							}
							
							// jump in movieplayer mode
							if(timeshiftstatus) 
							{
								//g_Zapit->lockPlayBack();
								audioDecoder->Stop();
								videoDecoder->Stop(false);
							}
						}
					}
			   	}
			}
			else if( (msg == (neutrino_msg_t)g_settings.mpkey_play) && timeshiftstatus) // play timeshift
			{
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::RealRun: timeshift play, timeshiftstatus %d, rec dir %s, timeshift dir %s temp timeshift %d ...\n", recordingstatus, g_settings.network_nfs_recordingdir, timeshiftDir, g_settings.temp_timeshift);
				printf("g_RemoteControl->is_video_started:%d\n\n", g_RemoteControl->is_video_started);

				if(g_RemoteControl->is_video_started) 
				{
					if(timeshiftstatus) 
					{
						moviePlayerGui->exec(NULL, tmode);
					}
				}
			}
			else if( msg == CRCInput::RC_record || msg == CRCInput::RC_stop ) 
			{
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::RealRun\n");

				// stop record if recording
				if( recordingstatus ) 
				{
					if(ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_SHUTDOWN_RECODING_QUERY, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, NULL, 450, 30, true) == CMessageBox::mbrYes)
					{
						g_Timerd->stopTimerEvent(recording_id);
						
						CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false );
					}
				} 
				// start record
				else if(msg != CRCInput::RC_stop )
				{
					recordingstatus = 1;

					doGuiRecord( g_settings.network_nfs_recordingdir, true );
				}
			}
			else if( msg == CRCInput::RC_red ) 
			{
				StopSubtitles();
				// event list
				showUserMenu(SNeutrinoSettings::BUTTON_RED);
				StartSubtitles();
			}
			else if( ( msg == CRCInput::RC_green) || ( msg == CRCInput::RC_audio) )
			{
				StopSubtitles();
				// audio
				showUserMenu(SNeutrinoSettings::BUTTON_GREEN);
				StartSubtitles();
			}
			else if( msg == CRCInput::RC_yellow || msg == CRCInput::RC_multifeed) 
			{ 
				StopSubtitles();
				// NVODs
				showUserMenu(SNeutrinoSettings::BUTTON_YELLOW);
				StartSubtitles();
			}
			else if( msg == CRCInput::RC_blue ) 
			{
				StopSubtitles();
				// features
				showUserMenu(SNeutrinoSettings::BUTTON_BLUE);
				StartSubtitles();
			}
#if defined (PLATFORM_GIGABLUE)			
			else if( msg == CRCInput::RC_f1 ) 
			{
				StopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F1);
				StartSubtitles();
			}
			else if( msg == CRCInput::RC_f2 )
			{
				StopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F2);
				StartSubtitles();
			}
			else if( msg == CRCInput::RC_f3 ) 
			{ 
				StopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F3);
				StartSubtitles();
			}
			else if( msg == CRCInput::RC_f4 ) 
			{
				StopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F4);
				StartSubtitles();
			}
#endif			
			//music player
			else if( msg == (neutrino_msg_t)g_settings.key_music ) 
			{
				StopSubtitles();

				CAudioPlayerGui tmpAudioPlayerGui;
				tmpAudioPlayerGui.exec(NULL, "");

				StartSubtitles();
			}
			else if( msg == CRCInput::RC_dvbsub)
			{
				StopSubtitles();
				
				// show list only if we have subs
				if(live_channel)
				{
					if(live_channel->getSubtitleCount() > 0)
					{
						CDVBSubSelectMenuHandler tmpDVBSubSelectMenuHandler;
						tmpDVBSubSelectMenuHandler.exec(NULL, "");
					}
				}
				
				StartSubtitles();
			}			
			else if( msg == (neutrino_msg_t)g_settings.key_net ) 	// internet radio
			{
				StopSubtitles();

				CAudioPlayerGui tmpAudioPlayerGui(true);
				tmpAudioPlayerGui.exec(NULL, "");

				StartSubtitles();
			}			
			else if( msg == (neutrino_msg_t)g_settings.key_video )	// movie browser (recorded files)
			{
#ifdef ENABLE_GRAPHLCD
				std::string c = "MoviePlayer";
				nGLCD::lockChannel(c);
#endif			  
			  
				StopSubtitles();

				moviePlayerGui->exec(NULL, "tsmoviebrowser");

				if( mode == mode_radio )
				{
					frameBuffer->loadBackgroundPic("radio.jpg");
						
#ifdef FB_BLIT
					frameBuffer->blit();
#endif						
				}
					
				StartSubtitles();
				
#if ENABLE_GRAPHLCD
				nGLCD::unlockChannel();
#endif				
			}
			else if( msg == (neutrino_msg_t)g_settings.key_video_player )	// fileplayer
			{
#ifdef ENABLE_GRAPHLCD
				std::string c = "MoviePlayer";
				nGLCD::lockChannel(c);
#endif			  
			  
				StopSubtitles();

				moviePlayerGui->exec(NULL, "fileplayback");

				if( mode == mode_radio )
				{
					frameBuffer->loadBackgroundPic("radio.jpg");
						
#ifdef FB_BLIT
					frameBuffer->blit();
#endif						
				}
					
				StartSubtitles();
				
#if ENABLE_GRAPHLCD
				nGLCD::unlockChannel();
#endif				
			}	
			else if( msg == (neutrino_msg_t)g_settings.key_picture ) 	// picture viewer
			{
				StopSubtitles();
				
				CPictureViewerGui tmpPictureViewerGui;
				tmpPictureViewerGui.exec(NULL, "");
				
				StartSubtitles();
			}			
			else if (CRCInput::isNumeric(msg) && g_RemoteControl->director_mode ) 
			{
				StopSubtitles();
				
				g_RemoteControl->setSubChannel(CRCInput::getNumericValue(msg));
				
				g_InfoViewer->showSubchan();
				
				StartSubtitles();
			}
			else if (CRCInput::isNumeric(msg)) 
			{
				StopSubtitles();
				
				channelList->numericZap( msg );
				
				StartSubtitles();
			}
			else if( (msg == CRCInput::RC_info) || ( msg == NeutrinoMessages::SHOW_INFOBAR ) )
			{
				bool show_info = ((msg != NeutrinoMessages::SHOW_INFOBAR) || (g_InfoViewer->is_visible || g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR] != 0));
				
			         // turn on LCD display
				CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
				
				if(show_info && channelList->getSize()) 
				{
					showInfo();
				}
				
#ifdef ENABLE_GRAPHLCD
				if (msg == NeutrinoMessages::EVT_CURRENTNEXT_EPG)
					nGLCD::Update();				
#endif				
			}
			else if( (msg == (neutrino_msg_t) g_settings.key_pip) || (msg == (neutrino_msg_t) g_settings.key_pip_subchannel) )
			{
				StopSubtitles();
				
				// first steo show channels from the same TP
				channelList->numericZap( msg );
				
				StartSubtitles();
			}
			else 
			{
				if ( msg == CRCInput::RC_home )
				{
  					CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
				}

				handleMsg(msg, data);
			}
		}
		else //other modes
		{
			if( msg == CRCInput::RC_home ) 
			{
				if( mode == mode_scart ) 
				{
					//wenn VCR Aufnahme dann stoppen
					if(CVCRControl::getInstance()->isDeviceRegistered()) 
					{
						if ((CVCRControl::getInstance()->Device->getDeviceType() == CVCRControl::DEVICE_VCR) &&
						    (CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_RECORD ||
						     CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_PAUSE))
						{
							CVCRControl::getInstance()->Stop();
							recordingstatus=0;
							startNextRecording();
						}
					}
					// Scart-Mode verlassen
					scartMode( false );
				}
			}
			else 
			{
				// hier geht es weiter
				handleMsg(msg, data);
			}
		}
	}
}

// handle msg
int CNeutrinoApp::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	int res = 0;

	//dprintf(DEBUG_DEBUG, "[neutrino] handleMsg %X data %X\n", msg, data); fflush(stdout);

	// handle neutrino msg
	if(msg == NeutrinoMessages::EVT_ZAP_COMPLETE) 
	{
		// set audio map after channel zap
		g_Zapit->getAudioMode(&g_settings.audio_AnalogMode);

		if(g_settings.audio_AnalogMode < 0 || g_settings.audio_AnalogMode > 2)
			g_settings.audio_AnalogMode = 0;

		if(shift_timer) 
		{
			g_RCInput->killTimer(shift_timer);
			shift_timer = 0;
		}

		// auto timeshift
		if (!recordingstatus && g_settings.auto_timeshift) 		  
		{			
			int delay = g_settings.auto_timeshift;			
			shift_timer = g_RCInput->addTimer( delay*1000*1000, true );
			g_InfoViewer->handleMsg(NeutrinoMessages::EVT_RECORDMODE, 1);
		}
		
#if ENABLE_GRAPHLCD
		nGLCD::Update();
#endif		

		if(scrambled_timer) 
		{
			g_RCInput->killTimer(scrambled_timer);
			scrambled_timer = 0;
		}

		scrambled_timer = g_RCInput->addTimer(10*1000*1000, true);
		
		// select subtitle
		SelectSubtitles();
		
		StartSubtitles(!g_InfoViewer->is_visible);
	}

	if ((msg == NeutrinoMessages::EVT_TIMER)) 
	{
		if(data == shift_timer) 
		{
			shift_timer = 0;
			startAutoRecord(true);
			return messages_return::handled;
		} 
		else if(data == scrambled_timer) 
		{
			scrambled_timer = 0;

			return messages_return::handled;	
		}
	}

	// handle msg with remotecontrol/Infoviewer/Channellist
	res = res | g_RemoteControl->handleMsg(msg, data);
	res = res | g_InfoViewer->handleMsg(msg, data);
	res = res | channelList->handleMsg(msg, data);

	//dprintf(DEBUG_DEBUG, "[neutrino] handleMsg %X unhandled by others\n", msg); fflush(stdout);
	
	if( res != messages_return::unhandled ) 
	{
		if( ( msg>= CRCInput::RC_WithData ) && ( msg< CRCInput::RC_WithData+ 0x10000000 ) )
			delete (unsigned char*) data;

		return( res & ( 0xFFFFFFFF - messages_return::unhandled ) );
	}

	// we assume g_CamHandler free/delete data if needed
	res = g_CamHandler->handleMsg(msg, data);
	if( res != messages_return::unhandled ) 
	{
		return(res & (0xFFFFFFFF - messages_return::unhandled));
	}	

	// handle Keys
	if( msg == CRCInput::RC_ok || msg == CRCInput::RC_sat || msg == CRCInput::RC_favorites)
	{
		if( (mode == mode_tv) || (mode == mode_radio))
		{
			StopSubtitles();

			int nNewChannel = -1;
			int old_num = 0;
			int old_b = bouquetList->getActiveBouquetNumber();
			int old_mode = g_settings.channel_mode;
			
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: ZAP START: bouquetList %x size %d old_b %d\n", (int) bouquetList, bouquetList->Bouquets.size(), old_b);fflush(stdout);

			if(bouquetList->Bouquets.size()) 
			{
				old_num = bouquetList->Bouquets[old_b]->channelList->getActiveChannelNumber();
			}

			if( msg == CRCInput::RC_ok ) 
			{
				if(bouquetList->Bouquets.size() && bouquetList->Bouquets[old_b]->channelList->getSize() > 0)
					nNewChannel = bouquetList->Bouquets[old_b]->channelList->exec();	//with ZAP!
				else
					nNewChannel = bouquetList->exec(true);
			}
			else if(msg == CRCInput::RC_sat) 
			{
				SetChannelMode(LIST_MODE_SAT);
				nNewChannel = bouquetList->exec(true);
			}
			else if(msg == CRCInput::RC_favorites) 
			{
				SetChannelMode(LIST_MODE_FAV);
				nNewChannel = bouquetList->exec(true);
			}
_repeat:
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: ZAP RES: nNewChannel %d\n", nNewChannel);
			fflush(stdout);

			if(nNewChannel == -1) 
			{
				// restore orig. bouquet and selected channel on cancel
				SetChannelMode(old_mode);
				bouquetList->activateBouquet(old_b, false);
				
				if(bouquetList->Bouquets.size())
					bouquetList->Bouquets[old_b]->channelList->setSelected(old_num-1);
				
				StartSubtitles(mode == mode_tv);
			}
			else if(nNewChannel == -3) 
			{ 
				// list mode changed
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: bouquetList %x size %d\n", (int) bouquetList, bouquetList->Bouquets.size());
				fflush(stdout);
				nNewChannel = bouquetList->exec(true);
				goto _repeat;
			}
			else if(nNewChannel == -4) 
			{
				if(old_b_id < 0) 
					old_b_id = old_b;

				g_Zapit->saveBouquets();
			}

			return messages_return::handled;
		}
	}
	else if (msg == CRCInput::RC_standby ) 
	{
		if (data == 0) 
		{
			neutrino_msg_t new_msg;

			/* 
			* Note: pressing the power button on the box (not the remote control) over 1 second
			* shuts down the system even if !g_settings.shutdown_real_rcdelay (see below)
			*/
			gettimeofday(&standby_pressed_at, NULL);

			if ((mode != mode_standby) && (g_settings.shutdown_real)) 
			{
				new_msg = NeutrinoMessages::SHUTDOWN;
			}
			else 
			{
				new_msg = (mode == mode_standby) ? NeutrinoMessages::STANDBY_OFF : NeutrinoMessages::STANDBY_ON;
				
				//printf("standby: new msg %X\n", new_msg);
				
				if ((g_settings.shutdown_real_rcdelay)) 
				{
					neutrino_msg_t      msg;
					neutrino_msg_data_t data;
					struct timeval      endtime;
					time_t              seconds;

					int timeout = 0;
					int timeout1 = 0;

					sscanf(g_settings.repeat_blocker, "%d", &timeout);
					sscanf(g_settings.repeat_genericblocker, "%d", &timeout1);

					if (timeout1 > timeout)
						timeout = timeout1;

					timeout += 500;
					
					//printf("standby: timeout %d\n", timeout);

					while(true) 
					{
						g_RCInput->getMsg_ms(&msg, &data, timeout);

						//printf("standby: input msg %X\n", msg);
						
						if (msg == CRCInput::RC_timeout)
							break;

						gettimeofday(&endtime, NULL);
						seconds = endtime.tv_sec - standby_pressed_at.tv_sec;
						if (endtime.tv_usec < standby_pressed_at.tv_usec)
							seconds--;
						
						//printf("standby: input seconds %d\n", seconds);
						
						if (seconds >= 1) 
						{
							if ( msg == CRCInput::RC_standby )
								new_msg = NeutrinoMessages::SHUTDOWN;
							break;
						}
					}
				}
			}
			
			g_RCInput->postMsg(new_msg, 0);
			return messages_return::cancel_all | messages_return::handled;
		}
		else if (standby_pressed_at.tv_sec != 0)
		{
			/* check if we received a KEY_POWER pressed event before */
		        /* required for correct handling of KEY_POWER events of  */
			/* the power button on the dbox (not the remote control) */
			
			struct timeval endtime;
			gettimeofday(&endtime, NULL);
			time_t seconds = endtime.tv_sec - standby_pressed_at.tv_sec;
			if (endtime.tv_usec < standby_pressed_at.tv_usec)
				seconds--;
			if (seconds >= 1) 
			{
				g_RCInput->postMsg(NeutrinoMessages::SHUTDOWN, 0);
				return messages_return::cancel_all | messages_return::handled;
			}
		}
	}
	else if ( (msg == CRCInput::RC_plus) || (msg == CRCInput::RC_minus) )
	{
		setVolume(msg, (mode != mode_scart));
		return messages_return::handled;
	}
	else if( msg == CRCInput::RC_spkr ) 
	{
		if( mode == mode_standby ) 
		{
			//switch lcd off/on
			CVFD::getInstance()->togglePower();
		}
		else 
		{
			//mute
			AudioMute( !current_muted, true);
		}
		return messages_return::handled;
	}
	// event messages
	else if (msg == NeutrinoMessages::EVT_VOLCHANGED) 
	{
		//setVolume(msg, false, true);
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_MUTECHANGED ) 
	{
		delete[] (unsigned char*) data;

		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_SERVICESCHANGED ) 
	{
		channelsInit();

		channelList->adjustToChannelID(live_channel_id);//FIXME
		
		if(old_b_id >= 0) 
		{
			bouquetList->activateBouquet(old_b_id, false);
			old_b_id = -1;
			g_RCInput->postMsg(CRCInput::RC_ok, 0);
		}
	}
	else if( msg == NeutrinoMessages::EVT_BOUQUETSCHANGED ) 
	{
		if(!reloadhintBox)
			reloadhintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_RELOAD_HINT));

		reloadhintBox->paint();

		channelsInit();
		channelList->adjustToChannelID(live_channel_id);//FIXME what if deleted ?
		
		reloadhintBox->hide();

		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_RECORDMODE ) 
	{
		// sent by rcinput, then got msg from zapit about record activated/deactivated
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: recordmode %s\n", ( data ) ? "on":"off" );
		
		if(!recordingstatus && (!data)) 
		{
			if( mode == mode_standby )
			{
				// set standby
				g_Zapit->setStandby(true);				
			}
		}
		
		recordingstatus = data;
		if( ( !g_InfoViewer->is_visible ) && data && !autoshift)
			g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::RECORD_START) 
	{
		if(autoshift) 
		{
			stopAutoRecord();
			recordingstatus = 0;
		}

		puts("CNeutrinoApp::handleMsg: executing " NEUTRINO_RECORDING_START_SCRIPT ".");
		if (system(NEUTRINO_RECORDING_START_SCRIPT) != 0)
			perror(NEUTRINO_RECORDING_START_SCRIPT " failed");
		/* set nextRecordingInfo to current event (replace other scheduled recording if available) */
		/* Note: CTimerd::RecordingInfo is a class!
		 * => typecast to avoid destructor call */

		if (nextRecordingInfo != NULL)
			delete[] (unsigned char *) nextRecordingInfo;

		nextRecordingInfo = (CTimerd::RecordingInfo *) data;
		startNextRecording();

		return messages_return::handled | messages_return::cancel_all;
	}
	else if( msg == NeutrinoMessages::RECORD_STOP) 
	{
		if(((CTimerd::RecordingStopInfo*)data)->eventID == recording_id)
		{ 
			if (CVCRControl::getInstance()->isDeviceRegistered()) 
			{
				if ((CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_RECORD) || (CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_PAUSE ))
				{
					CVCRControl::getInstance()->Stop();
					
					recordingstatus = 0;
					autoshift = 0;
					

					if(timeshiftstatus)
					{
						// set timeshift status to false
						timeshiftstatus = 0;
					}
				}
				else
					printf("CNeutrinoApp::handleMsg: falscher state\n");
			}
			else
				puts("CNeutrinoApp::handleMsg: no recording devices");

			startNextRecording();

			if ( recordingstatus == 0 ) 
			{
				puts("CNeutrinoApp::handleMsg: executing " NEUTRINO_RECORDING_ENDED_SCRIPT ".");
				if (system(NEUTRINO_RECORDING_ENDED_SCRIPT) != 0)
					perror(NEUTRINO_RECORDING_ENDED_SCRIPT " failed");

				CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false);
			}
		}
		else if(nextRecordingInfo != NULL) 
		{
			if(((CTimerd::RecordingStopInfo*)data)->eventID == nextRecordingInfo->eventID) 
			{
				/* Note: CTimerd::RecordingInfo is a class!
				 * => typecast to avoid destructor call */
				delete[] (unsigned char *) nextRecordingInfo;
				nextRecordingInfo=NULL;
			}
		}
		
		delete[] (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_PMT_CHANGED) 
	{
		res = messages_return::handled;
		//t_channel_id channel_id = *(t_channel_id*) data;
		//CRecordManager::getInstance()->Update(channel_id); //FIXME

		return res;
	}
	else if( msg == NeutrinoMessages::ZAPTO ) 
	{
		CTimerd::EventInfo * eventinfo;
		eventinfo = (CTimerd::EventInfo *) data;
		if(recordingstatus == 0) 
		{
			bool isTVMode = g_Zapit->isChannelTVChannel(eventinfo->channel_id);

			dvbsub_stop();

			if ((!isTVMode) && (mode != mode_radio)) 
			{
				radioMode(false);
			}
			else if (isTVMode && (mode != mode_tv)) 
			{
				tvMode(false);
			}
			channelList->zapTo_ChannelID(eventinfo->channel_id);
		}
		delete[] (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_ZAPTO) 
	{
		if( mode == mode_standby ) 
		{
			standbyMode( false );
		}
		
		if( mode != mode_scart ) 
		{
			std::string name = g_Locale->getText(LOCALE_ZAPTOTIMER_ANNOUNCE);

			CTimerd::TimerList tmpTimerList;
			CTimerdClient tmpTimerdClient;

			tmpTimerList.clear();
			tmpTimerdClient.getTimerList( tmpTimerList );

			if(tmpTimerList.size() > 0) 
			{
				sort( tmpTimerList.begin(), tmpTimerList.end() );

				CTimerd::responseGetTimer &timer = tmpTimerList[0];

				CZapitClient Zapit;
				name += "\n";

				std::string zAddData = Zapit.getChannelName( timer.channel_id ); // UTF-8
				if( zAddData.empty()) 
				{
					zAddData = g_Locale->getText(LOCALE_TIMERLIST_PROGRAM_UNKNOWN);
				}

				if(timer.epgID!=0) 
				{
					CEPGData epgdata;
					zAddData += " :\n";
					//if (g_Sectionsd->getEPGid(timer.epgID, timer.epg_starttime, &epgdata))
					if (sectionsd_getEPGid(timer.epgID, timer.epg_starttime, &epgdata)) 
					{
						zAddData += epgdata.title;
					}
					else if(strlen(timer.epgTitle)!=0) 
					{
						zAddData += timer.epgTitle;
					}
				}
				else if(strlen(timer.epgTitle)!=0) 
				{
					zAddData += timer.epgTitle;
				}

				name += zAddData;
			}
			ShowHintUTF( LOCALE_MESSAGEBOX_INFO, name.c_str() );
		}

		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_RECORD) 
	{
		system(NEUTRINO_RECORDING_TIMER_SCRIPT);

		if (g_settings.recording_type == RECORDING_FILE) 
		{
			char * recDir = ((CTimerd::RecordingInfo*)data)->recordingDir;

			// ether-wake
			for(int i = 0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++) 
			{
				if (strcmp(g_settings.network_nfs_local_dir[i], recDir) == 0) 
				{
					dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: waking up %s (%s)\n",g_settings.network_nfs_ip[i].c_str(),recDir);
					
					std::string command = "etherwake ";
					command += g_settings.network_nfs_mac[i];

					if(system(command.c_str()) != 0)
						perror("etherwake failed");
					break;
				}
			}

			//stop autoshift
			if(autoshift) 
			{
				stopAutoRecord();
				recordingstatus = 0;
			}

			//wakeup hdd
			//if(has_hdd) 
			//{
			//	char cmd[100];	
			//	sprintf(cmd, "(rm /media/sda1/.wakeup; touch /media/sda1/.wakeup; sync) > /dev/null  2> /dev/null &");
			//	system(cmd);
			//}
		}
		
		if( g_settings.recording_zap_on_announce ) 
		{
			if(recordingstatus==0) 
			{
				//dvbsub_stop(); //FIXME if same channel ?
				t_channel_id channel_id = ((CTimerd::RecordingInfo*)data)->channel_id;
				g_Zapit->zapTo_serviceID_NOWAIT(channel_id); 
			}
		}

		delete[] (unsigned char*) data;
		
		if( mode != mode_scart )
			ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_RECORDTIMER_ANNOUNCE));
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_SLEEPTIMER) 
	{
		if( mode != mode_scart )
			ShowHintUTF( LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SLEEPTIMERBOX_ANNOUNCE) );
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::SLEEPTIMER) 
	{
		if(g_settings.shutdown_real)
			ExitRun();
		else
			standbyMode( true );
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_TOGGLE ) 
	{
		standbyMode( !(mode & mode_standby) );
		g_RCInput->clearRCMsg();
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_ON ) 
	{
		if( mode != mode_standby ) 
		{
			standbyMode( true );
		}
		g_RCInput->clearRCMsg();
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_OFF ) 
	{
		if( mode == mode_standby ) 
		{
			standbyMode( false );
		}
		g_RCInput->clearRCMsg();
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_SHUTDOWN) 
	{
		if( mode != mode_scart )
			skipShutdownTimer = (ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_SHUTDOWNTIMER_ANNOUNCE, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, 450, 5) == CMessageBox::mbrYes);
	}
	else if( msg == NeutrinoMessages::SHUTDOWN ) 
	{
		if(!skipShutdownTimer) 
		{
			ExitRun();
		}
		else 
		{
			skipShutdownTimer = false;
		}
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_POPUP ) 
	{
		if (mode != mode_scart)
			ShowHintUTF(LOCALE_MESSAGEBOX_INFO, (const char *) data); // UTF-8
		
		delete (unsigned char*) data;
		
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_EXTMSG) 
	{
		if (mode != mode_scart)
			ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, (const char *) data, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO); // UTF-8
		delete[] (unsigned char*) data;
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_RECORDING_ENDED) 
	{
		if (mode != mode_scart) 
		{
			neutrino_locale_t msgbody;
			if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_BUFFER_OVERFLOW)
				msgbody = LOCALE_STREAMING_BUFFER_OVERFLOW;
			else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_WRITE_OPEN_FAILURE)
				msgbody = LOCALE_STREAMING_WRITE_ERROR_OPEN;
			else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_WRITE_FAILURE)
				msgbody = LOCALE_STREAMING_WRITE_ERROR;
			else
				goto skip_message;

			/* use a short timeout of only 5 seconds in case it was only a temporary network problem
			 * in case of STREAM2FILE_STATUS_IDLE we might even have to immediately start the next recording */
			ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, msgbody, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO, 450, 5);

skip_message:
			;
		}
		
		if ((* (stream2file_status2_t *) data).status != STREAM2FILE_STATUS_IDLE) 
		{
			/*
			 * note that changeNotify does not distinguish between LOCALE_MAINMENU_RECORDING_START and LOCALE_MAINMENU_RECORDING_STOP
			 * instead it checks the state of the variable recordingstatus
			 */
			/* restart recording */
			//FIXME doGuiRecord((*(stream2file_status2_t *) data).dir);
			//changeNotify(LOCALE_MAINMENU_RECORDING_START, data);
		}

		delete[] (unsigned char*) data;
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::REMIND) 
	{
		std::string text = (char*)data;
		std::string::size_type pos;
		while((pos=text.find('/'))!= std::string::npos)
		{
			text[pos] = '\n';
		}
		
		if( mode != mode_scart )
			ShowMsgUTF(LOCALE_TIMERLIST_TYPE_REMIND, text, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO); // UTF-8
			
		delete[] (unsigned char*) data;
		
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::LOCK_RC) 
	{
		this->rcLock->exec(NULL,CRCLock::NO_USER_INPUT);
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::CHANGEMODE ) 
	{
		if((data & mode_mask) != mode_radio)
		{
#if ENABLE_RADIOTEXT		  
			if (g_Radiotext)
			{
				delete g_Radiotext;
				g_Radiotext = NULL;
			}
#endif			
		}

		if((data & mode_mask) == mode_radio) 
		{
			if( mode != mode_radio ) 
			{
				if((data & norezap)==norezap)
					radioMode(false);
				else
					radioMode(true);
#if ENABLE_RADIOTEXT				
				if (g_settings.radiotext_enable)
					g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
#endif				
			}
		}
		
		if((data & mode_mask)== mode_tv) 
		{
			if( mode != mode_tv ) 
			{
				if((data & norezap)==norezap)
					tvMode(false);
				else
					tvMode(true);
			}
		}
		
		if((data &mode_mask)== mode_standby) 
		{
			if(mode != mode_standby)
				standbyMode( true );
		}
		
		if((data &mode_mask)== mode_audio) 
		{
			lastMode=mode;
			mode=mode_audio;
		}
		
		if((data &mode_mask)== mode_pic) 
		{
			lastMode=mode;
			mode=mode_pic;
		}
		
		if((data &mode_mask)== mode_ts) 
		{
			lastMode = mode;
			mode = mode_ts;
		}
	}
	else if( msg == NeutrinoMessages::VCR_ON ) 
	{
		if( mode != mode_scart ) 
		{
			scartMode( true );
		}
		else
		{
			CVFD::getInstance()->setMode(CVFD::MODE_SCART);
		}
	}
	else if( msg == NeutrinoMessages::VCR_OFF ) 
	{
		if( mode == mode_scart ) 
		{
			scartMode( false );
		}
	}
	else if (msg == NeutrinoMessages::EVT_START_PLUGIN) 
	{
		g_PluginList->startPlugin((const char *)data);
		
		/*
		if (!g_PluginList->getScriptOutput().empty()) 
		{
			ShowMsgUTF(LOCALE_PLUGINS_RESULT, g_PluginList->getScriptOutput(), CMessageBox::mbrBack,CMessageBox::mbBack,NEUTRINO_ICON_SHELL);
		}
		*/
		
		delete[] (unsigned char*) data;
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_SERVICES_UPD) 
	{
		// dont show this msg
		//ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_ZAPIT_SDTCHANGED), CMessageBox::mbrBack,CMessageBox::mbBack, NEUTRINO_ICON_INFO);
	}
	
	if ((msg >= CRCInput::RC_WithData) && (msg < CRCInput::RC_WithData + 0x10000000))
		delete [] (unsigned char*) data;

	return messages_return::unhandled;
}

// exit run
void CNeutrinoApp::ExitRun(int retcode)
{
	if (!recordingstatus || ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_SHUTDOWN_RECODING_QUERY, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, 450, 30, true) == CMessageBox::mbrYes)
	{
		// stop recording
		if(recordingstatus) 
		{
			CVCRControl::getInstance()->Stop();
			g_Timerd->stopTimerEvent(recording_id);
		}

		// vfd mode shutdown
		CVFD::getInstance()->setMode(CVFD::MODE_SHUTDOWN);
		
		// show good bye in VFD
#if defined (PLATFORM_GIGABLUE) || defined (PLATFORM_CUBEREVO_250HD)
		CVFD::getInstance()->ShowText((char *) "NHD2");
#else
		CVFD::getInstance()->ShowText((char *) "Good Bye");
#endif		

		dprintf(DEBUG_INFO, "ExitRun\n");

		// stop playback
		g_Zapit->stopPlayBack();

		frameBuffer->loadBackgroundPic("shutdown.jpg");
		
#ifdef FB_BLIT
		frameBuffer->blit();
#endif		

		// network down
		networkConfig.automatic_start = (network_automatic_start == 1);
		networkConfig.commitConfig();
		
		// save neutrino.conf
		saveSetup(NEUTRINO_SETTINGS_FILE);

		// save epg
		if(g_settings.epg_save /* && timeset && g_Sectionsd->getIsTimeSet ()*/) 
		{
			saveEpg();
		}

		if(retcode)
		{
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::ExitRun: [shutdown] entering off state\n");
			
			mode = mode_off;

			// check for events
			neutrino_msg_t      msg;
			neutrino_msg_data_t data;

			g_RCInput->clearRCMsg();
			
			// check for events
			while( true ) 
			{
				g_RCInput->getMsg(&msg, &data, 100);		//10 sec
				
				if( msg == CRCInput::RC_standby ) 
				{
					dprintf(DEBUG_NORMAL, "CNeutrinoApp::ExitRun: Power key, going to sleep...\n");
			
					sleep(2);
					
					if (g_RCInput != NULL)
						delete g_RCInput;

					if (frameBuffer != NULL)
						delete frameBuffer;
					
					// stop all deamons threads
					stop_daemons();
					
					exit(retcode);
				} 
				else if( ( msg == NeutrinoMessages::ANNOUNCE_RECORD) || ( msg == NeutrinoMessages::ANNOUNCE_ZAPTO) ) 
				{
					dprintf(DEBUG_NORMAL, "CNeutrinoApp::ExitRun: Zap/record timer, going to reboot...\n");
					
					sleep(2);
					
					if (g_RCInput != NULL)
						delete g_RCInput;

					if (frameBuffer != NULL)
						delete frameBuffer;
					
					// stop all deamons threads
					stop_daemons();
					
					reboot(LINUX_REBOOT_CMD_RESTART);
				}
				else 
				{
					// no message received.
					dprintf(DEBUG_NORMAL, "CNeutrinoApp::ExitRun: going to sleep...\n");
					
					sleep(2);
					
					if (g_RCInput != NULL)
						delete g_RCInput;

					if (frameBuffer != NULL)
						delete frameBuffer;
					
					// stop all deamons threads
					stop_daemons();
					
					dprintf(DEBUG_NORMAL, ">>> CNeutrinoApp::ExitRun: Good bye <<<\n");
					
					exit(retcode);
				}
			}
		} 
		else 	//without retcode
		{
			if (g_RCInput != NULL)
				delete g_RCInput;

			if (frameBuffer != NULL)
				delete frameBuffer;

			stop_daemons();

			dprintf(DEBUG_NORMAL, ">>> CNeutrinoApp::ExitRun: Good bye <<<\n");

			exit(0);
		}
	}
}

// save epg
void CNeutrinoApp::saveEpg()
{
	struct stat my_stat;    
	if(stat(g_settings.epg_dir.c_str(), &my_stat) == 0)
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::saveEpg: Saving EPG to %s....\n", g_settings.epg_dir.c_str());
		neutrino_msg_t      msg;
		neutrino_msg_data_t data;
		
		g_Sectionsd->writeSI2XML(g_settings.epg_dir.c_str());

		while( true ) 
		{
			g_RCInput->getMsg(&msg, &data, 300); // 30 secs..
			if (( msg == CRCInput::RC_timeout ) || (msg == NeutrinoMessages::EVT_SI_FINISHED)) 
			{
				//printf("Msg %x timeout %d EVT_SI_FINISHED %x\n", msg, CRCInput::RC_timeout, NeutrinoMessages::EVT_SI_FINISHED);
				break;
			}
		}
	}
}

// mute
void CNeutrinoApp::AudioMute( int newValue, bool isEvent )
{
	//printf("MUTE: val %d current %d event %d\n", newValue, current_muted, isEvent);

	int dx = 0;
	int dy = 0;
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_MUTE,&dx, &dy);
	int offset = (dx/4);
	dx += offset;
	dy += offset;

	int x = g_settings.screen_EndX - dx;
	int y = g_settings.screen_StartY + 10;

#if ENABLE_LCD
	CVFD::getInstance()->setMuted(newValue);
#endif

	current_muted = newValue;

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::AudioMute: current %d new %d isEvent: %d\n", current_muted, newValue, isEvent);
	
	g_Zapit->muteAudio(current_muted);

	if( isEvent && ( mode != mode_scart ) && ( mode != mode_audio) && ( mode != mode_pic))
	{
		if( current_muted ) 
		{
			//frameBuffer->paintBoxRel(x, y, dx, dy, COL_INFOBAR_PLUS_0);
			
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_MUTE, x+(offset/2), y + (offset/2) );

#ifdef FB_BLIT
			frameBuffer->blit();
#endif
		}
		else
		{
			frameBuffer->paintBackgroundBoxRel(x, y, dx, dy);

#ifdef FB_BLIT
			frameBuffer->blit();
#endif	
		}
	}
}

// set vol
void CNeutrinoApp::setvol(int vol)
{
	if(audioDecoder)
		audioDecoder->setVolume(vol, vol);
}

// set volume
void CNeutrinoApp::setVolume(const neutrino_msg_t key, const bool bDoPaint, bool nowait)
{
	neutrino_msg_t msg = key;

	int dx = 296;	//256 (16*16) for vulme bar + 40 for volume digits
	int dy = 32;

	int x = frameBuffer->getScreenX() + 10;
	int y = frameBuffer->getScreenY() + 10;

	int vol = g_settings.current_volume;
	
	int sw = frameBuffer->getScreenWidth();
	int sh = frameBuffer->getScreenHeight();
	
	int a_step = atoi(g_settings.audio_step);
	
	switch( g_settings.volume_pos )
	{
		case 0:// upper right
			x = sw - dx - 6;
			break;
			
		case 1:// upper left
			break;
			
		case 2:// bottom left
			y = sh - dy;
			break;
			
		case 3:// bottom right
			x = sw - dx;
			y = sh - dy;
			break;
			
		case 4:// center default
			x = ((sw - dx) / 2) + x;
			break;
			
		case 5:// center higher
			x = ((sw - dx) / 2) + x;
			y = sh - sh/15;
			break;
	}

	fb_pixel_t * pixbuf = NULL;

	if(bDoPaint) 
	{
		pixbuf = new fb_pixel_t[dx * dy];

		if(pixbuf != NULL)
		{
			frameBuffer->SaveScreen(x, y, dx, dy, pixbuf);
#ifdef FB_BLIT
			frameBuffer->blit();			
#endif			
		}

		// background box
		frameBuffer->paintBoxRel(x , y , dx, dy, COL_MENUCONTENT_PLUS_0, RADIUS_MID, CORNER_BOTH);
		
		// vol box aussen
		frameBuffer->paintBoxRel(x+dy+dy/4-2, y+dy/4-2, dy*25/4+4, dy/2+4, COL_MENUCONTENT_PLUS_3);
		
		// vol box innen
		frameBuffer->paintBoxRel(x+dy+dy/4,   y+dy/4,   dy*25/4,   dy/2,   COL_MENUCONTENT_PLUS_1);
		
		//icon
		frameBuffer->paintIcon(NEUTRINO_ICON_VOLUME,x+dy/2,y+(dy/4), 0, COL_MENUCONTENT_PLUS_0);

		g_volscale->reset();
		g_volscale->paint(x + dy+ (dy/4), y +(dy/4), vol);
		

		char p1[4]; // 3 digits + '\0'
		sprintf(p1, "%3d", vol);

		// erase the numbers
		frameBuffer->paintBoxRel(x + dx - 50, y , 40, dy, COL_MENUCONTENT_PLUS_0);

		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(x + dx - 45, y + dy/2 + 14, 36, p1, COL_MENUHEAD);

#ifdef FB_BLIT
		frameBuffer->blit();
#endif
	}

	neutrino_msg_data_t data;

	unsigned long long timeoutEnd;

	do {
		if (msg <= CRCInput::RC_MaxRC) 
		{
			if ( msg == CRCInput::RC_plus ) 
			{ 
				if (g_settings.current_volume < 100 - /*5*/ a_step )
					g_settings.current_volume += /*5*/ a_step;
				else
					g_settings.current_volume = 100;
			}
			else if ( msg == CRCInput::RC_minus ) 
			{ 
				if (g_settings.current_volume > /*5*/ a_step)
					g_settings.current_volume -= /*5*/ a_step;
				else
					g_settings.current_volume = 0;
			}
			else 
			{
				g_RCInput->postMsg(msg, data);
				break;
			}

			setvol(g_settings.current_volume);

			timeoutEnd = CRCInput::calcTimeoutEnd(nowait ? 1 : 3);
		}
		else if (msg == NeutrinoMessages::EVT_VOLCHANGED) 
		{
			timeoutEnd = CRCInput::calcTimeoutEnd(3);
		}
		else if (handleMsg(msg, data) & messages_return::unhandled) 
		{
			g_RCInput->postMsg(msg, data);
			break;
		}

		if (bDoPaint) 
		{
			if(vol != g_settings.current_volume) 
			{
				vol = g_settings.current_volume;

				g_volscale->paint(x + dy+ (dy/4), y +(dy/4), /*g_settings.current_volume*/ vol);

				char p[4]; // 3 digits + '\0'
				sprintf(p, "%3d", /*g_settings.current_volume*/ vol);

				// erase the numbers
				frameBuffer->paintBoxRel(x + dx - 50, y , 40, dy, COL_MENUCONTENT_PLUS_0);

				g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(x + dx - 45, y + dy/2 + 14, 36, p, COL_MENUHEAD);
				
#ifdef FB_BLIT
				frameBuffer->blit();
#endif				
			}
		}

#if ENABLE_LCD
		CVFD::getInstance()->showVolume(g_settings.current_volume);
#endif

		if (msg != CRCInput::RC_timeout) 
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
		}

#ifdef FB_BLIT
		frameBuffer->blit();
#endif		
	} while (msg != CRCInput::RC_timeout);

	if( (bDoPaint) && (pixbuf != NULL) ) 
	{
		frameBuffer->RestoreScreen(x, y, dx, dy, pixbuf);
#ifdef FB_BLIT
		frameBuffer->blit();
#endif		
		delete [] pixbuf;
	}
}

// TV Mode
void CNeutrinoApp::tvMode( bool rezap )
{
	if(mode == mode_radio ) 
	{
#if ENABLE_RADIOTEXT	  
		if (g_settings.radiotext_enable && g_Radiotext) 
		{
			delete g_Radiotext;
			g_Radiotext = NULL;
		}
#endif		

		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_TV_MODE, false );

		CVFD::getInstance()->ShowIcon(VFD_ICON_RADIO, false);

		StartSubtitles(!rezap);
	}

	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

        if( mode == mode_tv ) 
	{
                return;
	}
	else if( mode == mode_scart )
	{
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_SCART);
	}
	else if( mode == mode_standby ) 
	{
		CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
		
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_ENCODER);
	}

	bool stopauto = (mode != mode_ts);
	mode = mode_tv;

	if(stopauto && autoshift) 
	{
		//printf("standby on: autoshift ! stopping ...\n");
		
		stopAutoRecord();
		recordingstatus = 0;
	}

	frameBuffer->useBackground(false);
	frameBuffer->paintBackground();
#ifdef FB_BLIT
	frameBuffer->blit();
#endif

	g_RemoteControl->tvMode();
	
	SetChannelMode( g_settings.channel_mode);//FIXME needed ?

	if( rezap ) 
	{
		firstChannel();
		channelList->tuned = 0xfffffff;;
		channelList->zapTo( firstchannel.channelNumber -1 );
	}
}

// Scart Mode
void CNeutrinoApp::scartMode( bool bOnOff )
{
	printf( ( bOnOff ) ? "mode: scart on\n" : "mode: scart off\n" );

	if( bOnOff ) 
	{
		// SCART AN
		frameBuffer->useBackground(false);
		frameBuffer->paintBackground();
#ifdef FB_BLIT
		frameBuffer->blit();
#endif

		CVFD::getInstance()->setMode(CVFD::MODE_SCART);

		lastMode = mode;
		mode = mode_scart;
	} 
	else 
	{
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_ENCODER);
		
		mode = mode_unknown;
		
		//re-set mode
		if( lastMode == mode_radio ) 
		{
			radioMode( false );
		}
		else if( lastMode == mode_tv ) 
		{
			tvMode( false );
		}
		else if( lastMode == mode_standby ) 
		{
			standbyMode( true );
		}
	}
}

// Standby Mode
void CNeutrinoApp::standbyMode( bool bOnOff )
{
	static bool wasshift = false;

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::standbyMode: recordingstatus (%d) timeshiftstatus(%d) bOnOff (%d)\n", recordingstatus, timeshiftstatus, bOnOff);

	if( bOnOff ) 
	{
		if(autoshift) 
		{
			stopAutoRecord();
			wasshift = true;
			recordingstatus = 0;
		}
		
		if( mode == mode_scart ) 
		{
		}
		
#if ENABLE_RADIOTEXT		
		if (mode == mode_radio && g_settings.radiotext_enable && g_Radiotext != NULL)
		{
			delete g_Radiotext;
			g_Radiotext = NULL;
		}
#endif		

		StopSubtitles();

		frameBuffer->useBackground(false);
		frameBuffer->paintBackground();
#ifdef FB_BLIT
		frameBuffer->blit();
#endif		

		CVFD::getInstance()->Clear();
		
		// show time in vfd
		CVFD::getInstance()->setMode(CVFD::MODE_STANDBY);
		
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_SCART);

		// zapit standby
		if(!recordingstatus && !timeshiftstatus)
		{
			g_Zapit->setStandby(true);
		} 
		else
		{
			//zapit stop playback
			g_Zapit->stopPlayBack();
		}

		// stop sectionsd
		g_Sectionsd->setServiceChanged(0, false);
		g_Sectionsd->setPauseScanning(true);

		//save epg
		if(!recordingstatus && !timeshiftstatus)
		{
			if(g_settings.epg_save) 
			{
				saveEpg();
			}
		}

		//run script
		puts("CNeutrinoApp::standbyMode: executing " NEUTRINO_ENTER_STANDBY_SCRIPT ".");
		if (system(NEUTRINO_ENTER_STANDBY_SCRIPT) != 0)
			perror(NEUTRINO_ENTER_STANDBY_SCRIPT " failed");

		lastMode = mode;
		mode = mode_standby;

		frameBuffer->setActive(false);
			
		// set fan off
#if !ENABLE_LCD		
		CVFD::getInstance()->setFan(false);
#endif		
	} 
	else 
	{	
		// set fan on
#if !ENABLE_LCD		
		CVFD::getInstance()->setFan(true);
#endif		

		// set fb active
		frameBuffer->setActive(true);

		puts("CNeutrinoApp::standbyMode: executing " NEUTRINO_LEAVE_STANDBY_SCRIPT ".");
		if (system(NEUTRINO_LEAVE_STANDBY_SCRIPT) != 0)
			perror(NEUTRINO_LEAVE_STANDBY_SCRIPT " failed");

		// vfd mode
		CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
		
#if defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_GIGABLUE)		
		char tmp[5];
		
		sprintf(tmp, "%04d", channelList->getActiveChannelNumber() );
		
		CVFD::getInstance()->ShowText(tmp); // UTF-8
#endif		

		// zapit startplayback
		g_Zapit->setStandby(false);

		// this is buggy don't respect parentallock
		if(!recordingstatus && !timeshiftstatus)
			g_Zapit->startPlayBack();

		// video wake up
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_ENCODER);

		g_Sectionsd->setPauseScanning(false);
		g_Sectionsd->setServiceChanged(live_channel_id&0xFFFFFFFFFFFFULL, true );

		// setmode?radio:tv
		mode = mode_unknown;

		if( lastMode == mode_radio ) 
		{
			radioMode( false );
		} 
		else 
		{
			tvMode( false );
		}

		// set vol (saved)
		AudioMute(current_muted, false );

		// start record if
		if((mode == mode_tv) && wasshift) 
		{
			startAutoRecord(true);
		}

		wasshift = false;

		StartSubtitles();
	}
}

// Radio Mode
void CNeutrinoApp::radioMode( bool rezap)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::radioMode: rezap %s\n", rezap ? "yes" : "no");

	if(mode == mode_tv ) 
	{
		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_RADIO_MODE, false );

		StopSubtitles();
	}

	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

	if( mode == mode_radio ) 
	{
		return;
	}
	else if( mode == mode_scart ) 
	{
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_SCART);
	}
	else if( mode == mode_standby ) 
	{
#if 1	  
		CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
#endif		
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_ENCODER);
	}

	mode = mode_radio;

	if(autoshift) 
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::radioMode: standby on: autoshift ! stopping ...\n");
		stopAutoRecord();
		recordingstatus = 0;
	}

	g_RemoteControl->radioMode();
	SetChannelMode( g_settings.channel_mode);//FIXME needed?

	if( rezap ) 
	{
		firstChannel();
		channelList->tuned = 0xfffffff;;
		channelList->zapTo( firstchannel.channelNumber -1 );
	}

	frameBuffer->loadBackgroundPic("radio.jpg");
#ifdef FB_BLIT	
	frameBuffer->blit();
#endif	

#if ENABLE_RADIOTEXT
	if (g_settings.radiotext_enable) 
	{
		g_Radiotext = new CRadioText;
	}
#endif	
}

// start next recording
void CNeutrinoApp::startNextRecording()
{
	if ( nextRecordingInfo != NULL ) 
	{
		bool doRecord = true;
		if (CVCRControl::getInstance()->isDeviceRegistered()) 
		{
			recording_id = nextRecordingInfo->eventID;
			
			if (g_settings.recording_type == RECORDING_FILE) 
			{
				char *recDir = strlen(nextRecordingInfo->recordingDir) > 0 ? nextRecordingInfo->recordingDir : g_settings.network_nfs_recordingdir;

				if (!CFSMounter::isMounted(recDir)) 
				{
					doRecord = false;
					for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++) 
					{
						if (strcmp(g_settings.network_nfs_local_dir[i],recDir) == 0) 
						{
							CFSMounter::MountRes mres =
								CFSMounter::mount(g_settings.network_nfs_ip[i].c_str(), g_settings.network_nfs_dir[i],
										  g_settings.network_nfs_local_dir[i], (CFSMounter::FSType) g_settings.network_nfs_type[i],
										  g_settings.network_nfs_username[i], g_settings.network_nfs_password[i],
										  g_settings.network_nfs_mount_options1[i], g_settings.network_nfs_mount_options2[i]);
										  
							if (mres == CFSMounter::MRES_OK) 
							{
								doRecord = true;
							} 
							else 
							{
								const char * merr = mntRes2Str(mres);
								int msglen = strlen(merr) + strlen(nextRecordingInfo->recordingDir) + 7;
								char msg[msglen];
								strcpy(msg, merr);
								strcat(msg, "\nDir: ");
								strcat(msg, nextRecordingInfo->recordingDir);

								ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, msg); // UTF-8
								doRecord = false;
							}
							break;
						}
					}

					if (!doRecord) 
					{
						// recording dir does not seem to exist in config anymore
						// or an error occured while mounting
						// -> try default dir
						recDir = g_settings.network_nfs_recordingdir;
					
						doRecord = true;
					}
				}

				(static_cast<CVCRControl::CFileDevice*>(recordingdevice))->Directory = std::string(recDir);
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::startNextRecording: start to dir %s\n", recDir);

				CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);
			}
			
			if(doRecord && CVCRControl::getInstance()->Record(nextRecordingInfo))
				recordingstatus = 1;
			else
				recordingstatus = 0;
		}
		else
			puts("CNeutrinoApp::startNextRecording: no recording devices");

		/* Note: CTimerd::RecordingInfo is a class!
		 * What a brilliant idea to send classes via the eventserver!
		 * => typecast to avoid destructor call
		 */
		delete [](unsigned char *)nextRecordingInfo;

		nextRecordingInfo = NULL;
	}
}

// exec, menuitem callback (shutdown)
int CNeutrinoApp::exec(CMenuTarget* parent, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: actionKey: %s\n", actionKey.c_str());

	int returnval = menu_return::RETURN_REPAINT;

	if(actionKey == "help_recording") 
	{
		ShowLocalizedMessage(LOCALE_SETTINGS_HELP, LOCALE_RECORDINGMENU_HELP, CMessageBox::mbrBack, CMessageBox::mbBack);
	}
	else if(actionKey=="shutdown") 
	{
		ExitRun(1);
	}
	else if(actionKey=="reboot")
	{
		FILE *f = fopen("/tmp/.reboot", "w");
		fclose(f);

		ExitRun();

		unlink("/tmp/.reboot");
		returnval = menu_return::RETURN_NONE;
	}
	else if(actionKey=="tv") 
	{
		tvMode();
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="radio") 
	{
		radioMode();
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="scart") 
	{
		g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
		returnval = menu_return::RETURN_EXIT_ALL;
	}	
	else if(actionKey=="network") 
	{
		networkConfig.automatic_start = (network_automatic_start == 1);
		networkConfig.stopNetwork();
		networkConfig.commitConfig();
		networkConfig.startNetwork();
	}
	else if(actionKey=="networktest") 
	{
		dprintf(DEBUG_INFO, "CNeutrinoApp::exec: doing network test...\n");
		testNetworkSettings(networkConfig.address.c_str(), networkConfig.netmask.c_str(), networkConfig.broadcast.c_str(), networkConfig.gateway.c_str(), networkConfig.nameserver.c_str(), networkConfig.inet_static);
	}
	else if(actionKey=="networkshow") 
	{
		dprintf(DEBUG_INFO, "CNeutrinoApp::exec: showing current network settings...\n");
		showCurrentNetworkSettings();
	}
	else if (actionKey=="theme_neutrino") 
	{
		setupColors_neutrino();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if (actionKey=="theme_classic") 
	{
		setupColors_classic();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if (actionKey=="theme_dvb2k") 
	{
		setupColors_dvb2k();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if(actionKey=="theme_ru") 
	{
		setupColors_ru();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if(actionKey=="theme_red") 
	{
		setupColors_red();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if(actionKey=="theme_nhd2") 
	{
		setupColors_nhd2();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if(actionKey=="savesettings") 
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MAINSETTINGS_SAVESETTINGSNOW_HINT)); // UTF-8
		hintBox->paint();

		networkConfig.automatic_start = (network_automatic_start == 1);
		networkConfig.commitConfig();
		
		saveSetup(NEUTRINO_SETTINGS_FILE);

		tuxtxt_close();
		
		zapitCfg.saveLastChannel = g_settings.uselastchannel;
		setZapitConfig(&zapitCfg);

		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="recording") 
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MAINSETTINGS_SAVESETTINGSNOW_HINT)); // UTF-8
		hintBox->paint();
		
		setupRecordingDevice();
		
		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="reloadchannels") 
	{
		if(!reloadhintBox)
			reloadhintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_RELOAD_HINT));

		reloadhintBox->paint();
		
		g_Zapit->reinitChannels();	//we don't need the reloadhint box g_zapit reinit channels apple evt bouquets change 
		
		reloadhintBox->hide();
	}
	else if(actionKey=="reloadplugins") 
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_GETPLUGINS_HINT));
		hintBox->paint();

		g_PluginList->loadPlugins();

		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="restart") 
	{
		if (recordingstatus)
			DisplayErrorMessage(g_Locale->getText(LOCALE_SERVICEMENU_RESTART_REFUSED_RECORDING));
		else 
		{
			CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_RESTART_HINT));
			hintBox->paint();

			stop_daemons();

			delete g_RCInput;
			delete g_Sectionsd;
			delete g_Timerd;
			delete g_RemoteControl;
			delete g_fontRenderer;
			delete g_Zapit;
			delete CVFD::getInstance();

			networkConfig.automatic_start = (network_automatic_start == 1);
			networkConfig.commitConfig();
			
			saveSetup(NEUTRINO_SETTINGS_FILE);

			delete frameBuffer;
			
			execvp(global_argv[0], global_argv); // no return if successful
			exit(1);
		}
	}
	else if(actionKey=="osd.def") 
	{
		for (int i = 0; i < TIMING_SETTING_COUNT; i++)
			g_settings.timing[i] = default_timing[i];

		SetupTiming();
	}
	else if(actionKey == "audioplayerdir") 
	{
		parent->hide();
		
		CFileBrowser b;
		b.Dir_Mode=true;
		
		if (b.exec(g_settings.network_nfs_audioplayerdir))
			strncpy(g_settings.network_nfs_audioplayerdir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_audioplayerdir)-1);
		
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "picturedir") 
	{
		parent->hide();
		
		CFileBrowser b;
		b.Dir_Mode=true;
		
		if (b.exec(g_settings.network_nfs_picturedir))
			strncpy(g_settings.network_nfs_picturedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_picturedir)-1);
		
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "moviedir") 
	{
		parent->hide();
		
		CFileBrowser b;
		b.Dir_Mode=true;

		if (b.exec(g_settings.network_nfs_moviedir))
			strncpy(g_settings.network_nfs_moviedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_moviedir)-1);

		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "recordingdir") 
	{
		parent->hide();
		
		CFileBrowser b;
		b.Dir_Mode=true;

		if (b.exec(g_settings.network_nfs_recordingdir)) 
		{
			const char * newdir = b.getSelectedFile()->Name.c_str();
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: New recordingdir: selected %s\n", newdir);

			if(check_dir(newdir))
				printf("CNeutrinoApp::exec: Wrong/unsupported recording dir %s\n", newdir);
			else
			{
				strncpy(g_settings.network_nfs_recordingdir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_recordingdir)-1 );
				
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: New recordingdir: %s\n", g_settings.network_nfs_recordingdir);
				
				sprintf(timeshiftDir, "%s/.timeshift", g_settings.network_nfs_recordingdir);
					
				safe_mkdir(timeshiftDir);
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: New timeshift dir: %s\n", timeshiftDir);
			}
		}
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "update_dir") 
	{
		parent->hide();
		
		CFileBrowser fileBrowser;
		fileBrowser.Dir_Mode = true;
		
		if (fileBrowser.exec(g_settings.update_dir) == true) 
		{
			const char * newdir = fileBrowser.getSelectedFile()->Name.c_str();
			if(check_dir(newdir))
				printf("CNeutrinoApp::exec: Wrong/unsupported update dir %s\n", newdir);
			else
			{
				strcpy(g_settings.update_dir, fileBrowser.getSelectedFile()->Name.c_str());
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: new update dir %s\n", fileBrowser.getSelectedFile()->Name.c_str());
			}
		}
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "epgdir") 
	{
		parent->hide();
		
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if ( b.exec(g_settings.epg_dir.c_str())) 
		{
			const char * newdir = b.getSelectedFile()->Name.c_str();
			if(check_dir(newdir))
				printf("CNeutrinoApp::exec: Wrong/unsupported epg dir %s\n", newdir);
			else
			{
				g_settings.epg_dir = b.getSelectedFile()->Name;
				//strcpy(g_settings.epg_dir.c_str(), b.getSelectedFile()->Name.c_str());
				SendSectionsdConfig();
			}
		}

		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "logos_dir") 
	{
		parent->hide();
		
		CFileBrowser b;
		b.Dir_Mode=true;
		
		if (b.exec(g_settings.logos_dir.c_str())) 
		{
			const char * newdir = b.getSelectedFile()->Name.c_str();
			if(check_dir(newdir))
				printf("CNeutrinoApp::exec: Wrong/unsupported logos dir %s\n", newdir);
			else
			{
				g_settings.logos_dir = b.getSelectedFile()->Name;
				//strcpy(g_settings.logos_dir.c_str(), b.getSelectedFile()->Name.c_str());
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: new logos dir %s\n", b.getSelectedFile()->Name.c_str());
			}
		}

		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "loadcolors") 
	{
		parent->hide();
		
		CFileBrowser fileBrowser;
		CFileFilter fileFilter;
		fileFilter.addFilter("conf");
		fileBrowser.Filter = &fileFilter;
		
		if (fileBrowser.exec("/var/tuxbox/config") == true) 
		{
			loadColors(fileBrowser.getSelectedFile()->Name.c_str());
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: new colors: %s\n", fileBrowser.getSelectedFile()->Name.c_str());
		}
		
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "savecolors") 
	{
		parent->hide();
		
		CFileBrowser fileBrowser;
		fileBrowser.Dir_Mode = true;

		if (fileBrowser.exec("/var/tuxbox") == true) 
		{
			char  fname[256] = "colors.conf", sname[256];
			CStringInputSMS * sms = new CStringInputSMS(LOCALE_EXTRA_SAVECOLORS, fname, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789. ");
			sms->exec(NULL, "");
			sprintf(sname, "%s/%s", fileBrowser.getSelectedFile()->Name.c_str(), fname);
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: save colors: %s\n", sname);
			saveColors(sname);
			delete sms;
		}
		
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "loadkeys") 
	{
		parent->hide();
		
		CFileBrowser fileBrowser;
		CFileFilter fileFilter;
		fileFilter.addFilter("conf");
		fileBrowser.Filter = &fileFilter;
		
		if (fileBrowser.exec("/var/tuxbox/config") == true) 
		{
			loadKeys(fileBrowser.getSelectedFile()->Name.c_str());
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: new keys: %s\n", fileBrowser.getSelectedFile()->Name.c_str());
		}
		
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "savekeys") 
	{
		parent->hide();
		
		CFileBrowser fileBrowser;
		fileBrowser.Dir_Mode = true;
		
		if (fileBrowser.exec("/var/tuxbox") == true) 
		{
			char  fname[256] = "keys.conf", sname[256];
			CStringInputSMS * sms = new CStringInputSMS(LOCALE_EXTRA_SAVEKEYS, fname, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789. ");
			sms->exec(NULL, "");
			sprintf(sname, "%s/%s", fileBrowser.getSelectedFile()->Name.c_str(), fname);
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: save keys: %s\n", sname);
			saveKeys(sname);
			delete sms;
		}
		
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "clearSectionsd")
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, (char *)"clear EPG Cache..." );
		hintBox->paint();
		
		g_Sectionsd->freeMemory();
		
		hintBox->hide();
		delete hintBox;
	} 
	else if(actionKey == "setfptime")
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, (char *)"setting fp time..." );
		hintBox->paint();
		
#ifdef __sh__
		CVFD::getInstance()->setFPTime();
#endif
		
		sleep(2);
		
		hintBox->hide();
		delete hintBox;

		return menu_return::RETURN_REPAINT;	
	}

	return returnval;
}

// changeNotify - features menu recording start / stop
bool CNeutrinoApp::changeNotify(const neutrino_locale_t OptionName, void *data)
{
	if ((ARE_LOCALES_EQUAL(OptionName, LOCALE_MAINMENU_RECORDING_START)) || (ARE_LOCALES_EQUAL(OptionName, LOCALE_MAINMENU_RECORDING)))
	{
		if(g_RemoteControl->is_video_started) 
		{
			bool res = doGuiRecord(NULL, ARE_LOCALES_EQUAL(OptionName, LOCALE_MAINMENU_RECORDING));
			return res;
		}
		else 
		{
			recordingstatus = 0;
			return false;
		}
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_LANGUAGESETUP_SELECT)) 
	{
		g_Locale->loadLocale(g_settings.language);
		return true;
	}

	return false;
}

void stop_daemons()
{
	// stop dvbsub
	dvbsub_close();

	// stop txt
	tuxtxt_stop();
	tuxtxt_close();
	
#ifdef ENABLE_GRAPHLCD
	nGLCD::Exit();
#endif	

	// stop nhttpd	
	dprintf(DEBUG_NORMAL, "stop_daemons: httpd shutdown\n");
	pthread_cancel(nhttpd_thread);
	pthread_join(nhttpd_thread, NULL);
	dprintf(DEBUG_NORMAL, "stop_daemons: httpd shutdown done\n");	

	// streamts	
	streamts_stop = 1;
	pthread_join(stream_thread, NULL);	

	// timerd stop	  
	dprintf(DEBUG_NORMAL, "stop_daemons: timerd shutdown\n");
	g_Timerd->shutdown();
	pthread_join(timer_thread, NULL);
	dprintf(DEBUG_NORMAL, "stop_daemons: timerd shutdown done\n");		

	// sectionsd stop
	sectionsd_stop = 1;
	dprintf(DEBUG_NORMAL, "stop_daemons: sectionsd shutdown\n");
	pthread_join(sections_thread, NULL);
	dprintf(DEBUG_NORMAL, "stop_daemons: sectionsd shutdown done\n");

	tuxtx_stop_subtitle();

	// zapit stop	
	dprintf(DEBUG_NORMAL, "stop_daemons: zapit shutdown\n");
	g_Zapit->shutdown();
	pthread_join(zapit_thread, NULL);
	dprintf(DEBUG_NORMAL, "stop_daemons: zapit shutdown done\n");	

	// clear vfd
	CVFD::getInstance()->Clear();

	// movieplayerGui
	delete moviePlayerGui;
}

// load color
void CNeutrinoApp::loadColors(const char * fname)
{
	bool res;
	CConfigFile tconfig(',', true);

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::loadColors: %s\n", fname);

	res = tconfig.loadConfig(fname);
	if(!res) 
		return;
	
	//colors (neutrino defaultcolors)
	g_settings.menu_Head_alpha = tconfig.getInt32( "menu_Head_alpha", 0x00 );
	g_settings.menu_Head_red = tconfig.getInt32( "menu_Head_red", 0x00 );
	g_settings.menu_Head_green = tconfig.getInt32( "menu_Head_green", 0x0A );
	g_settings.menu_Head_blue = tconfig.getInt32( "menu_Head_blue", 0x19 );

	g_settings.menu_Head_Text_alpha = tconfig.getInt32( "menu_Head_Text_alpha", 0x00 );
	g_settings.menu_Head_Text_red = tconfig.getInt32( "menu_Head_Text_red", 0x5f );
	g_settings.menu_Head_Text_green = tconfig.getInt32( "menu_Head_Text_green", 0x46 );
	g_settings.menu_Head_Text_blue = tconfig.getInt32( "menu_Head_Text_blue", 0x00 );

	g_settings.menu_Content_alpha = tconfig.getInt32( "menu_Content_alpha", 0x14 );
	g_settings.menu_Content_red = tconfig.getInt32( "menu_Content_red", 0x00 );
	g_settings.menu_Content_green = tconfig.getInt32( "menu_Content_green", 0x0f );
	g_settings.menu_Content_blue = tconfig.getInt32( "menu_Content_blue", 0x23 );

	g_settings.menu_Content_Text_alpha = tconfig.getInt32( "menu_Content_Text_alpha", 0x00 );
	g_settings.menu_Content_Text_red = tconfig.getInt32( "menu_Content_Text_red", 0x64 );
	g_settings.menu_Content_Text_green = tconfig.getInt32( "menu_Content_Text_green", 0x64 );
	g_settings.menu_Content_Text_blue = tconfig.getInt32( "menu_Content_Text_blue", 0x64 );

	g_settings.menu_Content_Selected_alpha = tconfig.getInt32( "menu_Content_Selected_alpha", 0x14 );
	g_settings.menu_Content_Selected_red = tconfig.getInt32( "menu_Content_Selected_red", 0x19 );
	g_settings.menu_Content_Selected_green = tconfig.getInt32( "menu_Content_Selected_green", 0x37 );
	g_settings.menu_Content_Selected_blue = tconfig.getInt32( "menu_Content_Selected_blue", 0x64 );

	g_settings.menu_Content_Selected_Text_alpha = tconfig.getInt32( "menu_Content_Selected_Text_alpha", 0x00 );
	g_settings.menu_Content_Selected_Text_red = tconfig.getInt32( "menu_Content_Selected_Text_red", 0x00 );
	g_settings.menu_Content_Selected_Text_green = tconfig.getInt32( "menu_Content_Selected_Text_green", 0x00 );
	g_settings.menu_Content_Selected_Text_blue = tconfig.getInt32( "menu_Content_Selected_Text_blue", 0x00 );

	g_settings.menu_Content_inactive_alpha = tconfig.getInt32( "menu_Content_inactive_alpha", 0x14 );
	g_settings.menu_Content_inactive_red = tconfig.getInt32( "menu_Content_inactive_red", 0x00 );
	g_settings.menu_Content_inactive_green = tconfig.getInt32( "menu_Content_inactive_green", 0x0f );
	g_settings.menu_Content_inactive_blue = tconfig.getInt32( "menu_Content_inactive_blue", 0x23 );

	g_settings.menu_Content_inactive_Text_alpha = tconfig.getInt32( "menu_Content_inactive_Text_alpha", 0x00 );
	g_settings.menu_Content_inactive_Text_red = tconfig.getInt32( "menu_Content_inactive_Text_red", 55 );
	g_settings.menu_Content_inactive_Text_green = tconfig.getInt32( "menu_Content_inactive_Text_green", 70 );
	g_settings.menu_Content_inactive_Text_blue = tconfig.getInt32( "menu_Content_inactive_Text_blue", 85 );

	g_settings.infobar_alpha = tconfig.getInt32( "infobar_alpha", 0x14 );
	g_settings.infobar_red = tconfig.getInt32( "infobar_red", 0x00 );
	g_settings.infobar_green = tconfig.getInt32( "infobar_green", 0x0e );
	g_settings.infobar_blue = tconfig.getInt32( "infobar_blue", 0x23 );

	g_settings.infobar_Text_alpha = tconfig.getInt32( "infobar_Text_alpha", 0x00 );
	g_settings.infobar_Text_red = tconfig.getInt32( "infobar_Text_red", 0x64 );
	g_settings.infobar_Text_green = tconfig.getInt32( "infobar_Text_green", 0x64 );
	g_settings.infobar_Text_blue = tconfig.getInt32( "infobar_Text_blue", 0x64 );
	
	colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
}

// save color
void CNeutrinoApp::saveColors(const char * fname)
{
	CConfigFile tconfig(',', true);
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::saveColors: %s\n", fname);

	//colors
	tconfig.setInt32( "menu_Head_alpha", g_settings.menu_Head_alpha );
	tconfig.setInt32( "menu_Head_red", g_settings.menu_Head_red );
	tconfig.setInt32( "menu_Head_green", g_settings.menu_Head_green );
	tconfig.setInt32( "menu_Head_blue", g_settings.menu_Head_blue );

	tconfig.setInt32( "menu_Head_Text_alpha", g_settings.menu_Head_Text_alpha );
	tconfig.setInt32( "menu_Head_Text_red", g_settings.menu_Head_Text_red );
	tconfig.setInt32( "menu_Head_Text_green", g_settings.menu_Head_Text_green );
	tconfig.setInt32( "menu_Head_Text_blue", g_settings.menu_Head_Text_blue );

	tconfig.setInt32( "menu_Content_alpha", g_settings.menu_Content_alpha );
	tconfig.setInt32( "menu_Content_red", g_settings.menu_Content_red );
	tconfig.setInt32( "menu_Content_green", g_settings.menu_Content_green );
	tconfig.setInt32( "menu_Content_blue", g_settings.menu_Content_blue );

	tconfig.setInt32( "menu_Content_Text_alpha", g_settings.menu_Content_Text_alpha );
	tconfig.setInt32( "menu_Content_Text_red", g_settings.menu_Content_Text_red );
	tconfig.setInt32( "menu_Content_Text_green", g_settings.menu_Content_Text_green );
	tconfig.setInt32( "menu_Content_Text_blue", g_settings.menu_Content_Text_blue );

	tconfig.setInt32( "menu_Content_Selected_alpha", g_settings.menu_Content_Selected_alpha );
	tconfig.setInt32( "menu_Content_Selected_red", g_settings.menu_Content_Selected_red );
	tconfig.setInt32( "menu_Content_Selected_green", g_settings.menu_Content_Selected_green );
	tconfig.setInt32( "menu_Content_Selected_blue", g_settings.menu_Content_Selected_blue );

	tconfig.setInt32( "menu_Content_Selected_Text_alpha", g_settings.menu_Content_Selected_Text_alpha );
	tconfig.setInt32( "menu_Content_Selected_Text_red", g_settings.menu_Content_Selected_Text_red );
	tconfig.setInt32( "menu_Content_Selected_Text_green", g_settings.menu_Content_Selected_Text_green );
	tconfig.setInt32( "menu_Content_Selected_Text_blue", g_settings.menu_Content_Selected_Text_blue );

	tconfig.setInt32( "menu_Content_inactive_alpha", g_settings.menu_Content_inactive_alpha );
	tconfig.setInt32( "menu_Content_inactive_red", g_settings.menu_Content_inactive_red );
	tconfig.setInt32( "menu_Content_inactive_green", g_settings.menu_Content_inactive_green );
	tconfig.setInt32( "menu_Content_inactive_blue", g_settings.menu_Content_inactive_blue );

	tconfig.setInt32( "menu_Content_inactive_Text_alpha", g_settings.menu_Content_inactive_Text_alpha );
	tconfig.setInt32( "menu_Content_inactive_Text_red", g_settings.menu_Content_inactive_Text_red );
	tconfig.setInt32( "menu_Content_inactive_Text_green", g_settings.menu_Content_inactive_Text_green );
	tconfig.setInt32( "menu_Content_inactive_Text_blue", g_settings.menu_Content_inactive_Text_blue );

	tconfig.setInt32( "infobar_alpha", g_settings.infobar_alpha );
	tconfig.setInt32( "infobar_red", g_settings.infobar_red );
	tconfig.setInt32( "infobar_green", g_settings.infobar_green );
	tconfig.setInt32( "infobar_blue", g_settings.infobar_blue );

	tconfig.setInt32( "infobar_Text_alpha", g_settings.infobar_Text_alpha );
	tconfig.setInt32( "infobar_Text_red", g_settings.infobar_Text_red );
	tconfig.setInt32( "infobar_Text_green", g_settings.infobar_Text_green );
	tconfig.setInt32( "infobar_Text_blue", g_settings.infobar_Text_blue );	
	
	tconfig.saveConfig(fname);
}

// load keys
void CNeutrinoApp::loadKeys(const char * fname)
{
	bool res;
	CConfigFile tconfig(',', true);
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::loadKeys: %s\n", fname);

	res = tconfig.loadConfig(fname);
	if(!res) 
		return;

	//rc-key configuration
	g_settings.key_tvradio_mode = tconfig.getInt32( "key_tvradio_mode", CRCInput::RC_mode );

	g_settings.key_channelList_pageup = tconfig.getInt32( "key_channelList_pageup",  CRCInput::RC_minus );
	g_settings.key_channelList_pagedown = tconfig.getInt32( "key_channelList_pagedown", CRCInput::RC_plus );
	g_settings.key_channelList_cancel = tconfig.getInt32( "key_channelList_cancel",  CRCInput::RC_home );
	//g_settings.key_channelList_sort = tconfig.getInt32( "key_channelList_sort",  CRCInput::RC_blue );
	g_settings.key_channelList_reload = tconfig.getInt32( "key_channelList_reload",  CRCInput::RC_blue );
	g_settings.key_channelList_addrecord = tconfig.getInt32( "key_channelList_addrecord",  CRCInput::RC_red );
	g_settings.key_channelList_addremind = tconfig.getInt32( "key_channelList_addremind",  CRCInput::RC_yellow );

	g_settings.key_list_start = tconfig.getInt32( "key_list_start", CRCInput::RC_nokey );
	g_settings.key_list_end = tconfig.getInt32( "key_list_end", CRCInput::RC_nokey );
	g_settings.menu_left_exit = tconfig.getInt32( "menu_left_exit", 0 );

	g_settings.mpkey_rewind = tconfig.getInt32( "mpkey.rewind", CRCInput::RC_left );
	g_settings.mpkey_forward = tconfig.getInt32( "mpkey.forward", CRCInput::RC_right );
	g_settings.mpkey_pause = tconfig.getInt32( "mpkey.pause", CRCInput::RC_yellow );
	g_settings.mpkey_stop = tconfig.getInt32( "mpkey.stop", CRCInput::RC_home );
	g_settings.mpkey_play = tconfig.getInt32( "mpkey.play", CRCInput::RC_ok );
	g_settings.mpkey_audio = tconfig.getInt32( "mpkey.audio", CRCInput::RC_green );
	g_settings.mpkey_time = tconfig.getInt32( "mpkey.time", CRCInput::RC_setup );
	g_settings.mpkey_bookmark = tconfig.getInt32( "mpkey.bookmark", CRCInput::RC_blue );
	g_settings.key_timeshift = configfile.getInt32( "key_timeshift", CRCInput::RC_nokey );

	g_settings.key_quickzap_up = tconfig.getInt32( "key_quickzap_up",  CRCInput::RC_up );
	g_settings.key_quickzap_down = tconfig.getInt32( "key_quickzap_down",  CRCInput::RC_down );
	g_settings.key_subchannel_up = tconfig.getInt32( "key_subchannel_up",  CRCInput::RC_right );
	g_settings.key_subchannel_down = tconfig.getInt32( "key_subchannel_down",  CRCInput::RC_left );
	g_settings.key_zaphistory = tconfig.getInt32( "key_zaphistory",  CRCInput::RC_home );
	g_settings.key_lastchannel = tconfig.getInt32( "key_lastchannel",  CRCInput::RC_0 );

	g_settings.key_bouquet_up = tconfig.getInt32( "key_bouquet_up",  CRCInput::RC_right);
	g_settings.key_bouquet_down = tconfig.getInt32( "key_bouquet_down",  CRCInput::RC_left);
	strcpy(g_settings.repeat_blocker, tconfig.getString("repeat_blocker", "300").c_str());
	strcpy(g_settings.repeat_genericblocker, tconfig.getString("repeat_genericblocker", "100").c_str());
}

// save keys
void CNeutrinoApp::saveKeys(const char * fname)
{
	CConfigFile tconfig(',', true);
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::saveKeys: %s\n", fname);
	
	//rc-key configuration
	tconfig.setInt32( "key_tvradio_mode", g_settings.key_tvradio_mode );

	tconfig.setInt32( "key_channelList_pageup", g_settings.key_channelList_pageup );
	tconfig.setInt32( "key_channelList_pagedown", g_settings.key_channelList_pagedown );
	tconfig.setInt32( "key_channelList_cancel", g_settings.key_channelList_cancel );
	//tconfig.setInt32( "key_channelList_sort", g_settings.key_channelList_sort );
	tconfig.setInt32( "key_channelList_reload", g_settings.key_channelList_reload );
	tconfig.setInt32( "key_channelList_addrecord", g_settings.key_channelList_addrecord );
	tconfig.setInt32( "key_channelList_addremind", g_settings.key_channelList_addremind );

	tconfig.setInt32( "key_quickzap_up", g_settings.key_quickzap_up );
	tconfig.setInt32( "key_quickzap_down", g_settings.key_quickzap_down );
	tconfig.setInt32( "key_bouquet_up", g_settings.key_bouquet_up );
	tconfig.setInt32( "key_bouquet_down", g_settings.key_bouquet_down );
	tconfig.setInt32( "key_subchannel_up", g_settings.key_subchannel_up );
	tconfig.setInt32( "key_subchannel_down", g_settings.key_subchannel_down );
	tconfig.setInt32( "key_zaphistory", g_settings.key_zaphistory );
	tconfig.setInt32( "key_lastchannel", g_settings.key_lastchannel );

	tconfig.setInt32( "key_list_start", g_settings.key_list_start );
	tconfig.setInt32( "key_list_end", g_settings.key_list_end );
	
	tconfig.setInt32( "menu_left_exit", g_settings.menu_left_exit );

	tconfig.setInt32( "mpkey.rewind", g_settings.mpkey_rewind );
	tconfig.setInt32( "mpkey.forward", g_settings.mpkey_forward );
	tconfig.setInt32( "mpkey.pause", g_settings.mpkey_pause );
	tconfig.setInt32( "mpkey.stop", g_settings.mpkey_stop );
	tconfig.setInt32( "mpkey.play", g_settings.mpkey_play );
	tconfig.setInt32( "mpkey.audio", g_settings.mpkey_audio );
	tconfig.setInt32( "mpkey.time", g_settings.mpkey_time );
	tconfig.setInt32( "mpkey.bookmark", g_settings.mpkey_bookmark );
	tconfig.setInt32( "key_timeshift", g_settings.key_timeshift );

	tconfig.setInt32( "key_unlock", g_settings.key_unlock );

	tconfig.setString( "repeat_blocker", g_settings.repeat_blocker );
	tconfig.setString( "repeat_genericblocker", g_settings.repeat_genericblocker );

	tconfig.saveConfig(fname);
}

// stop subtitle
void CNeutrinoApp::StopSubtitles()
{
	printf("[neutrino] %s\n", __FUNCTION__);
	
	int ttx, ttxpid, ttxpage;

	// dvbsub
	int dvbpid = dvbsub_getpid();
	
	if(dvbpid)
	{
		dvbsub_pause();
	}
	
	// tuxtxt
	tuxtx_subtitle_running(&ttxpid, &ttxpage, &ttx);
	
	if(ttx) 
	{
		tuxtx_pause_subtitle(true, live_fe?live_fe->fenumber:0 );
		
		frameBuffer->paintBackground();
#ifdef FB_BLIT
		frameBuffer->blit();
#endif
	}
	
#if ENABLE_GRAPHLCD	
	nGLCD::MirrorOSD();
#endif
}

// start subtitle
void CNeutrinoApp::StartSubtitles(bool show)
{
	printf("%s: %s\n", __FUNCTION__, show ? "Show" : "Not show");
	
#if ENABLE_GRAPHLCD
	nGLCD::MirrorOSD(false);
#endif	
	
	if(!show)
		return;
	
	//dvbsub
	dvbsub_start(dvbsub_getpid());
	
	// tuxtxt
	tuxtx_pause_subtitle( false, live_fe?live_fe->fenumber:0 );
}

// select subtitle
void CNeutrinoApp::SelectSubtitles()
{
	if(!g_settings.auto_subs)
		return;

	int curnum = channelList->getActiveChannelNumber();
	CZapitChannel * cc = channelList->getChannel(curnum);

	for(int i = 0; i < 3; i++) 
	{
		if(strlen(g_settings.pref_subs[i]) == 0)
			continue;

		std::string temp(g_settings.pref_subs[i]);

		for(int j = 0 ; j < (int)cc->getSubtitleCount() ; j++) 
		{
			CZapitAbsSub* s = cc->getChannelSub(j);
			if (s->thisSubType == CZapitAbsSub::DVB) 
			{
				CZapitDVBSub* sd = reinterpret_cast<CZapitDVBSub*>(s);
				std::map<std::string, std::string>::const_iterator it;
				for(it = iso639.begin(); it != iso639.end(); it++) 
				{
					if(temp == it->second && sd->ISO639_language_code == it->first) 
					{
						printf("CNeutrinoApp::SelectSubtitles: found DVB %s, pid %x\n", sd->ISO639_language_code.c_str(), sd->pId);
						dvbsub_stop();
						dvbsub_setpid(sd->pId);
						return;
					}
				}
			}
		}
		
		for(int j = 0 ; j < (int)cc->getSubtitleCount() ; j++) 
		{
			CZapitAbsSub* s = cc->getChannelSub(j);
			if (s->thisSubType == CZapitAbsSub::TTX) 
			{
				CZapitTTXSub* sd = reinterpret_cast<CZapitTTXSub*>(s);
				std::map<std::string, std::string>::const_iterator it;
				for(it = iso639.begin(); it != iso639.end(); it++) 
				{
					if(temp == it->second && sd->ISO639_language_code == it->first) 
					{
						int page = ((sd->teletext_magazine_number & 0xFF) << 8) | sd->teletext_page_number;
						printf("CNeutrinoApp::SelectSubtitles: found TTX %s, pid %x page %03X\n", sd->ISO639_language_code.c_str(), sd->pId, page);
						tuxtx_stop_subtitle();
						tuxtx_set_pid(sd->pId, page, (char *) sd->ISO639_language_code.c_str());
						return;
					}
				}
			}
		}
	}
}

// signal handler
void sighandler (int signum)
{
	signal(signum, SIG_IGN);
	
        switch (signum) 
	{
		case SIGTERM:
		case SIGINT:
			stop_daemons();
			_exit(0);
			
		  default:
			break;
        }
}

// main function
int main(int argc, char *argv[])
{
	// build date
	printf(">>> NeutrinoHD2 (compiled %s %s) <<<\n", __DATE__, __TIME__);
	
	// set debug level (default normal)
	setDebugLevel(DEBUG_INFO);

	// sighandler
        signal(SIGTERM, sighandler);
        signal(SIGINT, sighandler);
        signal(SIGHUP, SIG_IGN);
	
	// from coolstream neutrino
	signal(SIGHUP, sighandler);
	signal(SIGPIPE, SIG_IGN);
	
	// set time convertion
	tzset();
	
	// init globals
	initGlobals();

	char * buf = (char *)malloc(64);
	int count;
	if (buf && (count = readlink("/proc/self/exe", buf, 63)) >= 0) 
	{
		buf[count] = '\0';
		printf("starting %s\n", buf);
		free(buf);
	}
	
	for(int i = 3; i < 256; i++)
		close(i);

	return CNeutrinoApp::getInstance()->run(argc, argv);
}

