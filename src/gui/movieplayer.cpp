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

#include <playback_cs.h>

#include <video_cs.h>
#include <audio_cs.h>

/*zapit includes*/
#include <channel.h>

#include <curl/curl.h>
#include <curl/easy.h>
 

static int streamtype;

#define STREAMTYPE_DVD	1
#define STREAMTYPE_SVCD	2
#define STREAMTYPE_FILE	3

#define MOVIEPLAYER_START_SCRIPT CONFIGDIR "/movieplayer.start" 
#define MOVIEPLAYER_END_SCRIPT CONFIGDIR "/movieplayer.end"


extern int dvbsub_start(int pid);
extern int dvbsub_pause();


cPlayback * playback;
extern CRemoteControl * g_RemoteControl;		/* neutrino.cpp */
extern CZapitChannel * live_channel;			/* zapit.cpp */
extern CInfoViewer * g_InfoViewer;

#define MOVIE_HINT_BOX_TIMER 5				// time to show bookmark hints in seconds

#define MINUTEOFFSET 117*262072
#define MP_TS_SIZE 262072				// ~0.5 sec

extern char rec_filename[512];				// defined in stream2file.cpp

CMoviePlayerGui::state playstate;
bool isMovieBrowser = false;

static int file_prozent;

int speed = 1;
int slow = 0;

int position = 0;
int duration = 0;

int startposition;
int timeshift;
off64_t minuteoffset;
off64_t secondoffset;

#ifndef __USE_FILE_OFFSET64
#error not using 64 bit file offsets
#endif /* __USE_FILE__OFFSET64 */

std::string startfilename;

int jumpminutes = 1;
static int g_jumpseconds = 0;
int buffer_time = 0;
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

bool isHTTP = false;

#define TIMESHIFT_SECONDS 3

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
	{ 0, LOCALE_VIDEOMENU_VIDEORATIO_43 },
	{ 1, LOCALE_VIDEOMENU_VIDEORATIO_169 },
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
	{ 0, LOCALE_VIDEOMENU_LETTERBOX },
	{ 1, LOCALE_VIDEOMENU_PANSCAN },
	{ 2, LOCALE_VIDEOMENU_FULLSCREEN },
	{ 3, LOCALE_VIDEOMENU_PANSCAN2 }
};
#else
// giga
/*
letterbox 
panscan 
bestfit 
nonlinear
*/
#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const CMenuOptionChooser::keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] = 
{
	{ 0, LOCALE_VIDEOMENU_LETTERBOX },
	{ 1, LOCALE_VIDEOMENU_PANSCAN },
	{ 2, LOCALE_VIDEOMENU_PANSCAN2 },
	{ 3, LOCALE_VIDEOMENU_FULLSCREEN }
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

static const char FILENAME[] = "movieplayer.cpp";

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

int CAPIDSelectExec::exec(CMenuTarget * parent, const std::string & actionKey)
{
	apidchanged = 0;
	unsigned int sel = atoi(actionKey.c_str());

	if (g_currentapid != g_apids[sel - 1]) 
	{
		g_currentapid = g_apids[sel - 1];
		g_currentac3 = g_ac3flags[sel - 1];
		apidchanged = 1;
		printf("[movieplayer] apid changed to %d\n", g_apids[sel - 1]);
	}

	return menu_return::RETURN_EXIT;
}

CMoviePlayerGui::CMoviePlayerGui()
{
	Init();
}

void CMoviePlayerGui::Init(void)
{
	stopped = false;

	frameBuffer = CFrameBuffer::getInstance();

	if (strlen(g_settings.network_nfs_moviedir) != 0)
		Path_local = g_settings.network_nfs_moviedir;
	else
		Path_local = "/";
	
	Path_vlc  = "vlc://";
	if ((g_settings.streaming_vlc10 < 2) || (strcmp(g_settings.streaming_server_startdir, "/") != 0))
		Path_vlc += g_settings.streaming_server_startdir;
	Path_vlc_settings = g_settings.streaming_server_startdir;	

	if (g_settings.filebrowser_denydirectoryleave)
		filebrowser = new CFileBrowser(Path_local.c_str());
	else
		filebrowser = new CFileBrowser();

	filebrowser->Multi_Select = false;
	filebrowser->Dirs_Selectable = false;
	//filebrowser->Hide_records = true;

	playback = new cPlayback();
	
	moviebrowser = new CMovieBrowser();

	// tsfilefilter
	tsfilefilter.addFilter("ts");
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
#if !ENABLE_GSTREAMER	
	tsfilefilter.addFilter("flv");
#endif	
	
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

	filebrowser->Filter = &tsfilefilter;
}

CMoviePlayerGui::~CMoviePlayerGui()
{
	//if(filebrowser)
	//	delete filebrowser;
	
	//if (moviebrowser)
	//	delete moviebrowser;

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
	
	#if 1 //FIXME: remove this to main control in neutrino.cpp
	/* hide AC3 Icon */
	if (g_RemoteControl->has_ac3)
		CVFD::getInstance()->ShowIcon(VFD_ICON_DOLBY, false);

	/* show HD Icon */
	if(live_channel)
	{
		if(live_channel->type == 1)
			CVFD::getInstance()->ShowIcon(VFD_ICON_HD, false);
	}
	#endif

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
	
	#if 1 //FIXME: remove this to main control in neutrino.cpp
	/* show AC3 Icon */
	if (g_RemoteControl->has_ac3)
		CVFD::getInstance()->ShowIcon(VFD_ICON_DOLBY, true);

	/* show HD Icon */
	if(live_channel)
	{
		if(live_channel->type == 1)
			CVFD::getInstance()->ShowIcon(VFD_ICON_HD, true);
	}
	#endif

	// tell neutrino that we are in the last mode
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE, m_LastMode);
	
	// start end script
	puts("[movieplayer.cpp] executing " MOVIEPLAYER_END_SCRIPT ".");
	if (system(MOVIEPLAYER_END_SCRIPT) != 0)
		perror("Datei " MOVIEPLAYER_END_SCRIPT " fehlt. Bitte erstellen, wenn gebraucht.\nFile " MOVIEPLAYER_END_SCRIPT " not found. Please create if needed.\n");

	stopped = false;
}

int CMoviePlayerGui::exec(CMenuTarget * parent, const std::string & actionKey)
{
	printf("[movieplayer] actionKey=%s\n", actionKey.c_str());

	dvbsub_pause();
	
	if(Path_vlc_settings != g_settings.streaming_server_startdir)
	{
		Path_vlc  = "vlc://";
		if ((g_settings.streaming_vlc10 < 2) || (strcmp(g_settings.streaming_server_startdir, "/") != 0))
			Path_vlc += g_settings.streaming_server_startdir;
		Path_vlc_settings = g_settings.streaming_server_startdir;
	}	

	if (parent) 
	{
		parent->hide();
	}

	bool usedBackground = frameBuffer->getuseBackground();

	if (usedBackground) 
	{
		frameBuffer->saveBackgroundImage();
		frameBuffer->ClearFrameBuffer();

#if !defined USE_OPENGL
		frameBuffer->blit();
#endif
	}
	
	startfilename = "";
	startposition = 0;

	isMovieBrowser = false;

	minuteoffset = MINUTEOFFSET;
	secondoffset = minuteoffset / 60;

	if (actionKey == "tsmoviebrowser") 
	{
		isMovieBrowser = true;
		moviebrowser->Hide_records = false;
		timeshift = 0;
		isHTTP = false;
		PlayFile();
	}
	if (actionKey == "moviebrowser") 
	{
		isMovieBrowser = true;
		moviebrowser->Hide_records = true;
		timeshift = 0;
		isHTTP = false;
		PlayFile();
	}
	else if (actionKey == "fileplayback") 
	{
		isMovieBrowser = false;
		timeshift = 0;
		//stream
		isHTTP = false;
		
		PlayFile();
	}
	else if (actionKey == "timeshift") 
	{
		timeshift = 1;
		PlayFile();
	} 
	else if (actionKey == "ptimeshift") 
	{
		timeshift = 2;
		PlayFile();
	} 
	else if (actionKey == "showtshelp")
	{
		showHelpTS();
	}
	else if ( actionKey == "vlcplayback" ) 
	{
		isHTTP = true;
		streamtype = STREAMTYPE_FILE;
		isMovieBrowser = false;
		timeshift = 0;
		PlayFile();
	}
	else if ( actionKey == "dvdplayback" ) 
	{
		isHTTP = true;
		streamtype = STREAMTYPE_DVD;
		isMovieBrowser = false;
		timeshift = 0;
		PlayFile();
	}
	else if ( actionKey == "vcdplayback" ) 
	{
		isHTTP = true;
		streamtype = STREAMTYPE_SVCD;
		isMovieBrowser = false;
		timeshift = 0;
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
		timeshift = 0;
		return menu_return::RETURN_EXIT_ALL;
	}

	return menu_return::RETURN_REPAINT;
}

size_t CurlDummyWrite (void *ptr, size_t size, size_t nmemb, void *data)
{
	std::string* pStr = (std::string*) data;
	*pStr += (char*) ptr;
	return size * nmemb;
}

CURLcode sendGetRequest (const std::string & url, std::string & response) 
{
	CURL *curl;
	CURLcode httpres;

	curl = curl_easy_init ();
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

bool VlcRequestStream(char* mrl, int  transcodeVideo, int transcodeAudio)
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
	if(transcodeVideo!=TRANSCODE_VIDEO_OFF || transcodeAudio!=0)
	{
		souturl += "transcode{";
		if(transcodeVideo!=TRANSCODE_VIDEO_OFF)
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
		if(transcodeAudio!=0)
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
//	souturl += g_settings.streaming_server_ip;
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

void * VlcReceiveStreamStart (void *mrl)
{
	printf ("[movieplayer.cpp] ReceiveStreamThread started\n");
	int skt;

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
		return NULL;
		// Assume safely that all succeeding HTTP requests are successful
	}


	int transcodeVideo, transcodeAudio;
	std::string sMRL=(char*)mrl;
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
		transcodeVideo=g_settings.streaming_transcode_video_codec+1;
		if((!memcmp((char*)mrl, "dvd", 3) && !g_settings.streaming_transcode_audio) ||
			(!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "vob") && !g_settings.streaming_transcode_audio) ||
			(!strcasecmp(sMRL.substr(sMRL.length()-3).c_str(), "ac3") && !g_settings.streaming_transcode_audio) ||
			g_settings.streaming_force_avi_rawaudio)
			transcodeAudio=0;
		else
			transcodeAudio=1;
	}
	VlcRequestStream((char*)mrl, transcodeVideo, transcodeAudio);

// TODO: Better way to detect if http://<server>:8080/dboxstream is already alive. For example repetitive checking for HTTP 404.
// Unfortunately HTTP HEAD requests are not supported by VLC :(
// vlc 0.6.3 and up may support HTTP HEAD requests.

// Open HTTP connection to VLC

	const char *server = g_settings.streaming_server_ip.c_str ();
	int port;
	sscanf (g_settings.streaming_server_port, "%d", &port);

	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons (port);
	servAddr.sin_addr.s_addr = inet_addr (server);

	printf ("[movieplayer.cpp] Server: %s\n", server);
	printf ("[movieplayer.cpp] Port: %d\n", port);
	int len;

	while(true)
	{
		//printf ("[movieplayer.cpp] Trying to call socket\n");
		skt = socket (AF_INET, SOCK_STREAM, 0);

		printf ("[movieplayer.cpp] Trying to connect socket\n");
		if(connect(skt, (struct sockaddr *) &servAddr, sizeof (servAddr)) < 0)
		{
			perror ("SOCKET");
			playstate = CMoviePlayerGui::STOPPED;
			return NULL;
		}
		fcntl (skt, O_NONBLOCK);
		printf ("[movieplayer.cpp] Socket OK\n");
       
		// Skip HTTP header
		const char * msg = "GET /dboxstream HTTP/1.0\r\n\r\n";
		int msglen = strlen (msg);
		if(send (skt, msg, msglen, 0) == -1)
		{
			perror ("send()");
			playstate = CMoviePlayerGui::STOPPED;
			return NULL;
		}

		printf ("[movieplayer.cpp] GET Sent\n");

		// Skip HTTP Header
		int found = 0;
		char buf[2];
		char line[200];
		buf[0] = buf[1] = '\0';
		strcpy (line, "");
		while(true)
		{
			len = recv (skt, buf, 1, 0);
			strncat (line, buf, 1);
			if(strcmp (line, "HTTP/1.0 404") == 0)
			{
				printf ("[movieplayer.cpp] VLC still does not send. Retrying...\n");
				close (skt);
				break;
			}
			if((((found & (~2)) == 0) && (buf[0] == '\r')) || /* found == 0 || found == 2 */
				(((found & (~2)) == 1) && (buf[0] == '\n')))  /*   found == 1 || found == 3 */
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
			close(skt);
			return NULL;
		}
	}
	vlc_is_sending:
	printf ("[movieplayer.cpp] Now VLC is sending. Read sockets created\n");
	return NULL;
}

void updateLcd(const std::string & sel_filename)
{
	char tmp[20];
	std::string lcd;

	switch (playstate) 
	{
		case CMoviePlayerGui::PAUSE:
			lcd = "|| ";
			lcd += sel_filename;
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
			lcd = "> ";
			lcd += sel_filename;
			break;
	}
	
#if !defined (PLATFORM_CUBEREVO_250HD) && !defined (PLATFORM_GIGABLUE) && !defined (PLATFORM_XTREND)
	CVFD::getInstance()->showMenuText(0, lcd.c_str(), -1, true);
#endif	
}

void CMoviePlayerGui::PlayFile(void)
{
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	std::string sel_filename;
	CTimeOSD FileTime;
	bool update_lcd = true, open_filebrowser = true, start_play = false, exit = false;
	bool timesh = timeshift;
	bool was_file = false;
	bool time_forced = false;
	playstate = CMoviePlayerGui::STOPPED;
	bool is_file_player = false;
	std::string stream_url;
	bool isVlc = false;
	bool aborted = false;
	char mrl[200];
	CFileList _filelist;
	unsigned int selected = 0;
	std::string title = "";
	bool cdDvd = false;

	if (isHTTP == true)
	{
		stream_url = "http://";
		stream_url += g_settings.streaming_server_ip;
		stream_url += ':';
		stream_url += g_settings.streaming_server_port;
		stream_url += "/dboxstream";
		filename = stream_url.c_str();;
		isVlc = true;
		open_filebrowser = isVlc;
			
		if(streamtype == STREAMTYPE_DVD)
		{
			strcpy (mrl, "dvd://");
			strcat (mrl, g_settings.streaming_server_cddrive);
			strcat (mrl, "@1");
			printf ("[movieplayer.cpp] Generated MRL: %s\n", mrl);
			title = "DVD";
			open_filebrowser = false;
			cdDvd = true;
		}
		else if(streamtype == STREAMTYPE_SVCD)
		{
			strcpy (mrl, "vcd://");
			strcat (mrl, g_settings.streaming_server_cddrive);
			printf ("[movieplayer.cpp] Generated MRL: %s\n", mrl);
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
//#define BOOKMARK_START_MENU_MAX_ITEMS 6
#define BOOKMARK_START_MENU_MAX_ITEMS 5
	CSelectedMenu cSelectedMenuBookStart[BOOKMARK_START_MENU_MAX_ITEMS];

	CMenuWidget bookStartMenu(LOCALE_MOVIEBROWSER_BOOK_NEW, NEUTRINO_ICON_STREAMING);

	bookStartMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_NEW, true, NULL, &cSelectedMenuBookStart[0]));
	bookStartMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_TYPE_FORWARD, true, NULL, &cSelectedMenuBookStart[1]));
	bookStartMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_TYPE_BACKWARD, true, NULL, &cSelectedMenuBookStart[2]));
	bookStartMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_MOVIESTART, true, NULL, &cSelectedMenuBookStart[3]));
	bookStartMenu.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_BOOK_MOVIEEND, true, NULL, &cSelectedMenuBookStart[4]));

 go_repeat:
	do {
		// generate mrl
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
				char *tmp = curl_escape (mrl_str.c_str (), 0);
				strncpy (mrl, tmp, sizeof (mrl) - 1);
				curl_free (tmp);
				printf ("[movieplayer.cpp] Generated FILE MRL: %s\n", mrl);
 
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
#if ENABLE_GSTREAMER		  
			playback->Stop();
#endif			
			exit = false;
			cdDvd = false;
			printf("[movieplayer] stop\n");			
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
			printf("[MoviePlayer] Timeshift: %s\n", sel_filename.c_str());

			update_lcd = true;
			start_play = true;
			open_filebrowser = false;
			timesh = false;
			
			CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

			FileTime.SetMode(CTimeOSD::MODE_ASC);
			FileTime.show( (position / 1000));
		}

		// moviebrowser
		if (isMovieBrowser == true) 
		{
			// do all moviebrowser stuff here ( like commercial jump etc.)
			if (playstate == CMoviePlayerGui::PLAY) 
			{
#if defined (PLATFORM_COOLSTREAM)
				playback->GetPosition(position, duration);
#else				
				playback->GetPosition(position);
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
									TRACE("[mp]  user stop in 5 sec...\r\n");
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
								// *********** we ARE close behind the stop position, stop playing *******************************
								TRACE("[mp]  user stop\r\n");
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
										playback->SetPosition(g_jumpseconds * 1000);
									} 
									else if (p_movie_info->bookmarks.user[book_nr].length > 0) 
									{
										// jump at least 15 seconds
										if (g_jumpseconds < 15)
											g_jumpseconds = 15;
										g_jumpseconds = g_jumpseconds + p_movie_info->bookmarks.user[book_nr].pos;

										//playstate = CMoviePlayerGui::JPOS;	// bookmark  is of type loop, jump backward
										playback->SetPosition(g_jumpseconds * 1000);
									}
									TRACE("[mp]  do jump %d sec\r\n", g_jumpseconds);
									update_lcd = true;
									loop = false;	// do no further bookmark checks
								}
							}
						}
						// check if we shall show the commercial warning
						if (showComHintBox == true) 
						{
							comHintBox.paint();
							TRACE("[mp]  com jump in 5 sec...\r\n");
						} 
						else
							comHintBox.hide();

						// check if we shall show the loop warning
						if (showLoopHintBox == true) 
						{
							loopHintBox.paint();
							TRACE("[mp]  loop jump in 5 sec...\r\n");
						} 
						else
							loopHintBox.hide();
					}
				}
			}
		}// isMovieBrowser == true

		// filebrowser
		if (open_filebrowser) 
		{
			open_filebrowser = false;
			timeshift = false;
			FileTime.hide();

			// clear audipopids
			for (int i = 0; i < g_numpida; i++) 
			{
				g_apids[i] = 0;
				m_apids[i] = 0;
				g_ac3flags[i] = 0;
				g_language[i].clear();
			}
			g_numpida = 0; g_currentapid = 0;

			if (isMovieBrowser == true) 
			{
				// start the moviebrowser instead of the filebrowser
				if (moviebrowser->exec(Path_local.c_str())) 
				{
					// get the current path and file name
					Path_local = moviebrowser->getCurrentDir();
					CFile * file;

					if ((file = moviebrowser->getSelectedFile()) != NULL) 
					{
						CFile::FileType ftype;
						ftype = file->getType();						

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
						
						//
						if(!p_movie_info->epgTitle.empty())
							g_file_epg = p_movie_info->epgTitle;
						else
							g_file_epg = sel_filename;
						
						if(!p_movie_info->epgInfo1.empty())
							g_file_epg1 = p_movie_info->epgInfo1;
						else
							g_file_epg1 = sel_filename;
						//
						
						printf("CMoviePlayerGui::PlayFile: file %s apid 0x%X atype %d vpid 0x%X vtype %d\n", filename, g_currentapid, g_currentac3, g_vpid, g_vtype);
						printf("CMoviePlayerGui::PlayFile: Bytes per minute: %lld\n", minuteoffset);
						
						// get the start position for the movie
						startposition = 1000 * moviebrowser->getCurrentStartPos();
						
						//TRACE("[mp] start pos %llu, %d s Name: %s\r\n", startposition, moviebrowser->getCurrentStartPos(), filename);

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
			else if(isVlc && !cdDvd)
			{
				filename = NULL;
				filebrowser->Filter = &vlcfilefilter;
				if(filebrowser->exec(Path_vlc.c_str()))
				{
					Path_vlc = filebrowser->getCurrentDir ();

					CFile *file = filebrowser->getSelectedFile();
					_filelist.clear();
					_filelist.push_back(*file);

					if(!_filelist.empty())
					{
						filename = _filelist[0].Name.c_str();
						sel_filename = _filelist[0].getFileName();
						//printf ("[movieplayer.cpp] sel_filename: %s\n", filename);
						int namepos = _filelist[0].Name.rfind("vlc://");
						std::string mrl_str = "";
						if (g_settings.streaming_vlc10 > 1)
						{
							mrl_str += "file://";
							if (filename[namepos + 6] != '/')
								mrl_str += "/";
						}
						mrl_str += _filelist[0].Name.substr(namepos + 6);
						char *tmp = curl_escape (mrl_str.c_str (), 0);
						strncpy (mrl, tmp, sizeof (mrl) - 1);
						curl_free (tmp);
						printf ("[movieplayer.cpp] Generated FILE MRL: %s\n", mrl);

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
			else 
			{
				filebrowser->Filter = &tsfilefilter;

				if (filebrowser->exec(Path_local.c_str()) == true) 
				{
					Path_local = filebrowser->getCurrentDir();
					CFile *file;

					if ((file = filebrowser->getSelectedFile()) != NULL) 
					{
						CFile::FileType ftype;
						ftype = file->getType();

						is_file_player = true;

						filename = file->Name.c_str();
						update_lcd = true;
						start_play = true;
						was_file = true;
						sel_filename = filebrowser->getSelectedFile()->getFileName();
						
						//
						g_file_epg = sel_filename;
						g_file_epg1 = sel_filename;
						//
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
				
				CAPIDSelectExec *APIDChanger = new CAPIDSelectExec;
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
							if (apidtitle.find("AC3") < 0 || is_file_player)
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
				//FileTime.show(position / 1000);
			} 
			else 
			{
				FileTime.update((duration - position) / 1000);
				//FileTime.show(position / 1000);
			}
			
			if(!timeshift)
				FileTime.updatePos(file_prozent);
		}

		// start playing
		if (start_play) 
		{
			printf("%s::%s Startplay at %d seconds\n", FILENAME, __FUNCTION__, startposition/1000);

			start_play = false;
			
			if (isVlc)
			{
				playstate = CMoviePlayerGui::SOFTRESET;
				VlcReceiveStreamStart(mrl);

				stream_url = "http://";
				stream_url += g_settings.streaming_server_ip;
				stream_url += ':';
				stream_url += g_settings.streaming_server_port;
				stream_url += "/dboxstream";
				filename = stream_url.c_str();
			}			

			if (playstate >= CMoviePlayerGui::PLAY) 
			{
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
#if defined (PLATFORM_COOLSTREAM)			  
			if(!playback->Start((char *)filename, g_vpid, g_vtype, g_currentapid, g_currentac3, duration))
#else
			if(!playback->Start((char *)filename)) 
#endif
			{
				printf("%s::%s Starting Playback failed!\n", FILENAME, __FUNCTION__);
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

					printf("[movieplayer] Timeshift %d, position %d, seek to %d seconds\n", timeshift, position, startposition/1000);
				}

				// set position 
				if( !is_file_player && startposition >= 0)//FIXME no jump for file at start yet
					playback->SetPosition(startposition);
				
				// get duration
				if(isVlc)
					duration = VlcGetStreamLength();
				else
				{
#if defined (PLATFORM_COOLSTREAM)
					playback->GetPosition(position, duration);
#else					
					playback->GetDuration(duration);
#endif					
				}
			}
		}
		
		// control loop
		//g_RCInput->getMsg(&msg, &data, 10);	// 1 secs
		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR]);
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

		//get position/duration/speed during playing
		if ( playstate >= CMoviePlayerGui::PLAY )
		{
			if(!isVlc)
			{
#if defined (PLATFORM_COOLSTREAM)
				if( playback->GetPosition(position, duration) )
				{
#else			  
				if(playback->GetPosition(position)) 
				{
					playback->GetDuration(duration);
#endif					
					
					if(duration > 100)
						file_prozent = (unsigned char) (position / (duration / 100));

					playback->GetSpeed(speed);
					
					dprintf(DEBUG_DEBUG, "CMoviePlayerGui::PlayFile: speed %d position %d duration %d (%d, %d%%)\n", speed, position, duration, duration-position, file_prozent);			
				}
				else
				{
					sleep(3);
					exit = true;
				}
			}
			else
			{
				duration = VlcGetStreamLength();
				position = VlcGetStreamTime();
				
				if(duration > 100)
					file_prozent = (unsigned char) (position / (duration / 100));
			}
		}
		
		if ( msg == CRCInput::RC_timeout )
		{
			if (FileTime.IsVisible()) 
				FileTime.hide();
		}else
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

			if (isMovieBrowser == true ) 
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
			
			if(timeshift)
				g_RCInput->postMsg((neutrino_msg_t) CRCInput::RC_stop, 0); // this will send msg yes/nos to stop timeshift
				//g_Timerd->stopTimerEvent(CNeutrinoApp::getInstance()->recording_id/*recordingstatus*/); // this stop immediatly timeshift

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

			// movie title
			if(!timeshift)
			{
				if (FileTime.IsVisible()) 
				{
					if (FileTime.GetMode() == CTimeOSD::MODE_ASC) 
					{
						if(timeshift)
							FileTime.hide();
						else
						{
							FileTime.SetMode(CTimeOSD::MODE_DESC);
							FileTime.update((duration - position) / 1000);
							//FileTime.show(position / 1000);
							
							FileTime.updatePos(file_prozent);
						}
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
					
					FileTime.updatePos(file_prozent);
				}
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
						if(timeshift)
							FileTime.hide();
						else
						{
							FileTime.SetMode(CTimeOSD::MODE_DESC);
							FileTime.update((duration - position) / 1000);
							//FileTime.show(position / 1000);
							
							FileTime.updatePos(file_prozent);
						}
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
					
					FileTime.updatePos(file_prozent);
				}
			}
		} 
		else if (msg == (neutrino_msg_t) g_settings.mpkey_bookmark) 
		{
			if (FileTime.IsVisible()) 
				FileTime.hide();
			
			if(isMovieBrowser == true)
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
			showHelpTS();
		}
		else if (msg == CRCInput::RC_info) 
		{
			if(!timeshift)
			{
				if (FileTime.IsVisible()) 
				{
					if (FileTime.GetMode() == CTimeOSD::MODE_ASC) 
					{
						if(timeshift)
							FileTime.hide();
						else
						{
							FileTime.SetMode(CTimeOSD::MODE_DESC);
							FileTime.update((duration - position) / 1000);
							//FileTime.show(position / 1000);
							
							if(!timeshift)
								FileTime.updatePos(file_prozent);
						}
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
					
					FileTime.updatePos(file_prozent);
				}
			}
			else
			if(timeshift)
			{
				if (FileTime.IsVisible()) 
					FileTime.hide();
				else
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
				}
				
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
						if(timeshift)
							FileTime.hide();
						else
						{
							FileTime.SetMode(CTimeOSD::MODE_DESC);
							FileTime.update((duration - position) / 1000);
							//FileTime.show(position / 1000);
							
							FileTime.updatePos(file_prozent);
						}
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
					
					FileTime.updatePos(file_prozent);
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
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
					time_forced = true;
				}
			}
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
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
					time_forced = true;
				}
			}
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
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
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
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
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
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
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
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
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
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
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
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
					time_forced = true;
				}
			}
		} 
		else if ( msg == CRCInput::RC_2 || msg == CRCInput::RC_repeat )
		{	// goto start
			playback->SetPosition(startposition);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_5) 
		{	
			// goto middle
			playback->SetPosition(duration/2);
			
			// time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_8) 
		{	
			// goto end
			playback->SetPosition(duration - 60 * 1000);
			
			//time
			if (!FileTime.IsVisible()) 
			{
				if( !g_InfoViewer->is_visible)
				{
					FileTime.SetMode(CTimeOSD::MODE_ASC);
					FileTime.show(position / 1000);
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
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
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
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
					
					if(!timeshift)
						FileTime.updatePos(file_prozent);
					time_forced = true;
				}
			}
		} 
		else if (msg == CRCInput::RC_0) 
		{	// cancel bookmark jump
			if (isMovieBrowser == true) 
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
			
			if(isHTTP)
				showFileInfoVLC();
			else if (p_movie_info != NULL)
				cMovieInfo.showMovieInfo(*p_movie_info);
		}
		else if(msg == CRCInput::RC_home)
		{
			if (FileTime.IsVisible()) 
				FileTime.hide();
		}
		else if (msg == CRCInput::RC_timeout) 
		{
			// nothing
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
#if ENABLE_GSTREAMER		  
			playback->Stop();
#endif			

			if (isMovieBrowser == true) 
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

	if(FileTime.IsVisible())
		FileTime.hide();
	
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


