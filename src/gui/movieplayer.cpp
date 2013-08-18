/*
  Neutrino-GUI  -   DBoxII-Project

  Movieplayer (c) 2003, 2004 by gagga
  Based on code by Dirch, obi and the Metzler Bros. Thanks.

  $Id: movieplayer.cpp,v 1.97 2004/07/18 00:54:52 thegoodguy Exp $

  Homepage: http://www.giggo.de/dbox2/movieplayer.html

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

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <sys/mount.h>

#include <gui/movieplayer.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/vcrcontrol.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>

#include <gui/eventlist.h>
#include <gui/color.h>
#include <gui/infoviewer.h>
#include <gui/nfs.h>
#include <gui/timeosd.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/helpbox.h>
#include <system/debug.h>

#include <libxmltree/xmlinterface.h>

#include <algorithm>
#include <fstream>
#include <sstream>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <poll.h>
#include <sys/timeb.h>

/* libdvbapi */
#include <playback_cs.h>
#include <video_cs.h>
#include <audio_cs.h>

/*zapit includes*/
#include <channel.h>

/* curl */
#include <curl/curl.h>
#include <curl/easy.h>


static const char FILENAME[] = "movieplayer.cpp";
 
// vlc
static int streamtype;

#define STREAMTYPE_DVD	1
#define STREAMTYPE_SVCD	2
#define STREAMTYPE_FILE	3

static int skt = -1; //dirty hack to close socket when stop playing

#define MOVIEPLAYER_START_SCRIPT CONFIGDIR "/movieplayer.start" 
#define MOVIEPLAYER_END_SCRIPT CONFIGDIR "/movieplayer.end"

cPlayback * playback = NULL;

//
extern CRemoteControl * g_RemoteControl;		/* neutrino.cpp */
extern CZapitChannel * live_channel;			/* zapit.cpp */
extern CInfoViewer * g_InfoViewer;
extern t_channel_id live_channel_id; 			//defined in zapit.cpp

#define MOVIE_HINT_BOX_TIMER 5				// time to show bookmark hints in seconds

#define MINUTEOFFSET 117*262072
#define MP_TS_SIZE 262072				// ~0.5 sec

extern char rec_filename[512];				// defined in stream2file.cpp

CMoviePlayerGui::state playstate;
bool isMovieBrowser = false;
bool isVlc = false;
bool isWebTV = false;
bool isDVD = false;
bool isBlueRay = false;
bool isURL = false;

int speed = 1;
int slow = 0;

static int position = 0;
static int duration = 0;
static int file_prozent;

int startposition;
int timeshift;
off64_t minuteoffset;
off64_t secondoffset;

#ifndef __USE_FILE_OFFSET64
#error not using 64 bit file offsets
#endif /* __USE_FILE__OFFSET64 */

static int g_jumpseconds = 0;

unsigned short g_apids[10];
unsigned short m_apids[10]; // needed to get language from mb
unsigned short g_ac3flags[10];
unsigned short g_numpida = 0;
unsigned short g_vpid = 0;
unsigned short g_vtype = 0;
std::string    g_language[10];

unsigned int g_currentapid = 0, g_currentac3 = 0, apidchanged = 0;

unsigned int ac3state = CInfoViewer::NO_AC3;

std::string g_file_epg;
std::string g_file_epg1;

bool showaudioselectdialog = false;

#define TIMESHIFT_SECONDS 	3

enum {
	NO_TIMESHIFT,
	TIMESHIFT,
	P_TIMESHIFT
};

extern CVideoSetupNotifier * videoSetupNotifier;	/* defined neutrino.cpp */
// aspect ratio
#ifdef __sh__
#define VIDEOMENU_VIDEORATIO_OPTION_COUNT 2
const CMenuOptionChooser::keyval VIDEOMENU_VIDEORATIO_OPTIONS[VIDEOMENU_VIDEORATIO_OPTION_COUNT] =
{
	{ 0, LOCALE_VIDEOMENU_VIDEORATIO_43 },
	{ 1, LOCALE_VIDEOMENU_VIDEORATIO_169 }
};
#else
#define VIDEOMENU_VIDEORATIO_OPTION_COUNT 3
const CMenuOptionChooser::keyval VIDEOMENU_VIDEORATIO_OPTIONS[VIDEOMENU_VIDEORATIO_OPTION_COUNT] =
{
	{ 0, LOCALE_VIDEOMENU_VIDEORATIO_43, NULL },
	{ 1, LOCALE_VIDEOMENU_VIDEORATIO_169, NULL },
	{ 2, NONEXISTANT_LOCALE, "Auto" }
};
#endif

// policy
#ifdef __sh__
/*
letterbox 
panscan 
non 
bestfit
*/
#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const CMenuOptionChooser::keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] = 
{
	{ 0, LOCALE_VIDEOMENU_LETTERBOX, NULL },
	{ 1, LOCALE_VIDEOMENU_PANSCAN, NULL },
	{ 2, LOCALE_VIDEOMENU_FULLSCREEN, NULL },
	{ 3, LOCALE_VIDEOMENU_PANSCAN2, NULL }
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
	{ 0, LOCALE_VIDEOMENU_LETTERBOX, NULL },
	{ 1, LOCALE_VIDEOMENU_PANSCAN, NULL },
	{ 2, LOCALE_VIDEOMENU_PANSCAN2, NULL },
	{ 3, LOCALE_VIDEOMENU_FULLSCREEN, NULL }
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

bool get_movie_info_apid_name(int apid, MI_MOVIE_INFO * movie_info, std::string * apidtitle)
{
	if (movie_info == NULL || apidtitle == NULL)
		return false;

	for (int i = 0; i < (int)movie_info->audioPids.size(); i++) 
	{
		if (movie_info->audioPids[i].epgAudioPid == apid && !movie_info->audioPids[i].epgAudioPidName.empty()) 
		{
			*apidtitle = movie_info->audioPids[i].epgAudioPidName;
			return true;
		}
	}

	return false;
}

int CAPIDSelectExec::exec(CMenuTarget */*parent*/, const std::string & actionKey)
{
	apidchanged = 0;
	unsigned int sel = atoi(actionKey.c_str());

	if (g_currentapid != g_apids[sel - 1]) 
	{
		g_currentapid = g_apids[sel - 1];
		g_currentac3 = g_ac3flags[sel - 1];
		apidchanged = 1;
		dprintf(DEBUG_NORMAL, "[movieplayer] apid changed to %d\n", g_apids[sel - 1]);
	}

	return menu_return::RETURN_EXIT;
}

// MovieInfoViewer
#define TIMEOSD_FONT 		SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME
#define TIMEBARH 		38
#define BARLEN 			200
#define SHADOW_OFFSET		5
#define LEFT_OFFSET 		5
#define RIGHT_OFFSET 		5
#define TIMESCALE_HEIGHT	6

extern cVideo *videoDecoder;
extern cAudio *audioDecoder;
static CProgressBar * timescale;

CMovieInfoViewer::CMovieInfoViewer()
{
	frameBuffer = CFrameBuffer::getInstance();
	visible = false;
	
	GetDimensions();

	if(!timescale)
		timescale = new CProgressBar( BoxWidth - 15, TIMESCALE_HEIGHT, 40, 100, 70, true );
}

CMovieInfoViewer::~CMovieInfoViewer()
{
	hide();

	if(timescale) 
	{
		delete timescale;
		timescale = 0;
	}
}

void CMovieInfoViewer::show(int Position, bool lshow)
{	
	// show / update
	GetDimensions();
	visible = true;
	
	// timescale
	timescale->reset();
	
	// paint shadow
	frameBuffer->paintBoxRel(BoxStartX + SHADOW_OFFSET, BoxStartY + SHADOW_OFFSET, BoxWidth, BoxHeight, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID, CORNER_BOTH );
		
	// paint info box
	frameBuffer->paintBoxRel(BoxStartX, BoxStartY, BoxWidth, BoxHeight, COL_INFOBAR_PLUS_0, RADIUS_MID, CORNER_BOTH); 
		
	// timescale bg
	frameBuffer->paintBoxRel(BoxStartX + 10, BoxStartY + 15, BoxWidth - 20, 6, COL_INFOBAR_SHADOW_PLUS_1 ); 
		
	// bottum bar
	frameBuffer->paintBoxRel(BoxStartX, BoxStartY + (BoxHeight - 20), BoxWidth, 20, COL_INFOBAR_SHADOW_PLUS_1,  RADIUS_MID, CORNER_BOTTOM); 
		
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
	int icon_w, icon_h;
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_w, &icon_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, BoxStartX + RIGHT_OFFSET, BoxStartY + (BoxHeight - 20) + (20 - icon_h)/2);
	//if( isMovieBrowser || isVlc )
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString( BoxStartX + RIGHT_OFFSET + icon_w + 2, BoxEndY + 2, BoxWidth/5, (char *)"Movie Info", (COL_INFOBAR_SHADOW + 1), 0, true); // UTF-8
		
	// green
	// audio
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_GREEN, &icon_w, &icon_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, BoxStartX + BoxWidth/5, BoxStartY + BoxHeight - 20 + (20 - icon_h)/2);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString( BoxStartX + (BoxWidth/5) + icon_w + 2, BoxEndY + 2, BoxWidth/5, g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES), (COL_INFOBAR_SHADOW + 1), 0, true); // UTF-8
		
	// yellow
	// help
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_YELLOW, &icon_w, &icon_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, BoxStartX + (BoxWidth/5)*2, BoxStartY + (BoxHeight - 20) + (20 - icon_h)/2);
	if( !isWebTV)
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString( BoxStartX + (BoxWidth/5)*2 + icon_w + 2, BoxEndY + 2, BoxWidth/5, (char *)"help", (COL_INFOBAR_SHADOW * 1), 0, true); // UTF-8
		
	// blue
	// bookmark
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_BLUE, &icon_w, &icon_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, BoxStartX + (BoxWidth/5)*3, BoxStartY + (BoxHeight - 20) + (20 - icon_h)/2);
	if(isMovieBrowser)
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString( BoxStartX + (BoxWidth/5)*3 + icon_w + 2, BoxEndY + 2, BoxWidth/5, g_Locale->getText(LOCALE_MOVIEPLAYER_BOOKMARK), (COL_INFOBAR_SHADOW + 1), 0, true); // UTF-8
		
	// ac3
	int icon_w_ac3, icon_h_ac3;
	frameBuffer->getIconSize(NEUTRINO_ICON_DD, &icon_w_ac3, &icon_h_ac3);
	frameBuffer->paintIcon( (ac3state == CInfoViewer::AC3_ACTIVE)?NEUTRINO_ICON_DD : NEUTRINO_ICON_DD_GREY, BoxStartX + BoxWidth - LEFT_OFFSET - icon_w_ac3, BoxStartY + BoxHeight - 20 + (20 - icon_h_ac3)/2);
		
	// 4:3/16:9
	const char * aspect_icon = NEUTRINO_ICON_16_9_GREY;
				
	if(videoDecoder->getAspectRatio() == 1)
		aspect_icon = NEUTRINO_ICON_16_9;
	
	int icon_w_aspect, icon_h_aspect;
	frameBuffer->getIconSize(aspect_icon, &icon_w_aspect, &icon_h_aspect);
	frameBuffer->paintIcon(aspect_icon, BoxStartX + BoxWidth - LEFT_OFFSET - icon_w_ac3 - 2 - icon_w_aspect, BoxStartY + BoxHeight - 20 + (20 - icon_h_aspect)/2);
	
	/* mp keys */
	frameBuffer->getIconSize("ico_mp_rewind", &icon_w, &icon_h);
	frameBuffer->paintIcon("ico_mp_rewind", BoxStartX + BoxWidth - LEFT_OFFSET - icon_w_ac3 - 2 - icon_w_aspect - 2 - 5*icon_w, BoxStartY + BoxHeight - 20 + (20 - icon_h)/2);
	frameBuffer->paintIcon("ico_mp_play", BoxStartX + BoxWidth - LEFT_OFFSET - icon_w_ac3 - 2 - icon_w_aspect - 2 - 4*icon_w, BoxStartY + BoxHeight - 20 + (20 - icon_h)/2);
	frameBuffer->paintIcon("ico_mp_pause", BoxStartX + BoxWidth - LEFT_OFFSET - icon_w_ac3 - 2 - icon_w_aspect - 2 - 3*icon_w, BoxStartY + BoxHeight - 20 + (20 - icon_h)/2);
	frameBuffer->paintIcon("ico_mp_stop", BoxStartX + BoxWidth - LEFT_OFFSET - icon_w_ac3 - 2 - icon_w_aspect - 2 - 2*icon_w, BoxStartY + BoxHeight - 20 + (20 - icon_h)/2);
	frameBuffer->paintIcon("ico_mp_forward", BoxStartX + BoxWidth - LEFT_OFFSET - icon_w_ac3 - 2 - icon_w_aspect - 2 - icon_w, BoxStartY + BoxHeight - 20 + (20 - icon_h)/2);
		
	//playstate
	const char *icon = "mp_play";
		
	switch(playstate)
	{
		case CMoviePlayerGui::PAUSE: icon = "mp_pause"; break;
		case CMoviePlayerGui::PLAY: icon = "mp_play"; break;
		case CMoviePlayerGui::REW: icon = "mp_b-skip"; break;
		case CMoviePlayerGui::FF: icon = "mp_f-skip"; break;
		case CMoviePlayerGui::SOFTRESET: break;
		case CMoviePlayerGui::SLOW: break;
		case CMoviePlayerGui::STOPPED: break;
	}

	// get icon size	
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
	//char runningTotal[32]; // %d can be 10 digits max...	
	//sprintf(runningTotal, "%d / %d min", Position/60, (duration + 30000) / 60000 );	
	//sprintf(runningTotal, "%d min", (duration + 30000) / 60000 );	
	
	//
	time_t tDisplayTime = duration/1000;
	char cDisplayTime[8 + 1];
	strftime(cDisplayTime, 9, "%T", gmtime(&tDisplayTime));
	//
	
		
	int durationWidth = /*g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(runningTotal)*/g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth("00:00:00");;
	int durationTextPos = BoxEndX - durationWidth - 15;
		
	int speedWidth = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth("-8");
		
	int InfoStartX = BoxStartX + 5 + m_icon_w + 10 + icon_w + 5 + speedWidth + 20;
	int InfoWidth = durationTextPos - InfoStartX;
		
	//Title 1
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (InfoStartX, BoxStartY + BoxHeight/2 - 5, InfoWidth, g_file_epg, COL_INFOBAR, 0, true);

	//Title2
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString (InfoStartX, BoxStartY + BoxHeight/2 + 25, InfoWidth, g_file_epg1, COL_INFOBAR, 0, true);

	// duration
	if(!isWebTV && !isVlc && lshow)
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(durationTextPos, BoxStartY + BoxHeight/2 - 5, durationWidth, /*runningTotal*/cDisplayTime, COL_INFOBAR);
	
	updatePos(file_prozent);

#if !defined USE_OPENGL
	frameBuffer->blit();
#endif
}

void CMovieInfoViewer::GetDimensions()
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

void CMovieInfoViewer::updatePos(short runningPercent)
{
	timescale->paint(BoxStartX + 10, BoxStartY + 15, runningPercent);
#if !defined USE_OPENGL
	frameBuffer->blit();
#endif	
}

void CMovieInfoViewer::hide()
{
	GetDimensions();

	if(!visible)
		return;

	// hide infobar
	frameBuffer->paintBackgroundBoxRel(BoxStartX, BoxStartY, BoxWidth + SHADOW_OFFSET, BoxHeight + SHADOW_OFFSET );
	
#if !defined USE_OPENGL
	frameBuffer->blit();
#endif
	visible = false;
	
	timescale->reset();
}

CMoviePlayerGui::CMoviePlayerGui()
{
	Init();
}

void CMoviePlayerGui::Init(void)
{
	stopped = false;

	frameBuffer = CFrameBuffer::getInstance();

	// local path
	if (strlen(g_settings.network_nfs_moviedir) != 0)
		Path_local = g_settings.network_nfs_moviedir;
	else
		Path_local = "/";
	
	// vlc path
	Path_vlc  = "vlc://";
	if ((g_settings.streaming_vlc10 < 2) || (strcmp(g_settings.streaming_server_startdir, "/") != 0))
		Path_vlc += g_settings.streaming_server_startdir;
	Path_vlc_settings = g_settings.streaming_server_startdir;
	
	// dvd path
	Path_dvd = "/mnt/dvd";
	
	Path_blueray = "/mnt/blueray";
	
	// playback
	playback = new cPlayback();
	
	// filebrowser
	if (g_settings.filebrowser_denydirectoryleave)
		filebrowser = new CFileBrowser(Path_local.c_str());
	else
		filebrowser = new CFileBrowser();

	filebrowser->Multi_Select = false;
	filebrowser->Dirs_Selectable = false;

	// moviebrowser
	moviebrowser = new CMovieBrowser();
	
	// webtv
	webtv = new CWebTV();

	// tsfilefilter
	tsfilefilter.addFilter("ts");
	
#if defined (ENABLE_LIBEPLAYER3) || defined (ENABLE_GSTREAMER)	
	tsfilefilter.addFilter("mpg");
	tsfilefilter.addFilter("mpeg");
	tsfilefilter.addFilter("divx");
	tsfilefilter.addFilter("avi");
	tsfilefilter.addFilter("mkv");
	tsfilefilter.addFilter("asf");
	tsfilefilter.addFilter("aiff");
	tsfilefilter.addFilter("m2p");
	tsfilefilter.addFilter("mpv");
	tsfilefilter.addFilter("m2ts");
	tsfilefilter.addFilter("vob");
	tsfilefilter.addFilter("mp4");
	tsfilefilter.addFilter("mov");	
	tsfilefilter.addFilter("flv");	
	tsfilefilter.addFilter("dat");
	tsfilefilter.addFilter("trp");
	tsfilefilter.addFilter("vdr");
	tsfilefilter.addFilter("mts");
	tsfilefilter.addFilter("wav");
	tsfilefilter.addFilter("flac");
	tsfilefilter.addFilter("mp3");
	tsfilefilter.addFilter("wmv");
	tsfilefilter.addFilter("wma");
	tsfilefilter.addFilter("ogg");
#endif	
	
	// vlcfilefilter
	vlcfilefilter.addFilter ("ts");
	vlcfilefilter.addFilter ("mpg");
	vlcfilefilter.addFilter ("mpeg");
	vlcfilefilter.addFilter ("divx");
	vlcfilefilter.addFilter ("avi");
	vlcfilefilter.addFilter ("mkv");
	vlcfilefilter.addFilter ("asf");
	vlcfilefilter.addFilter ("m2p");
	vlcfilefilter.addFilter ("mpv");
	vlcfilefilter.addFilter ("m2ts");
	vlcfilefilter.addFilter ("vob");
	vlcfilefilter.addFilter ("mp4");
	vlcfilefilter.addFilter ("mov");
	vlcfilefilter.addFilter ("flv");
	vlcfilefilter.addFilter ("m2v");
	vlcfilefilter.addFilter ("wmv");
}

CMoviePlayerGui::~CMoviePlayerGui()
{
	if(playback)
		delete playback;
}

void CMoviePlayerGui::cutNeutrino()
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::%s\n", __FUNCTION__);
	
	if (stopped)
		return;
	
	// pause epg scanning
	g_Sectionsd->setPauseScanning(true);
		
	// lock playback
	g_Zapit->lockPlayBack();
	
	//FIXME: remove this to main control in neutrino.cpp
	/* hide AC3 Icon */
	//TODO: check if ac3 is selected???
	if (g_RemoteControl->has_ac3)
		CVFD::getInstance()->ShowIcon(VFD_ICON_DOLBY, false);

	/* show HD Icon */
	if(live_channel)
	{
		if(live_channel->type == 1)
			CVFD::getInstance()->ShowIcon(VFD_ICON_HD, false);
	}

	// tell neutrino we are in ts mode
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE, NeutrinoMessages::mode_ts);
	
	// save (remeber) last mode
	m_LastMode = (CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);
	
	// start mp start-script
	puts("[movieplayer.cpp] executing " MOVIEPLAYER_START_SCRIPT ".");
	if (system(MOVIEPLAYER_START_SCRIPT) != 0)
		perror("Datei " MOVIEPLAYER_START_SCRIPT " fehlt. Bitte erstellen, wenn gebraucht.\nFile " MOVIEPLAYER_START_SCRIPT " not found. Please create if needed.\n");

	stopped = true;
}

void CMoviePlayerGui::restoreNeutrino()
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::%s\n", __FUNCTION__);
	
	if (!stopped)
		return;

	// unlock playback
	g_Zapit->unlockPlayBack();
		
	// start epg scanning
	g_Sectionsd->setPauseScanning(false);
	
	//FIXME: remove this to main control in neutrino.cpp
	//TODO: check if ac3 is selected???
	/* show AC3 Icon */
	if (g_RemoteControl->has_ac3)
		CVFD::getInstance()->ShowIcon(VFD_ICON_DOLBY, true);

	/* show HD Icon */
	if(live_channel)
	{
		if(live_channel->type == 1)
			CVFD::getInstance()->ShowIcon(VFD_ICON_HD, true);
	}

	// tell neutrino that we are in the last mode
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE, m_LastMode);
	
	//show infobar
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );
	
	// start end script
	puts("[movieplayer.cpp] executing " MOVIEPLAYER_END_SCRIPT ".");
	if (system(MOVIEPLAYER_END_SCRIPT) != 0)
		perror("Datei " MOVIEPLAYER_END_SCRIPT " fehlt. Bitte erstellen, wenn gebraucht.\nFile " MOVIEPLAYER_END_SCRIPT " not found. Please create if needed.\n");

	stopped = false;
}

int CMoviePlayerGui::exec(CMenuTarget * parent, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "[movieplayer] actionKey=%s\n", actionKey.c_str());
	
	// chek vlc path again
	if(Path_vlc_settings != g_settings.streaming_server_startdir)
	{
		Path_vlc  = "vlc://";
		if ((g_settings.streaming_vlc10 < 2) || (strcmp(g_settings.streaming_server_startdir, "/") != 0))
			Path_vlc += g_settings.streaming_server_startdir;
		Path_vlc_settings = g_settings.streaming_server_startdir;
	}

	if (parent) 
		parent->hide();

	bool usedBackground = frameBuffer->getuseBackground();

	if (usedBackground) 
	{
		frameBuffer->saveBackgroundImage();
		frameBuffer->ClearFrameBuffer();

#if !defined USE_OPENGL
		frameBuffer->blit();
#endif
	}
	
	startposition = 0;

	isMovieBrowser = false;
	isWebTV = false;
	isVlc = false;
	isDVD = false;
	isBlueRay = false;
	isURL = false;

	minuteoffset = MINUTEOFFSET;
	secondoffset = minuteoffset / 60;

	if (actionKey == "tsmoviebrowser") 
	{
		isMovieBrowser = true;
		moviebrowser->setMode(MB_SHOW_RECORDS);
		
		timeshift = NO_TIMESHIFT;
		isWebTV = false;
		isVlc = false;
		isDVD = false;
		isBlueRay = false;
		isURL = false;

		PlayFile();
	}
	else if (actionKey == "moviebrowser") 
	{
		isMovieBrowser = true;
		moviebrowser->setMode(MB_SHOW_FILES);
		
		timeshift = NO_TIMESHIFT;
		isWebTV = false;
		isVlc = false;
		isDVD = false;
		isBlueRay = false;
		isURL = false;
		
		PlayFile();
	}
	else if (actionKey == "ytplayback") 
	{
		isMovieBrowser = true;
		moviebrowser->setMode(MB_SHOW_YT);
		
		timeshift = NO_TIMESHIFT;
		isWebTV = false;
		isVlc = false;
		isDVD = false;
		isBlueRay = false;
		isURL = false;
		
		PlayFile();
 	}
	else if (actionKey == "fileplayback") 
	{
		isMovieBrowser = false;
		timeshift = NO_TIMESHIFT;
		isWebTV = false;
		isVlc = false;
		isDVD = false;
		isBlueRay = false;
		isURL = false;
		
		PlayFile();
	}
	else if (actionKey == "timeshift") 
	{
		timeshift = TIMESHIFT;
		PlayFile();
	} 
	else if (actionKey == "ptimeshift") 
	{
		timeshift = P_TIMESHIFT;
		PlayFile();
	} 
	else if ( actionKey == "vlcplayback" ) 
	{
		isVlc = true;
		isWebTV = false;
		streamtype = STREAMTYPE_FILE;
		isMovieBrowser = false;
		timeshift = NO_TIMESHIFT;
		isDVD = false;
		isBlueRay = false;
		isURL = false;
		
		PlayFile();
	}
	else if(actionKey == "webtv")
	{
		isVlc = false;
		isWebTV = true;
		isMovieBrowser = false;
		timeshift = NO_TIMESHIFT;
		isDVD = false;
		isBlueRay = false;
		isURL = false;
		
		PlayFile();
	}
	else if(actionKey == "dvdplayback")
	{
		isMovieBrowser = false;
		timeshift = NO_TIMESHIFT;
		isWebTV = false;
		isVlc = false;
		isBlueRay = false;
		isDVD = true;
		isURL = false;
		
		PlayFile();
	}
	else if(actionKey == "bluerayplayback")
	{
		isMovieBrowser = false;
		timeshift = NO_TIMESHIFT;
		isWebTV = false;
		isVlc = false;
		isDVD = false;
		isBlueRay = true;
		isURL = false;
		
		PlayFile();
	}
	else if(actionKey == "urlplayback")
	{
		isMovieBrowser = false;
		timeshift = NO_TIMESHIFT;
		isWebTV = false;
		isVlc = false;
		isDVD = false;
		isBlueRay = false;
		isURL = true;
		
		PlayFile();
	}
	
	// Restore previous background
	if (usedBackground) 
	{
		frameBuffer->restoreBackgroundImage();
		frameBuffer->useBackground(true);
		frameBuffer->paintBackground();

#if !defined USE_OPENGL
		frameBuffer->blit();
#endif
	}

	restoreNeutrino();
	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

	if (timeshift) 
	{
		timeshift = NO_TIMESHIFT;
		return menu_return::RETURN_EXIT_ALL;
	}
	
	// umount dvd/blueray mount point
	if(isDVD)
		umount((char *)Path_dvd.c_str());
	else if(isBlueRay)
		umount((char *)Path_blueray.c_str());

	return menu_return::RETURN_REPAINT;
}

// vlc
size_t CurlDummyWrite (void *ptr, size_t size, size_t nmemb, void *data)
{
	std::string* pStr = (std::string*) data;
	*pStr += (char*) ptr;
	
	return size * nmemb;
}

CURLcode sendGetRequest (const std::string & url, std::string & response) 
{
	CURL * curl;
	CURLcode httpres;

	curl = curl_easy_init();
	curl_easy_setopt (curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, CurlDummyWrite);
	curl_easy_setopt (curl, CURLOPT_FILE, (void *)&response);
	curl_easy_setopt (curl, CURLOPT_FAILONERROR, true);
	httpres = curl_easy_perform (curl);
	//printf ("[movieplayer.cpp] HTTP Result: %d\n", httpres);
	curl_easy_cleanup (curl);
	
	return httpres;
}

#define TRANSCODE_VIDEO_OFF 0
#define TRANSCODE_VIDEO_MPEG1 1
#define TRANSCODE_VIDEO_MPEG2 2

bool VlcRequestStream(char * mrl, int  transcodeVideo, int transcodeAudio)
{
	CURLcode httpres;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';

	// add sout (URL encoded)
	// Example(mit transcode zu mpeg1): ?sout=#transcode{vcodec=mpgv,vb=2000,acodec=mpga,ab=192,channels=2}:duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
	// Example(ohne transcode zu mpeg1): ?sout=#duplicate{dst=std{access=http,mux=ts,url=:8080/dboxstream}}
	//TODO make this nicer :-)
	std::string souturl;

	//Resolve Resolution from Settings...
	const char * res_horiz;
	const char * res_vert;
	
	switch(g_settings.streaming_resolution)
	{
		case 0:
			res_horiz = "352";
			res_vert = "288";
			break;
		case 1:
			res_horiz = "352";
			res_vert = "576";
			break;
		case 2:
			res_horiz = "480";
			res_vert = "576";
			break;
		case 3:
			res_horiz = "704";
			res_vert = "576";
			break;
		case 4:
			res_horiz = "704";
			res_vert = "288";
			break;
		default:
			res_horiz = "352";
			res_vert = "288";
	} //switch
	
	souturl = "#";
	if(transcodeVideo != TRANSCODE_VIDEO_OFF || transcodeAudio != 0)
	{
		souturl += "transcode{";
		if(transcodeVideo != TRANSCODE_VIDEO_OFF)
		{
			souturl += "vcodec=";
			souturl += (transcodeVideo == TRANSCODE_VIDEO_MPEG1) ? "mpgv" : "mp2v";
			souturl += ",vb=";
			souturl += g_settings.streaming_videorate;
			if (g_settings.streaming_vlc10 != 0)
			{
				souturl += ",scale=1,vfilter=canvas{padd,width=";
				souturl += res_horiz;
				souturl += ",height=";
				souturl += res_vert;
				souturl += ",aspect=4:3}";
			}
			else
			{
				souturl += ",width=";
				souturl += res_horiz;
				souturl += ",height=";
				souturl += res_vert;
			}
			souturl += ",fps=25";
		}
		
		if(transcodeAudio != 0)
		{
			if(transcodeVideo!=TRANSCODE_VIDEO_OFF)
				souturl += ",";
			souturl += "acodec=mpga,ab=";
			souturl += g_settings.streaming_audiorate;
			souturl += ",channels=2";
		}
		souturl += "}:";
	}
	souturl += "std{access=http,mux=ts,dst=";
	//souturl += g_settings.streaming_server_ip;
	souturl += ':';
	souturl += g_settings.streaming_server_port;
	souturl += "/dboxstream}";
	
	char *tmp = curl_escape (souturl.c_str (), 0);

	std::string url = baseurl;
	url += "requests/status.xml?command=in_play&input=";
	url += mrl;	
	
	if (g_settings.streaming_vlc10 > 1)
		url += "&option=";
	else
		url += "%20";
	url += "%3Asout%3D";
	url += tmp;
	curl_free(tmp);
	printf("[movieplayer.cpp] URL(enc) : %s\n", url.c_str());
	std::string response;
	httpres = sendGetRequest(url, response);

	return true; // TODO error checking
}

int VlcGetStreamTime()
{
	// TODO calculate REAL position as position returned by VLC does not take the ringbuffer into consideration
	std::string positionurl = "http://";
	positionurl += g_settings.streaming_server_ip;
	positionurl += ':';
	positionurl += g_settings.streaming_server_port;
	positionurl += "/requests/status.xml";
	//printf("[movieplayer.cpp] positionurl=%s\n",positionurl.c_str());
	std::string response = "";
	CURLcode httpres = sendGetRequest(positionurl, response);
	//printf("[movieplayer.cpp] httpres=%d, response.length()=%d, stream_length = %s\n",httpres,response.length(),response.c_str());
	if(httpres == 0 && response.length() > 0) 
	{
		xmlDocPtr answer_parser = parseXml(response.c_str());
		if (answer_parser != NULL) 
		{
			xmlNodePtr element = xmlDocGetRootElement(answer_parser);
			element = element->xmlChildrenNode;
			while (element) 
			{
				char* tmp = xmlGetName(element);
				if (strcmp(tmp, "time") == 0) 
				{
					return atoi(xmlGetData(element));
				}
				element = element->xmlNextNode;
			}
		}
		return -1;
	}
	else
		return -1;
}

int VlcGetStreamLength()
{
	// TODO calculate REAL position as position returned by VLC does not take the ringbuffer into consideration
	std::string positionurl = "http://";
	positionurl += g_settings.streaming_server_ip;
	positionurl += ':';
	positionurl += g_settings.streaming_server_port;
	positionurl += "/requests/status.xml";
	//printf("[movieplayer.cpp] positionurl=%s\n",positionurl.c_str());
	std::string response = "";
	CURLcode httpres = sendGetRequest(positionurl, response);
	//printf("[movieplayer.cpp] httpres=%d, response.length()=%d, stream_length = %s\n",httpres,response.length(),response.c_str());
	if(httpres == 0 && response.length() > 0) 
	{
		xmlDocPtr answer_parser = parseXml(response.c_str());
		if (answer_parser != NULL) 
		{
			xmlNodePtr element = xmlDocGetRootElement(answer_parser);
			element = element->xmlChildrenNode;
			while (element) 
			{
				char* tmp = xmlGetName(element);
				if (strcmp(tmp, "length") == 0) 
				{
					return atoi(xmlGetData(element));
				}
				element = element->xmlNextNode;
			}
		}
		return -1;
	}
	else
		return -1;
}

bool VlcReceiveStreamStart(void * mrl)
{
	dprintf(DEBUG_NORMAL, "[movieplayer.cpp] ReceiveStream started\n");

	// Get Server and Port from Config
	std::string response;
	std::string baseurl = "http://";
	baseurl += g_settings.streaming_server_ip;
	baseurl += ':';
	baseurl += g_settings.streaming_server_port;
	baseurl += '/';
	baseurl += "requests/status.xml";
	CURLcode httpres = sendGetRequest(baseurl, response);
	
	if(httpres != 0)
	{
		DisplayErrorMessage(g_Locale->getText(LOCALE_MOVIEPLAYER_NOSTREAMINGSERVER));	// UTF-8
		playstate = CMoviePlayerGui::STOPPED;
		return false;
		// Assume safely that all succeeding HTTP requests are successful
	}


	int transcodeVideo, transcodeAudio;
	std::string sMRL = (char*)mrl;
	//Menu Option Force Transcode: Transcode all Files, including mpegs.
	if((!memcmp((char*)mrl, "vcd:", 4) ||
		 !strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "mpg") ||
		 !strcasecmp(sMRL.substr(sMRL.length()-4).c_str(), "mpeg") ||
		 !strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "m2p")))
	{
		if(g_settings.streaming_force_transcode_video)
			transcodeVideo=g_settings.streaming_transcode_video_codec+1;
		else
			transcodeVideo=0;
		transcodeAudio=g_settings.streaming_transcode_audio;
	}
	else
	{
		transcodeVideo = g_settings.streaming_transcode_video_codec + 1;
		if((!memcmp((char*)mrl, "dvd", 3) && !g_settings.streaming_transcode_audio) ||
			(!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "vob") && !g_settings.streaming_transcode_audio) ||
			(!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "ac3") && !g_settings.streaming_transcode_audio) ||
			g_settings.streaming_force_avi_rawaudio)
			transcodeAudio = 0;
		else
			transcodeAudio = 1;
	}
	
	VlcRequestStream((char*)mrl, transcodeVideo, transcodeAudio);

	const char * server = g_settings.streaming_server_ip.c_str();
	int port;
	sscanf(g_settings.streaming_server_port, "%d", &port);

	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons (port);
	servAddr.sin_addr.s_addr = inet_addr (server);

	dprintf(DEBUG_NORMAL, "[movieplayer.cpp] Server: %s\n", server);
	dprintf(DEBUG_NORMAL, "[movieplayer.cpp] Port: %d\n", port);
	int len;

	while(true)
	{
		//printf ("[movieplayer.cpp] Trying to call socket\n");
		
		if(skt < 0)
		{
			skt = socket (AF_INET, SOCK_STREAM, 0);

			dprintf(DEBUG_NORMAL, "[movieplayer.cpp] Trying to connect socket\n");
			
			if(connect(skt, (struct sockaddr *) &servAddr, sizeof (servAddr)) < 0)
			{
				perror ("SOCKET");
				playstate = CMoviePlayerGui::STOPPED;
				return false;
			}
			fcntl (skt, O_NONBLOCK);
		}
		
		dprintf(DEBUG_NORMAL, "[movieplayer.cpp] Socket OK\n");
       
		// Skip HTTP header
		const char * msg = "GET /dboxstream HTTP/1.0\r\n\r\n";
		int msglen = strlen (msg);
		if(send (skt, msg, msglen, 0) == -1)
		{
			perror ("send()");
			playstate = CMoviePlayerGui::STOPPED;
			
			return false;
		}

		dprintf(DEBUG_NORMAL, "[movieplayer.cpp] GET Sent\n");
		
		usleep(1000);

		// Skip HTTP Header
		int found = 0;
		char buf[2];
		char line[200];
		buf[0] = buf[1] = '\0';
		strcpy (line, "");
		
		while(true)
		{
			len = recv(skt, buf, 1, 0);
			strncat (line, buf, 1);
			
			if(strcmp (line, "HTTP/1.0 404") == 0)
			{
				dprintf(DEBUG_NORMAL, "[movieplayer.cpp] VLC still does not send. Exiting...\n");
				
				if(skt > 0)
				{
					close(skt);
					skt = -1;
				}
		
				//break;
				// only once
				return false;
			}
			
			if((((found & (~2)) == 0) && (buf[0] == '\r')) || (((found & (~2)) == 1) && (buf[0] == '\n')))  	// found == 0 || found == 2 *//*   found == 1 || found == 3
			{
				if(found == 3)
					goto vlc_is_sending;
				else
					found++;
			}
			else
			{
				found = 0;
			}
		}
		
		if(playstate == CMoviePlayerGui::STOPPED)
		{
			if(skt > 0 )
			{
				close(skt);
				skt = -1;
			}
			return false;
		}
	}
	
vlc_is_sending:
	dprintf(DEBUG_NORMAL, "[movieplayer.cpp] Now VLC is sending. Read sockets created\n");
	
	return true;
}

void updateLcd(const std::string & sel_filename)
{
	char tmp[20];
	std::string lcd;

	switch (playstate) 
	{
		case CMoviePlayerGui::PAUSE:
			//lcd = "|| ";
			lcd = sel_filename;
			break;
			
		case CMoviePlayerGui::REW:
			sprintf(tmp, "%dx<< ", speed);
			lcd = tmp;
			lcd += sel_filename;
			break;
			
		case CMoviePlayerGui::FF:
			sprintf(tmp, "%dx>> ", speed);
			lcd = tmp;
			lcd += sel_filename;
			break;

		case CMoviePlayerGui::SLOW:
			sprintf(tmp, "%ds>> ", slow);
			lcd = tmp;
			lcd += sel_filename;
			break;

		default:
			//lcd = "> ";
			lcd = sel_filename;
			break;
	}
	
	CVFD::getInstance()->showMenuText(0, lcd.c_str(), -1, true);	
}

void CMoviePlayerGui::PlayFile(void)
{
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	std::string sel_filename;
	CTimeOSD FileTime;
	CMovieInfoViewer MovieInfoViewer;
	
	//
	position = 0;
	duration = 0;
	startposition = 0;
	
	// global flags
	bool update_lcd = false;
	bool open_filebrowser = true;	//always default true (true valeue is needed for file/moviebrowser)
	bool start_play = false;
	bool exit = false;
	bool was_file = false; // needed to reopen browser after playing
	
	// for playing
	playstate = CMoviePlayerGui::STOPPED;
	bool is_file_player = false;
	
	// timeosd
	bool time_forced = false;
	
	// timeshift
	bool timesh = timeshift;
	
	// vlc
	std::string stream_url;
	bool aborted = false;
	char mrl[200];
	CFileList _filelist;
	unsigned int selected = 0;
	std::string title = "";
	bool cdDvd = false;

	// vlc
	if (isVlc == true)
	{
		stream_url = "http://";
		stream_url += g_settings.streaming_server_ip;
		stream_url += ':';
		stream_url += g_settings.streaming_server_port;
		stream_url += "/dboxstream";
		filename = stream_url.c_str();
		
		open_filebrowser = true;
			
		if(streamtype == STREAMTYPE_DVD)
		{
			strcpy (mrl, "dvd://");
			strcat (mrl, g_settings.streaming_server_cddrive);
			strcat (mrl, "@1");
			dprintf(DEBUG_NORMAL, "[movieplayer.cpp] Generated MRL: %s\n", mrl);
			title = "DVD";
			open_filebrowser = false;
			cdDvd = true;
		}
		else if(streamtype == STREAMTYPE_SVCD)
		{
			strcpy (mrl, "vcd://");
			strcat (mrl, g_settings.streaming_server_cddrive);
			dprintf(DEBUG_NORMAL, "[movieplayer.cpp] Generated MRL: %s\n", mrl);
			title = "(S)VCD";
			open_filebrowser = false;
			cdDvd = true;
		}
							
		sel_filename = "VLC Player";
		
		update_lcd = true;
		start_play = true;
		
		g_file_epg = std::string(rindex(filename, '/') + 1);
		g_file_epg1 = std::string(rindex(filename, '/') + 1);
		
		CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);
		
#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)		
		// hide ts icon
		CVFD::getInstance()->ShowIcon(VFD_ICON_TV, false);
#endif		
	}
	
	// webtv
	if(isWebTV)
	{
		sel_filename = "WebTV";
		
		open_filebrowser = true;
		
		update_lcd = true;
		start_play = true;
		
		CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);
		
#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)		
		// hide ts icon
		CVFD::getInstance()->ShowIcon(VFD_ICON_TV, false);
#endif		
	}
	
	// dvd
	if (isDVD || isBlueRay)
	{
		open_filebrowser = true;
							
		sel_filename = "DVD/Blue Ray Player";
		
		update_lcd = true;
		start_play = true;
		
		CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);
		
#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)		
		// hide ts icon
		CVFD::getInstance()->ShowIcon(VFD_ICON_TV, false);
#endif		
	}
	
	if(isURL)
	{
		open_filebrowser = false;
							
		sel_filename = "Movie Player";
		
		filename = g_settings.streaming_server_url.c_str();
		sel_filename = g_settings.streaming_server_url.c_str();
		
		g_file_epg = sel_filename;
		g_file_epg1 = sel_filename;
		
		update_lcd = true;
		start_play = true;
		was_file = false;
		is_file_player = true;
						
		CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);
		
#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)		
		// hide ts icon
		CVFD::getInstance()->ShowIcon(VFD_ICON_TV, false);
#endif		
	}

	// bookmarks menu
	timeb current_time;
	CMovieInfo cMovieInfo;			// funktions to save and load movie info
	MI_MOVIE_INFO * p_movie_info = NULL;	// movie info handle which comes from the MovieBrowser, if not NULL MoviePla yer is able to save new bookmarks

	int width = 280;
	int height = 65;
        int x = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - width) / 2;
        int y = frameBuffer->getScreenY() + frameBuffer->getScreenHeight() - height - 20;

	CBox boxposition(x, y, width, height);	// window position for the hint boxes

	CTextBox endHintBox(g_Locale->getText(LOCALE_MOVIEBROWSER_HINT_MOVIEEND), NULL, CTextBox::CENTER, &boxposition);
	CTextBox comHintBox(g_Locale->getText(LOCALE_MOVIEBROWSER_HINT_JUMPFORWARD), NULL, CTextBox::CENTER, &boxposition);
	CTextBox loopHintBox(g_Locale->getText(LOCALE_MOVIEBROWSER_HINT_JUMPBACKWARD), NULL, CTextBox::CENTER, &boxposition);
	CTextBox newLoopHintBox(g_Locale->getText(LOCALE_MOVIEBROWSER_HINT_NEWBOOK_BACKWARD), NULL, CTextBox::CENTER , &boxposition);
	CTextBox newComHintBox(g_Locale->getText(LOCALE_MOVIEBROWSER_HINT_NEWBOOK_FORWARD), NULL, CTextBox::CENTER, &boxposition);

	bool showEndHintBox = false;	// flag to check whether the box shall be painted
	bool showComHintBox = false;	// flag to check whether the box shall be painted
	bool showLoopHintBox = false;	// flag to check whether the box shall be painted
	int jump_not_until = 0;		// any jump shall be avoided until this time (in seconds from moviestart)
	MI_BOOKMARK new_bookmark;	// used for new movie info bookmarks created from the movieplayer
	new_bookmark.pos = 0;		// clear , since this is used as flag for bookmark activity
	new_bookmark.length = 0;

	// very dirty usage of the menue, but it works and I already spent to much time with it, feel free to make it better ;-)
#define BOOKMARK_START_MENU_MAX_ITEMS 5
	CSelectedMenu cSelectedMenuBookStart[BOOKMARK_START_MENU_MAX_ITEMS];

	CMenuWidget bookStartMenu(LOCALE_MOVIEBROWSER_BOOK_NEW, NEUTRINO_ICON_STREAMING);

	bookStartMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_NEW, true, NULL, &cSelectedMenuBookStart[0]));
	bookStartMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_TYPE_FORWARD, true, NULL, &cSelectedMenuBookStart[1]));
	bookStartMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_TYPE_BACKWARD, true, NULL, &cSelectedMenuBookStart[2]));
	bookStartMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_MOVIESTART, true, NULL, &cSelectedMenuBookStart[3]));
	bookStartMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_MOVIEEND, true, NULL, &cSelectedMenuBookStart[4]));

	// play loop
 go_repeat:
	do {
		// vlc (generate mrl)
		if (playstate == CMoviePlayerGui::STOPPED && isVlc && !cdDvd) 
		{
			if(selected + 1 < _filelist.size() && !aborted) 
			{
				selected++;
				filename = _filelist[selected].Name.c_str();
				sel_filename = _filelist[selected].getFileName();
				//printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
				int namepos = _filelist[selected].Name.rfind("vlc://");
				std::string mrl_str = _filelist[selected].Name.substr(namepos + 6);
				char * tmp = curl_escape (mrl_str.c_str (), 0);
				strncpy (mrl, tmp, sizeof (mrl) - 1);
				curl_free (tmp);
				
				dprintf(DEBUG_NORMAL, "[movieplayer.cpp] Generated FILE MRL: %s\n", mrl);
 
				update_lcd = true;
				start_play = true;
			} 
			else 
			{
				open_filebrowser = true;
				aborted = false;
			}
		}

		// exit
		if (exit) 
		{	  
			exit = false;
			cdDvd = false;
			
			if(skt > 0)
			{
				close(skt);
				skt = -1;
			}
			
			dprintf(DEBUG_NORMAL, "[movieplayer] stop >>\n");			
			playstate = CMoviePlayerGui::STOPPED;
			break;
		}

		// timeshift
		if (timesh) 
		{
			char fname[255];
			int cnt = 10 * 1000000;

			while (!strlen(rec_filename)) 
			{
				usleep(1000);
				cnt -= 1000;
				if (!cnt)
					break;
			}

			if (!strlen(rec_filename))
				return;

			sprintf(fname, "%s.ts", rec_filename);
			filename = fname;
			sel_filename = std::string(rindex(filename, '/') + 1);
			dprintf(DEBUG_NORMAL, "[MoviePlayer] Timeshift: %s\n", sel_filename.c_str());

			update_lcd = true;
			start_play = true;
			open_filebrowser = false;
			timesh = false;
			
			CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

			// start timeosd
			FileTime.SetMode(CTimeOSD::MODE_ASC);
			FileTime.update(position/1000);
		}

		// movie infos (moviebrowser)
		if (isMovieBrowser == true && moviebrowser->getMode() != MB_SHOW_YT) 
		{	  
			// do all moviebrowser stuff here ( like commercial jump etc.)
			if (playstate == CMoviePlayerGui::PLAY) 
			{				
#if defined (PLATFORM_COOLSTREAM)
				playback->GetPosition(position, duration);
#else
 				playback->GetPosition((int64_t &)position, (int64_t &)duration);
#endif				
				
				int play_sec = position / 1000;	// get current seconds from moviestart

				if (play_sec + 10 < jump_not_until || play_sec > jump_not_until + 10)
					jump_not_until = 0;	// check if !jump is stale (e.g. if user jumped forward or backward)

				if (new_bookmark.pos == 0)	// do bookmark activities only, if there is no new bookmark started
				{
					if (p_movie_info != NULL)	// process bookmarks if we have any movie info
					{
						if (p_movie_info->bookmarks.end != 0) 
						{
							// Check for stop position
							if (play_sec >= p_movie_info->bookmarks.end - MOVIE_HINT_BOX_TIMER && play_sec < p_movie_info->bookmarks.end && play_sec > jump_not_until) 
							{
								if (showEndHintBox == false) 
								{
									endHintBox.paint();	// we are 5 sec before the end postition, show warning
									showEndHintBox = true;
									dprintf(DEBUG_INFO, "[mp]  user stop in 5 sec...\r\n");
								}
							} 
							else 
							{
								if (showEndHintBox == true) 
								{
									endHintBox.hide();	// if we showed the warning before, hide the box again
									showEndHintBox = false;
								}
							}

							if (play_sec >= p_movie_info->bookmarks.end && play_sec <= p_movie_info->bookmarks.end + 2 && play_sec > jump_not_until)	// stop playing
							{
								// we ARE close behind the stop position, stop playing 
								dprintf(DEBUG_INFO, "[mp]  user stop\r\n");
								playstate = CMoviePlayerGui::STOPPED;
							}
						}
						
						// Check for bookmark jumps
						int loop = true;
						showLoopHintBox = false;
						showComHintBox = false;

						for (int book_nr = 0; book_nr < MI_MOVIE_BOOK_USER_MAX && loop == true; book_nr++) 
						{
							if (p_movie_info->bookmarks.user[book_nr].pos != 0 && p_movie_info->bookmarks.user[book_nr].length != 0) 
							{
								// valid bookmark found, now check if we are close before or after it
								if (play_sec >= p_movie_info->bookmarks.user[book_nr].pos - MOVIE_HINT_BOX_TIMER && play_sec < p_movie_info->bookmarks.user[book_nr].pos && play_sec > jump_not_until) 
								{
									if (p_movie_info->bookmarks.user[book_nr].length < 0)
										showLoopHintBox = true;	// we are 5 sec before , show warning
									else if (p_movie_info->bookmarks.user[book_nr].length > 0)
										showComHintBox = true;	// we are 5 sec before, show warning
									//else  // TODO should we show a plain bookmark infomation as well?
								}

								if (play_sec >= p_movie_info->bookmarks.user[book_nr].pos && play_sec <= p_movie_info->bookmarks.user[book_nr].pos + 2 && play_sec > jump_not_until)	//
								{
									//for plain bookmark, the following calc shall result in 0 (no jump)
									g_jumpseconds = p_movie_info->bookmarks.user[book_nr].length;

									// we are close behind the bookmark, do bookmark activity (if any)
									if (p_movie_info->bookmarks.user[book_nr].length < 0) 
									{
										// if the jump back time is to less, it does sometimes cause problems (it does probably jump only 5 sec which will cause the next jump, and so on)
										if (g_jumpseconds > -15)
											g_jumpseconds = -15;

										g_jumpseconds = g_jumpseconds + p_movie_info->bookmarks.user[book_nr].pos;

										//playstate = CMoviePlayerGui::JPOS;	// bookmark  is of type loop, jump backward
										playback->SetPosition((int64_t)g_jumpseconds * 1000);
									} 
									else if (p_movie_info->bookmarks.user[book_nr].length > 0) 
									{
										// jump at least 15 seconds
										if (g_jumpseconds < 15)
											g_jumpseconds = 15;
										g_jumpseconds = g_jumpseconds + p_movie_info->bookmarks.user[book_nr].pos;

										//playstate = CMoviePlayerGui::JPOS;	// bookmark  is of type loop, jump backward
										playback->SetPosition((int64_t)g_jumpseconds * 1000);
									}
									
									dprintf(DEBUG_INFO, "[mp]  do jump %d sec\r\n", g_jumpseconds);
									update_lcd = true;
									loop = false;	// do no further bookmark checks
								}
							}
						}
						
						// check if we shall show the commercial warning
						if (showComHintBox == true) 
						{
							comHintBox.paint();
							dprintf(DEBUG_INFO, "[mp]  com jump in 5 sec...\r\n");
						} 
						else
							comHintBox.hide();

						// check if we shall show the loop warning
						if (showLoopHintBox == true) 
						{
							loopHintBox.paint();
							dprintf(DEBUG_INFO, "[mp]  loop jump in 5 sec...\r\n");
						} 
						else
							loopHintBox.hide();
					}
				}
			}
		}	

		// setup all needed flags
		if (open_filebrowser) 
		{
			open_filebrowser = false;
			timeshift = false;
			FileTime.hide();
			MovieInfoViewer.hide();

			// clear audipopids
			for (int i = 0; i < g_numpida; i++) 
			{
				g_apids[i] = 0;
				m_apids[i] = 0;
				g_ac3flags[i] = 0;
				g_language[i].clear();
			}
			g_numpida = 0; g_currentapid = 0;

			// moviebrowser
			if (isMovieBrowser == true) //moviebrowser
			{	
				if (moviebrowser->exec(Path_local.c_str())) 
				{
					// get the current path and file name
					Path_local = moviebrowser->getCurrentDir();
					CFile * file;

					if ((file = moviebrowser->getSelectedFile()) != NULL) 
					{
						CFile::FileType ftype;
						ftype = file->getType();
						
						if (moviebrowser->getMode() == MB_SHOW_YT) 
						{
							filename = file->Url.c_str();
						}
						else
							filename = file->Name.c_str();

						sel_filename = file->getFileName();

						// get the movie info handle (to be used for e.g. bookmark handling)
						p_movie_info = moviebrowser->getCurrentMovieInfo();
						bool recfile = CNeutrinoApp::getInstance()->recordingstatus && !strncmp(rec_filename, filename, strlen(rec_filename));

						if (!recfile && p_movie_info->length) 
						{
							minuteoffset = file->Size / p_movie_info->length;
							minuteoffset = (minuteoffset / MP_TS_SIZE) * MP_TS_SIZE;
							if (minuteoffset < 5000000 || minuteoffset > 190000000)
								minuteoffset = MINUTEOFFSET;
							secondoffset = minuteoffset / 60;
						}

						if(!p_movie_info->audioPids.empty()) 
						{
							g_currentapid = p_movie_info->audioPids[0].epgAudioPid;	//FIXME
							g_currentac3 = p_movie_info->audioPids[0].atype;

							if(g_currentac3)
								ac3state = CInfoViewer::AC3_AVAILABLE;
						}

						for (int i = 0; i < (int)p_movie_info->audioPids.size(); i++) 
						{
							m_apids[i] = p_movie_info->audioPids[i].epgAudioPid;

							g_ac3flags[i] = p_movie_info->audioPids[i].atype;
							g_numpida++;

							if (p_movie_info->audioPids[i].selected) 
							{
								g_currentapid = p_movie_info->audioPids[i].epgAudioPid;	//FIXME
								g_currentac3 = p_movie_info->audioPids[i].atype;
								//break;

								if(g_currentac3)
									ac3state = CInfoViewer::AC3_ACTIVE;
							}
						}

						g_vpid = p_movie_info->epgVideoPid;
						g_vtype = p_movie_info->VideoType;
						
						// needed for movies (not ts)
						if(!p_movie_info->epgTitle.empty())
							g_file_epg = p_movie_info->epgTitle;
						else
							g_file_epg = sel_filename;
						
						if(!p_movie_info->epgInfo2.empty())
							g_file_epg1 = p_movie_info->epgInfo2;
						else
							g_file_epg1 = sel_filename;
						
						dprintf(DEBUG_INFO, "CMoviePlayerGui::PlayFile: file %s apid 0x%X atype %d vpid 0x%X vtype %d\n", filename, g_currentapid, g_currentac3, g_vpid, g_vtype);
						dprintf(DEBUG_NORMAL, "CMoviePlayerGui::PlayFile: Bytes per minute: %lld\n", minuteoffset);
						
						// get the start position for the movie					
						startposition = 1000 * moviebrowser->getCurrentStartPos();						

						update_lcd = true;
						start_play = true;
						was_file = true;
					}
				} 
				else if (playstate == CMoviePlayerGui::STOPPED) 
				{
					was_file = false;
					break;
				}
				
				CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);
				
#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
				// hide ts icon
				CVFD::getInstance()->ShowIcon(VFD_ICON_TV, false);
#endif				
			} 
			else if(isVlc && !cdDvd) // vlc (file not dvd)
			{
				filename = NULL;
				filebrowser->Filter = &vlcfilefilter;
				
				if(filebrowser->exec(Path_vlc.c_str()))
				{
					Path_vlc = filebrowser->getCurrentDir();

					CFile * file = filebrowser->getSelectedFile();
					_filelist.clear();
					_filelist.push_back(*file);

					if(!_filelist.empty())
					{
						filename = _filelist[0].Name.c_str();
						sel_filename = _filelist[0].getFileName();
						
						int namepos = _filelist[0].Name.rfind("vlc://");
						std::string mrl_str = "";
						
						if (g_settings.streaming_vlc10 > 1)
						{
							mrl_str += "file://";
							if (filename[namepos + 6] != '/')
								mrl_str += "/";
						}
						
						mrl_str += _filelist[0].Name.substr(namepos + 6);
						char * tmp = curl_escape(mrl_str.c_str (), 0);
						strncpy (mrl, tmp, sizeof (mrl) - 1);
						curl_free (tmp);
						dprintf(DEBUG_NORMAL, "[movieplayer.cpp] Generated FILE MRL: %s\n", mrl);

						update_lcd = true;
						start_play = true;
						was_file = true;
						selected = 0;
					}
				}
				else if(playstate == CMoviePlayerGui::STOPPED)
				{
					was_file = false;
					break;
				}

			}
			else if (isWebTV)	//webtv
			{
				if( webtv->exec())
				{
					CFile * file;

					if ((file = webtv->getSelectedFile()) != NULL) 
					{
						filename = file->Url.c_str();
						sel_filename = file->Name.c_str();
						
						g_file_epg = sel_filename;
						g_file_epg1 = file->Description.c_str();
						
						update_lcd = true;
						start_play = true;
						was_file = true;
						is_file_player = true;
					}
				}
				else if (playstate == CMoviePlayerGui::STOPPED) 
				{
					was_file = false;
					break;
				}
			}
			else if(isDVD) // dvd
			{
				filename = NULL;
				filebrowser->Filter = &tsfilefilter;
				
				// create mount path
				safe_mkdir((char *)Path_dvd.c_str());
						
				// mount selected iso image
				char cmd[128];
				sprintf(cmd, "mount -o loop /tmp/mydvd.iso %s", (char *)Path_dvd.c_str());
				system(cmd);
				
				if(filebrowser->exec(Path_dvd.c_str()))
				{
					Path_dvd = filebrowser->getCurrentDir();

					CFile * file = filebrowser->getSelectedFile();

					if ((file = filebrowser->getSelectedFile()) != NULL) 
					{
						CFile::FileType ftype;
						ftype = file->getType();

						filename = file->Name.c_str();
						sel_filename = filebrowser->getSelectedFile()->getFileName();
						
						g_file_epg = sel_filename;
						g_file_epg1 = sel_filename;
						
						update_lcd = true;
						start_play = true;
						was_file = true;
						is_file_player = true;
					}
				}
				else if(playstate == CMoviePlayerGui::STOPPED)
				{
					was_file = false;
					break;
				}

			}
			else if(isBlueRay) // blueray
			{
				filename = NULL;
				filebrowser->Filter = &tsfilefilter;
				
				// create mount path
				safe_mkdir((char *)Path_blueray.c_str());
						
				// mount selected myblueray
				char cmd[128];

				sprintf(cmd, "mount -o loop -t udf /tmp/myblueray.iso %s", (char *)Path_blueray.c_str());
				system(cmd);
				
				if(filebrowser->exec(Path_blueray.c_str()))
				{
					Path_blueray = filebrowser->getCurrentDir();

					CFile * file = filebrowser->getSelectedFile();

					if ((file = filebrowser->getSelectedFile()) != NULL) 
					{
						CFile::FileType ftype;
						ftype = file->getType();

						filename = file->Name.c_str();
						sel_filename = filebrowser->getSelectedFile()->getFileName();
						
						g_file_epg = sel_filename;
						g_file_epg1 = sel_filename;
						
						update_lcd = true;
						start_play = true;
						was_file = true;
						is_file_player = true;
					}
				}
				else if(playstate == CMoviePlayerGui::STOPPED)
				{
					was_file = false;
					break;
				}

			}
			else  // filebrowser
			{
				filebrowser->Filter = &tsfilefilter;

				if (filebrowser->exec(Path_local.c_str()) == true) 
				{
					Path_local = filebrowser->getCurrentDir();
					CFile * file;

					if ((file = filebrowser->getSelectedFile()) != NULL) 
					{
						CFile::FileType ftype;
						ftype = file->getType();

						filename = file->Name.c_str();
						sel_filename = filebrowser->getSelectedFile()->getFileName();
						
						g_file_epg = sel_filename;
						g_file_epg1 = sel_filename;
						
						update_lcd = true;
						start_play = true;
						was_file = true;
						is_file_player = true;
					}
				}
				else if (playstate == CMoviePlayerGui::STOPPED) 
				{
					was_file = false;
					break;
				}

				CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);
				
#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
				// hide ts icon
				CVFD::getInstance()->ShowIcon(VFD_ICON_TV, false);
#endif				
			}
		}

		// LCD 
		if (update_lcd) 
		{
			update_lcd = false;

			if (isMovieBrowser && strlen(p_movie_info->epgTitle.c_str()) && strncmp(p_movie_info->epgTitle.c_str(), "not", 3))
				updateLcd(p_movie_info->epgTitle);
			else
				updateLcd(sel_filename);
		}

		// Audio Pids
		if (showaudioselectdialog) 
		{
			CMenuWidget APIDSelector(LOCALE_APIDSELECTOR_HEAD, NEUTRINO_ICON_AUDIO);

			// g_apids will be rewritten for mb
			playback->FindAllPids(g_apids, g_ac3flags, &g_numpida, g_language);
			
			if (g_numpida > 0) 
			{
				// intros
				//APIDSelector.addItem(GenericMenuSeparator);
				
				CAPIDSelectExec * APIDChanger = new CAPIDSelectExec;
				bool enabled;
				bool defpid;

				for (unsigned int count = 0; count < g_numpida; count++) 
				{
					bool name_ok = false;
					char apidnumber[10];
					sprintf(apidnumber, "%d %X", count + 1, g_apids[count]);
					enabled = true;
					defpid = g_currentapid ? (g_currentapid == g_apids[count]) : (count == 0);
					std::string apidtitle = "Stream ";

					// language name from mb
					if(!is_file_player)
					{
						// we use again the apids from mb
						name_ok = get_movie_info_apid_name(m_apids[count], p_movie_info, &apidtitle);
					}
					else if (!g_language[count].empty())
					{
						apidtitle = g_language[count];
						name_ok = true;
					}

					if (!name_ok)
					{
						apidtitle = "Stream ";
						name_ok = true;
					}

					switch(g_ac3flags[count])
					{
						case 1: /*AC3,EAC3*/
							if (apidtitle.find("AC3") <= 0 || is_file_player)
							{
								apidtitle.append(" (AC3)");
								
								// ac3 state
								ac3state = CInfoViewer::AC3_AVAILABLE;
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
				//

				apidchanged = 0;
				APIDSelector.exec(NULL, "");

				if (apidchanged) 
				{
					if (g_currentapid == 0) 
					{
						g_currentapid = g_apids[0];
						g_currentac3 = g_ac3flags[0];

						if(g_currentac3)
							ac3state = CInfoViewer::AC3_ACTIVE;
					}

#if defined (PLATFORM_COOLSTREAM)
					playback->SetAPid(g_currentapid, g_currentac3);
#else					
					playback->SetAPid(g_currentapid);
#endif					
					apidchanged = 0;
				}
				
				delete APIDChanger;
				showaudioselectdialog = false;
				CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);
#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
				// hide ts icon
				CVFD::getInstance()->ShowIcon(VFD_ICON_TV, false);
#endif				
				update_lcd = true;
			} 
			else 
			{
				DisplayErrorMessage(g_Locale->getText(LOCALE_AUDIOSELECTMENUE_NO_TRACKS)); // UTF-8
				showaudioselectdialog = false;
			}
		}

		/* Time */
		if (FileTime.IsVisible()) 
		{
			if (FileTime.GetMode() == CTimeOSD::MODE_ASC) 
			{
				FileTime.update(position / 1000);
				FileTime.show(position / 1000);
			} 
			else 
			{
				FileTime.update((duration - position) / 1000);
				FileTime.show(position / 1000);
			}
		}
		
		// movieinfobar
		if (MovieInfoViewer.IsVisible()) 
			MovieInfoViewer.updatePos(file_prozent);

		// start playing
		if (start_play) 
		{
			dprintf(DEBUG_NORMAL, "%s::%s Startplay at %d seconds\n", FILENAME, __FUNCTION__, startposition/1000);

			start_play = false;
			
			if (isVlc)
			{
				playstate = CMoviePlayerGui::SOFTRESET;
				
				if( VlcReceiveStreamStart(mrl) )
				{
					stream_url = "http://";
					stream_url += g_settings.streaming_server_ip;
					stream_url += ':';
					stream_url += g_settings.streaming_server_port;
					stream_url += "/dboxstream";
					filename = stream_url.c_str();
				}
			}			

			if (playstate >= CMoviePlayerGui::PLAY) 
			{
				// vlc socket
				if(skt > 0)
				{
					close(skt);
					skt = -1;
				}
				
				playstate = CMoviePlayerGui::STOPPED;
				
				playback->Close();
			}

			cutNeutrino();

			// init player
#if defined (PLATFORM_COOLSTREAM)
			playback->Open(is_file_player ? PLAYMODE_FILE : PLAYMODE_TS);
#else			
			playback->Open();
#endif			
			
			duration = 0;
			if(p_movie_info != NULL)
				duration = p_movie_info->length * 60 * 1000;
			  
			// PlayBack Start			  
			if(!playback->Start((char *)filename, g_vpid, g_vtype, g_currentapid, g_currentac3, duration))
			{
				dprintf(DEBUG_NORMAL, "%s::%s Starting Playback failed!\n", FILENAME, __FUNCTION__);
				playback->Close();
				restoreNeutrino();
			} 
			else 
			{
				// set PlayState
				playstate = CMoviePlayerGui::PLAY;

				CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, true);
				
				// PlayBack SetStartPosition for timeshift
				if(timeshift)
				{
					startposition = 0;
					
					//wait
					usleep(TIMESHIFT_SECONDS*1000);

					dprintf(DEBUG_NORMAL, "[movieplayer] Timeshift %d, position %d, seek to %d seconds\n", timeshift, position, startposition/1000);
				}
				
				// set position 
				if( !is_file_player && startposition >= 0)//FIXME no jump for file at start yet
					playback->SetPosition((int64_t)startposition);
				
#if defined (PLATFORM_COOLSTREAM)
				playback->GetPosition(position, duration);
#else
 				playback->GetPosition((int64_t &)position, (int64_t &)duration);
#endif	
				
				// show movieinfoviewer at start
				if(timeshift)
				{
					g_InfoViewer->showTitle(CNeutrinoApp::getInstance()->channelList->getActiveChannelNumber(), CNeutrinoApp::getInstance()->channelList->getActiveChannelName(), CNeutrinoApp::getInstance()->channelList->getActiveSatellitePosition(), CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID());	// UTF-8
				}
				else
				{
					if(!MovieInfoViewer.IsVisible())
						MovieInfoViewer.show(position/1000, false);
				}
			}
		}
		
		// control loop
		g_RCInput->getMsg(&msg, &data, 10);	// 1 secs

		//get position/duration/speed during playing
		if ( playstate >= CMoviePlayerGui::PLAY )
		{	  	
#if defined (PLATFORM_COOLSTREAM)
			if( playback->GetPosition(position, duration) )
#else
 			if( playback->GetPosition((int64_t &)position, (int64_t &)duration) )
#endif			
			{					
				if(duration > 100)
					file_prozent = (unsigned char) (position / (duration / 100));

				playback->GetSpeed(speed);
							
				dprintf(DEBUG_DEBUG, "CMoviePlayerGui::PlayFile: speed %d position %d duration %d (%d%%)\n", speed, position, duration, file_prozent);					
			}
			else
			{
				if(isWebTV)
				{
					// hint
					ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText (LOCALE_INFOVIEWER_NOTAVAILABLE), 430);	// UTF-8
				}
				else
					g_RCInput->postMsg((neutrino_msg_t) g_settings.mpkey_stop, 0);
			}
		}
		
		if (msg == (neutrino_msg_t) g_settings.mpkey_stop) 
		{
			//exit play
			playstate = CMoviePlayerGui::STOPPED;
			
			aborted = true;
			
			if (cdDvd) 
			{
				cdDvd = false;
				exit = true;
			}

			if (isMovieBrowser == true && moviebrowser->getMode() != MB_SHOW_YT) 
			{
				// if we have a movie information, try to save the stop position
				ftime(&current_time);
				p_movie_info->dateOfLastPlay = current_time.time;
				current_time.time = time(NULL);
				p_movie_info->bookmarks.lastPlayStop = position / 1000;
				
				if(p_movie_info->epgChannel.empty())
					p_movie_info->epgChannel = sel_filename;
				if(p_movie_info->epgTitle.empty())
					p_movie_info->epgTitle = sel_filename;

				cMovieInfo.saveMovieInfo(*p_movie_info);
				//p_movie_info->fileInfoStale(); //TODO: we might to tell the Moviebrowser that the movie info has changed, but this could cause long reload times  when reentering the Moviebrowser
			}
			
			if(timeshift == TIMESHIFT) //timeshift
			{
				CVCRControl::getInstance()->Stop();
				
				g_Timerd->stopTimerEvent(CNeutrinoApp::getInstance()->recording_id); // this stop immediatly timeshift
				CNeutrinoApp::getInstance()->recording_id = 0;
				CNeutrinoApp::getInstance()->recordingstatus = 0;
				CNeutrinoApp::getInstance()->timeshiftstatus = 0;
				
				CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false );
			}
			else if(timeshift == P_TIMESHIFT) //ptimeshift
				g_RCInput->postMsg((neutrino_msg_t) CRCInput::RC_stop, 0); // this will send msg yes/nos to stop timeshift

			if (!was_file)
				exit = true;
		} 
		else if (msg == (neutrino_msg_t) g_settings.mpkey_play) 
		{
			if (playstate >= CMoviePlayerGui::PLAY) 
			{
				playstate = CMoviePlayerGui::PLAY;
				update_lcd = true;
				CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, true);
				CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, false);
				
				speed = 1;
				playback->SetSpeed(speed);
			} 
			else if (!timeshift) //???
			{
				open_filebrowser = true;
			}

			if (time_forced) 
			{
				time_forced = false;
				
				FileTime.hide();
			}
			
			if (FileTime.IsVisible()) 
				FileTime.hide();
			
			if(MovieInfoViewer.IsVisible() )
				MovieInfoViewer.hide();

			// movie title
			if(!timeshift)
			{
				if (FileTime.IsVisible()) 
				{
					if (FileTime.GetMode() == CTimeOSD::MODE_ASC) 
					{
						FileTime.SetMode(CTimeOSD::MODE_DESC);
						FileTime.update((duration - position) / 1000);
						//FileTime.show(position / 1000);
						//FileTime.updatePos(file_prozent);
					} 
					else 
					{
						FileTime.hide();
					}
				}
				else 
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					//FileTime.updatePos(file_prozent);
				}
				
				if(!MovieInfoViewer.IsVisible())
					MovieInfoViewer.show(position/1000);
			}
		} 
		else if ( msg == (neutrino_msg_t) g_settings.mpkey_pause) 
		{
			update_lcd = true;
			
			if (playstate == CMoviePlayerGui::PAUSE) 
			{
				playstate = CMoviePlayerGui::PLAY;
				CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, false);
				// show play icon
				CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, true);
				speed = 1;
				playback->SetSpeed(speed);
			} 
			else 
			{
				playstate = CMoviePlayerGui::PAUSE;
				CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, true);
				CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, false);
				speed = 0;
				playback->SetSpeed(speed);
			}
			
			if (FileTime.IsVisible()) 
				FileTime.hide();

			//show MovieInfoBar
			if(!timeshift)
			{
				if (FileTime.IsVisible()) 
				{
					if (FileTime.GetMode() == CTimeOSD::MODE_ASC) 
					{
						FileTime.SetMode(CTimeOSD::MODE_DESC);
						FileTime.update((duration - position) / 1000);
						//FileTime.show(position / 1000);
						//FileTime.updatePos(file_prozent);
					} 
					else 
					{
						FileTime.hide();
					}
				}
				else 
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					//FileTime.updatePos(file_prozent);
				}
				
				if(!MovieInfoViewer.IsVisible())
					MovieInfoViewer.show(position/1000);
			}
		} 
		else if (msg == (neutrino_msg_t) g_settings.mpkey_bookmark) 
		{
			if (FileTime.IsVisible()) 
				FileTime.hide();
						
			if(isMovieBrowser == true && moviebrowser->getMode() != MB_SHOW_YT)
			{
				int pos_sec = position / 1000;

				if (newComHintBox.isPainted() == true) 
				{
					// yes, let's get the end pos of the jump forward
					new_bookmark.length = pos_sec - new_bookmark.pos;
					TRACE("[mp] commercial length: %d\r\n", new_bookmark.length);
					if (cMovieInfo.addNewBookmark(p_movie_info, new_bookmark) == true) 
					{
						cMovieInfo.saveMovieInfo(*p_movie_info);	/* save immediately in xml file */
					}
					new_bookmark.pos = 0;	// clear again, since this is used as flag for bookmark activity
					newComHintBox.hide();
				} 
				else if (newLoopHintBox.isPainted() == true) 
				{
					// yes, let's get the end pos of the jump backward
					new_bookmark.length = new_bookmark.pos - pos_sec;
					new_bookmark.pos = pos_sec;
					TRACE("[mp] loop length: %d\r\n", new_bookmark.length);
					if (cMovieInfo.addNewBookmark(p_movie_info, new_bookmark) == true) 
					{
						cMovieInfo.saveMovieInfo(*p_movie_info);	/* save immediately in xml file */
						jump_not_until = pos_sec + 5;	// avoid jumping for this time
					}
					new_bookmark.pos = 0;	// clear again, since this is used as flag for bookmark activity
					newLoopHintBox.hide();
				} 
				else 
				{
					// no, nothing else to do, we open a new bookmark menu
					new_bookmark.name = "";	// use default name
					new_bookmark.pos = 0;
					new_bookmark.length = 0;

					// next seems return menu_return::RETURN_EXIT, if something selected
					bookStartMenu.exec(NULL, "none");
					
					if (cSelectedMenuBookStart[0].selected == true) 
					{
						/* Moviebrowser plain bookmark */
						new_bookmark.pos = pos_sec;
						new_bookmark.length = 0;
						if (cMovieInfo.addNewBookmark(p_movie_info, new_bookmark) == true)
							cMovieInfo.saveMovieInfo(*p_movie_info);	/* save immediately in xml file */
						new_bookmark.pos = 0;	// clear again, since this is used as flag for bookmark activity
						cSelectedMenuBookStart[0].selected = false;	// clear for next bookmark menu
					} 
					else if (cSelectedMenuBookStart[1].selected == true)
					{
						/* Moviebrowser jump forward bookmark */
						new_bookmark.pos = pos_sec;
						TRACE("[mp] new bookmark 1. pos: %d\r\n", new_bookmark.pos);
						newComHintBox.paint();

						cSelectedMenuBookStart[1].selected = false;	// clear for next bookmark menu
					} 
					else if (cSelectedMenuBookStart[2].selected == true) 
					{
						/* Moviebrowser jump backward bookmark */
						new_bookmark.pos = pos_sec;
						TRACE("[mp] new bookmark 1. pos: %d\r\n", new_bookmark.pos);
						newLoopHintBox.paint();
						cSelectedMenuBookStart[2].selected = false;	// clear for next bookmark menu
					} 
					else if (cSelectedMenuBookStart[3].selected == true) 
					{
						/* Moviebrowser movie start bookmark */
						p_movie_info->bookmarks.start = pos_sec;
						TRACE("[mp] New movie start pos: %d\r\n", p_movie_info->bookmarks.start);
						cMovieInfo.saveMovieInfo(*p_movie_info);	/* save immediately in xml file */
						cSelectedMenuBookStart[3].selected = false;	// clear for next bookmark menu
					} 
					else if (cSelectedMenuBookStart[4].selected == true) 
					{
						/* Moviebrowser movie end bookmark */
						p_movie_info->bookmarks.end = pos_sec;
						TRACE("[mp]  New movie end pos: %d\r\n", p_movie_info->bookmarks.start);
						cMovieInfo.saveMovieInfo(*p_movie_info);	/* save immediately in xml file */
						cSelectedMenuBookStart[4].selected = false;	// clear for next bookmark menu
					}
				}
			}		
		} 
		else if ( (msg == (neutrino_msg_t) g_settings.mpkey_audio) || ( msg == CRCInput::RC_audio) ) 
		{
			if (FileTime.IsVisible()) 
				FileTime.hide();
			
			showaudioselectdialog = true;
		} 
		else if(msg == CRCInput::RC_yellow)
		{
			if (FileTime.IsVisible()) 
				FileTime.hide();
			
			//show help
			if(!isWebTV)
				showHelpTS();
		}
		else if (msg == CRCInput::RC_info) 
		{
			if(!timeshift)
			{
				if (FileTime.IsVisible()) 
					FileTime.hide();
				
				if(!MovieInfoViewer.IsVisible())
					MovieInfoViewer.show(position/1000);
				else 
				{
					MovieInfoViewer.hide();
					
					if(isVlc)
						showFileInfoVLC();
					else if(isWebTV)
						showFileInfoWebTV();
					else if (p_movie_info != NULL)
						cMovieInfo.showMovieInfo(*p_movie_info);
				}
			}
			else if(timeshift)
			{
				if (FileTime.IsVisible()) 
					FileTime.hide();
				
				g_InfoViewer->showTitle(CNeutrinoApp::getInstance()->channelList->getActiveChannelNumber(), CNeutrinoApp::getInstance()->channelList->getActiveChannelName(), CNeutrinoApp::getInstance()->channelList->getActiveSatellitePosition(), CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID());	// UTF-8
			}
		}
		else if ( msg == (neutrino_msg_t) g_settings.mpkey_time )
		{
			if(!timeshift)
			{
				if (FileTime.IsVisible()) 
				{
					if (FileTime.GetMode() == CTimeOSD::MODE_ASC) 
					{
						FileTime.SetMode(CTimeOSD::MODE_DESC);
						FileTime.update((duration - position) / 1000);
						//FileTime.show(position / 1000);
						//FileTime.updatePos(file_prozent);
					} 
					else 
					{
						FileTime.hide();
					}
				}
				else 
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					//FileTime.updatePos(file_prozent);
				}
			}
			else
			{
				if (FileTime.IsVisible()) 
					FileTime.hide();
				else
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
				}
			}
		} 
		else if (msg == (neutrino_msg_t) g_settings.mpkey_rewind) 
		{
			// backward
			speed = (speed >= 0) ? -1 : speed - 1;
						
			if(speed < -15)
				speed = -15;			
			
			// hide icons
			CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, false);
			CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, false);

			playback->SetSpeed(speed);
			playstate = CMoviePlayerGui::REW;
			update_lcd = true;

			if (FileTime.IsVisible()) 
				FileTime.hide();
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
			
			if(!MovieInfoViewer.IsVisible())
				MovieInfoViewer.show(position/1000);
		}
		else if (msg == (neutrino_msg_t) g_settings.mpkey_forward) 
		{	// fast-forward
			speed = (speed <= 0) ? 2 : speed + 1;
						
			if(speed > 15)
				speed = 15;			
			
			// icons
			CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, false);
			CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, false);

			playback->SetSpeed(speed);

			update_lcd = true;
			playstate = CMoviePlayerGui::FF;

			if (FileTime.IsVisible()) 
				FileTime.hide();

			// movie info viewer
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
			
			if(!MovieInfoViewer.IsVisible())
				MovieInfoViewer.show(position/1000);
		} 
		else if (msg == CRCInput::RC_1) 
		{	// Jump Backwards 1 minute
			//update_lcd = true;
			playback->SetPosition(-60 * 1000);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_3) 
		{	// Jump Forward 1 minute
			//update_lcd = true;
			playback->SetPosition(60 * 1000);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_4) 
		{	// Jump Backwards 5 minutes
			playback->SetPosition(-5 * 60 * 1000);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_6) 
		{	// Jump Forward 5 minutes
			playback->SetPosition(5 * 60 * 1000);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_7) 
		{	// Jump Backwards 10 minutes
			playback->SetPosition(-10 * 60 * 1000);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_9) 
		{	// Jump Forward 10 minutes
			playback->SetPosition(10 * 60 * 1000);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
		} 
		else if ( msg == CRCInput::RC_2 || msg == CRCInput::RC_repeat )
		{	// goto start
			playback->SetPosition((int64_t)startposition);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_5) 
		{	
			// goto middle
			playback->SetPosition((int64_t)duration/2);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_8) 
		{	
			// goto end
			playback->SetPosition((int64_t)duration - 60 * 1000);
			
			//time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_page_up) 
		{
			playback->SetPosition(10 * 1000);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}

		} 
		else if (msg == CRCInput::RC_page_down) 
		{
			playback->SetPosition(-10 * 1000);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_0 && !isWebTV) 
		{	// cancel bookmark jump
			if (isMovieBrowser == true && moviebrowser->getMode() != MB_SHOW_YT) 
			{
				if (new_bookmark.pos != 0) 
				{
					new_bookmark.pos = 0;	// stop current bookmark activity, TODO:  might bemoved to another key
					newLoopHintBox.hide();	// hide hint box if any
					newComHintBox.hide();
				}
				jump_not_until = (position / 1000) + 10;	// avoid bookmark jumping for the next 10 seconds, , TODO:  might be moved to another key
			} 
			else if (playstate != CMoviePlayerGui::PAUSE)
				playstate = CMoviePlayerGui::SOFTRESET;
		} 
#if !defined (PLATFORM_COOLSTREAM)		
		else if (msg == CRCInput::RC_slow) 
		{
			if (slow > 0)
				slow = 0;
			
			slow += 2;
		
			// set slow
			playback->SetSlow(slow);
			//update_lcd = true;
			playstate = CMoviePlayerGui::SLOW;
			update_lcd = true;
		}
#endif		
		else if(msg == CRCInput::RC_red)
		{
			if (FileTime.IsVisible()) 
				FileTime.hide();
			
			if(isVlc)
				showFileInfoVLC();
			else if(isWebTV)
				showFileInfoWebTV();
			else if (p_movie_info != NULL)
				cMovieInfo.showMovieInfo(*p_movie_info);
			else if(timeshift)
				g_EpgData->show(live_channel_id);
		}
		else if(msg == CRCInput::RC_home)
		{
			if (FileTime.IsVisible()) 
				FileTime.hide();
			
			if (MovieInfoViewer.IsVisible()) 
				MovieInfoViewer.hide();
		}
		else if(msg == CRCInput::RC_ok)
		{
			if(isWebTV)
				open_filebrowser = true;
		}
		else if( ( msg == (neutrino_msg_t) g_settings.key_quickzap_up ) || ( msg == (neutrino_msg_t) g_settings.key_quickzap_down ) )
		{
			if(isWebTV)
			{
				//exit = true;
				open_filebrowser = true;
				webtv->quickZap(msg);
			}
		}
		else if (msg == (neutrino_msg_t)g_settings.key_screenshot && isMovieBrowser == true && moviebrowser->getMode() != MB_SHOW_YT)
		{
         		if(ShowMsgUTF (LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SCREENSHOT_ANNOUNCE), CMessageBox::mbrNo, CMessageBox:: mbYes | CMessageBox::mbNo) == CMessageBox::mbrYes) 
			{
				CVCRControl::getInstance()->Screenshot(0, (char *)filename, position / 1000);
			}
		}
		else if (msg == CRCInput::RC_timeout) 
		{
			// nothing
			if (MovieInfoViewer.IsVisible()) 
			{
				sleep(1);
				MovieInfoViewer.hide();
			}
		}
		else if ((msg == NeutrinoMessages::ANNOUNCE_RECORD) || msg == NeutrinoMessages::RECORD_START || msg == NeutrinoMessages::ZAPTO || msg == NeutrinoMessages::STANDBY_ON || msg == NeutrinoMessages::SHUTDOWN || msg == NeutrinoMessages::SLEEPTIMER) 
		{	
			// Exit for Record/Zapto Timers
			exit = true;
			g_RCInput->postMsg(msg, data);
		}
		else 
		{
			if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
				exit = true;
			else if ( msg <= CRCInput::RC_MaxRC ) 
			{
				CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);
#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
				// hide ts icon
				CVFD::getInstance()->ShowIcon(VFD_ICON_TV, false);
#endif				
				update_lcd = true;
			}
		}

		if (exit) 
		{
			dprintf(DEBUG_NORMAL, "[movieplayer] stop >\n");	

			if (isMovieBrowser == true && moviebrowser->getMode() != MB_SHOW_YT) 
			{
				// if we have a movie information, try to save the stop position
				ftime(&current_time);
				p_movie_info->dateOfLastPlay = current_time.time;
				current_time.time = time(NULL);
				p_movie_info->bookmarks.lastPlayStop = position / 1000;
				
				if(p_movie_info->epgChannel.empty())
					p_movie_info->epgChannel = sel_filename;
				if(p_movie_info->epgTitle.empty())
					p_movie_info->epgTitle = sel_filename;

				cMovieInfo.saveMovieInfo(*p_movie_info);
				//p_movie_info->fileInfoStale(); //TODO: we might to tell the Moviebrowser that the movie info has changed, but this could cause long reload times  when reentering the Moviebrowser
			}
		}
	} while (playstate >= CMoviePlayerGui::PLAY);
	
	dprintf(DEBUG_NORMAL, "[movieplayer] stop >>\n");	

	if(FileTime.IsVisible())
		FileTime.hide();
	
	if(MovieInfoViewer.IsVisible())
		MovieInfoViewer.hide();
	
	playback->Close();

	CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, false);
	CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, false);

	if (was_file) 
	{
		restoreNeutrino();
		open_filebrowser = true;
		start_play = true;
		goto go_repeat;
	}
}

void CMoviePlayerGui::showHelpTS()
{
	Helpbox helpbox;
	helpbox.addLine(NEUTRINO_ICON_BUTTON_RED, /*g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP1)*/ (char *)"Movie info");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_GREEN, g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP2));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_YELLOW, g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP3));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_BLUE, g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP4));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_DBOX, g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP5));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_HELP, g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP5));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_1, (char *)"jump backward 1 min"/*g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP6)*/ );
	helpbox.addLine(NEUTRINO_ICON_BUTTON_2, (char *)"goto start"/*g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP6)*/ );
	helpbox.addLine(NEUTRINO_ICON_BUTTON_3, (char *)"jump forward 1 min"/*g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP7)*/ );
	helpbox.addLine(NEUTRINO_ICON_BUTTON_4, (char *)"jump backward 5 min"/*g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP8)*/ );
	helpbox.addLine(NEUTRINO_ICON_BUTTON_5, (char *)"goto middle"/*g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP6)*/ );
	helpbox.addLine(NEUTRINO_ICON_BUTTON_6, (char *)"jump forward 5 min" /*g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP9)*/ );
	helpbox.addLine(NEUTRINO_ICON_BUTTON_7, (char *)"jump backward 10 min" /*g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP10)*/);
	helpbox.addLine(NEUTRINO_ICON_BUTTON_8, (char *)"goto end"/*g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP6)*/ );
	helpbox.addLine(NEUTRINO_ICON_BUTTON_9, (char *)"jump forward 10 min" /*g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP11)*/ );
	//helpbox.addLine(g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP12));
	//helpbox.addLine("Version: $Revision: 1.97 $");
	//helpbox.addLine("Movieplayer (c) 2003, 2004 by gagga");
	hide();
	helpbox.show(LOCALE_MESSAGEBOX_INFO);
}

void CMoviePlayerGui::showFileInfoVLC()
{
	Helpbox helpbox;
	std::string url = "http://";
	url += g_settings.streaming_server_ip;
	url += ':';
	url += g_settings.streaming_server_port;
	url += "/requests/status.xml";
	std::string response = "";
	CURLcode httpres = sendGetRequest(url, response);
	
	if (httpres == 0 && response.length() > 0)
	{
		xmlDocPtr answer_parser = parseXml(response.c_str());
		if (answer_parser != NULL)
		{
			xmlNodePtr element = xmlDocGetRootElement(answer_parser);
			element = element->xmlChildrenNode;
			while (element)
			{
				if (strcmp(xmlGetName(element), "information") == 0)
				{
					element = element->xmlChildrenNode;
					break;
				}
				element = element->xmlNextNode;
			}
			
			while (element)
			{
				char *data = xmlGetAttribute(element, "name");
				if (data)
					helpbox.addLine(NEUTRINO_ICON_BUTTON_RED, data);
				xmlNodePtr element1 = element->xmlChildrenNode;
				
				while (element1)
				{
					char tmp[50] = "-- ";
					data = xmlGetAttribute(element1, "name");
					if (data)
					{
						strcat(tmp, data);
						strcat(tmp, " : ");
						data = xmlGetData(element1);
						if (data)
							strcat(tmp, data);
						helpbox.addLine(tmp);
					}
					element1 = element1->xmlNextNode;
				}
				element = element->xmlNextNode;
			}
			xmlFreeDoc(answer_parser);
			hide();
			helpbox.show(LOCALE_MESSAGEBOX_INFO);
		}
	}
}

void CMoviePlayerGui::showFileInfoWebTV()
{
	Helpbox helpbox;
	
	helpbox.addLine(g_file_epg.c_str());
	helpbox.addLine(g_file_epg1.c_str());
	
	hide();
	helpbox.show(LOCALE_MESSAGEBOX_INFO);
}


