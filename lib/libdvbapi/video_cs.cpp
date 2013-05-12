/*
 * 	video_cs.c
 *
 * 	from TDT adapted by mohousch

 *
 * 	Copyright (C) 2011 duckbox project
 *
 *  	This program is free software; you can redistribute it and/or modify
 *  	it under the terms of the GNU General Public License as published by
 *  	the Free Software Foundation; either version 2 of the License, or
 *  	(at your option) any later version.
 *
 *  	This program is distributed in the hope that it will be useful,
 *  	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  	GNU General Public License for more details.
 *
 *  	You should have received a copy of the GNU General Public License
 *  	along with this program; if not, write to the Free Software
 *  	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>

#include <config.h>

#include "video_cs.h"

#include <linux/fb.h>

#include <driver/framebuffer.h>
#include <system/debug.h>


static const char * FILENAME = "[video_cs.cpp]";


cVideo * videoDecoder = NULL;

//ugly most functions are done in proc
/* constructor & destructor */
cVideo::cVideo()
{ 
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
	video_fd = -1;

	playstate = VIDEO_STOPPED;
}

cVideo::~cVideo(void)
{  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	Close();
}

bool cVideo::Open(int num)
{ 
	video_num = 0; // eventually always 0
	
	char devname[32];

	// Open video device
	sprintf(devname, "/dev/dvb/adapter0/video%d", video_num);
	
	if(video_fd > 0)
	{
		printf("%s %s already opened\n", __FUNCTION__, devname);
		return true;
	}

	video_fd = open(devname, O_RDWR);

	if(video_fd > 0)
	{
		dprintf(DEBUG_INFO, "cVideo::Open %s\n", devname);
		return true;
	}
	
	dprintf(DEBUG_INFO, "%s %s failed\n", __FUNCTION__, devname);

	return false;
}

bool cVideo::Close()
{ 
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	
	
	//if(video_fd >= 0)
	close(video_fd);
	video_fd = -1;	

	return true;
}

int cVideo::getAspectRatio(void) 
{  
	int ratio = 0; // 0 = 4:3, 1 = 16:9
	 
#if !defined (PLATFORM_GENERIC)	 
	unsigned char buffer[2];
	int n, fd;

	fd = open("/proc/stb/vmpeg/0/aspect", O_RDONLY);
	n = read(fd, buffer, 2);
	close(fd);
	
	if (n > 0) 
	{
		ratio = atoi((const char*) buffer);
	}
	
	char buf[100];
	
	switch (ratio) 
	{
		case 0:
			sprintf ((char *) buf, "4:3");
			break;
		
		case 1:
			sprintf ((char *) buf, "16:9");
			break;
		case 2:
			sprintf ((char *) buf, "14:9");
			break;
	
		case 3:
			sprintf ((char *) buf, "20:9");
			break;
			
		default:
			strncpy (buf, "unknow", sizeof (buf));
			break;
	}
	
	dprintf(DEBUG_INFO, "%s:%s (ratio=%d) %s\n", FILENAME, __FUNCTION__, ratio, buf);
#endif	
	
	return ratio;
}

/*
letterbox 
panscan 
non 
bestfit
*/
/* set aspect ratio */
int cVideo::setAspectRatio(int ratio, int format) 
{ 
#if !defined (PLATFORM_GENERIC)  
	int fd;

	// aspectratio	
	const char * sRatio[] =
	{
	   	"4:3",
	   	"16:9",
	   	"any" 
        }; 

        fd = open("/proc/stb/video/aspect", O_WRONLY);
	
	write(fd, sRatio[ratio], strlen(sRatio[ratio]));

        close(fd);

	// policy
	fd = open("/proc/stb/video/policy", O_WRONLY);
	
#ifdef __sh__	
	const char* sFormat[]=
	{
		"letterbox",
		"panscan",
		"non",
		"bestfit" 
	};
#else
	const char* sFormat[]=
	{
		"letterbox",
		"panscan", 
		"bestfit",
		"nonlinear"
	};
#endif

	dprintf(DEBUG_INFO, "%s %s (aspect=%d format=%d) set %s %s\n", FILENAME, __FUNCTION__, ratio, format, sRatio[ratio], sFormat[format]);

	write(fd, sFormat[format], strlen((const char*) sFormat[format]));

	close(fd);
#endif	

    	return 0; 
}

void cVideo::getPictureInfo(int &width, int &height, int &rate) 
{
#if !defined (PLATFORM_GENERIC)	  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__); 

  	unsigned char buffer[10];
	int n, fd;	

	// framerate
	rate = 0;
	fd = open("/proc/stb/vmpeg/0/framerate", O_RDONLY);
	n = read(fd, buffer, 10);
	close(fd);

	if (n > 0) 
	{
#ifdef __sh__	  
		sscanf((const char*) buffer, "%X", &rate);
#else
		sscanf((const char*) buffer, "%d", &rate);
#endif		
		rate = rate/1000;
	}

	// width (xres)
	width = 0;
	fd = open("/proc/stb/vmpeg/0/xres", O_RDONLY);
	n = read(fd, buffer, 10);
	close(fd);

	if (n > 0) 
	{
		sscanf((const char*) buffer, "%X", &width);
	}

	// height  (yres)
	height = 0;
	fd = open("/proc/stb/vmpeg/0/yres", O_RDONLY);
	n = read(fd, buffer, 10);
	close(fd);

	if (n > 0) 
	{
		sscanf((const char*) buffer, "%X", &height);
	}	
	
	dprintf(DEBUG_INFO, "%s:%s < w %d, h %d, r %d\n", FILENAME, __FUNCTION__, width, height, rate);
#endif	
}

int cVideo::Start()
{ 
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);

	if (playstate == VIDEO_PLAYING)
		return 0;

	playstate = VIDEO_PLAYING;
	
	// Video Play
	if(ioctl(video_fd, VIDEO_PLAY) < 0)
		perror("VIDEO_PLAY");	
		
	return true;
}

int cVideo::Stop(bool blank)
{ 
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s blank:%d\n", FILENAME, __FUNCTION__, blank);	
		
	playstate = blank ? VIDEO_STOPPED : VIDEO_FREEZED;
	
	if( ioctl(video_fd, VIDEO_STOP, blank ? 1 : 0) < 0 )  
		perror("VIDEO_STOP");	
	
	return true;
}

bool cVideo::Pause(void)
{ 
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
		
	if (ioctl(video_fd, VIDEO_FREEZE) < 0)
		perror("VIDEO_FREEZE");
	
	playstate = VIDEO_FREEZED;	
		
	return true;
}

bool cVideo::Resume(void)
{
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	
		
	if (ioctl(video_fd, VIDEO_CONTINUE) < 0)
		perror("VIDEO_CONTINUE");
	
	playstate = VIDEO_PLAYING;	
		
	return true;
}

int cVideo::Flush(void)
{  
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
	int ret = -1;

#ifdef __sh__
	ret = ioctl(video_fd, VIDEO_FLUSH);
#else
	ret = ioctl(video_fd, VIDEO_CLEAR_BUFFER);
#endif
	if(ret < 0)
		perror("VIDEO_FLUSH");		
	
	return ret;
}

int cVideo::setSlowMotion(int repeat)
{
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "VIDEO_SLOWMOTION(%d) - \n", repeat);
	
	int ret = -1;
		
	ret = ::ioctl(video_fd, VIDEO_SLOWMOTION, repeat);
	if (ret < 0)
		perror("VIDEO_SLOWMOTION");	
	
	return ret;
}

int cVideo::setFastForward(int skip)
{
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "VIDEO_FAST_FORWARD(%d) - \n", skip);
	
	int ret = -1;
		
	ret = ::ioctl(video_fd, VIDEO_FAST_FORWARD, skip);
	if (ret < 0)
		perror("VIDEO_FAST_FORWARD");	

	return ret;
}

/* set video_system */
int cVideo::SetVideoSystem(int video_system)
{	
#ifdef __sh__
/*
pal 
1080i50 
720p50 
576p50 
576i50 
1080i60 
720p60 
1080p24 
1080p25 
1080p30 
1080p50
PC
*/
	const char *aVideoSystems[][2] = {
		{"VIDEO_STD_PAL", "pal"},
		{"VIDEO_STD_1080I50", "1080i50"},
		{"VIDEO_STD_720P50", "720p50"},
		{"VIDEO_STD_576P", "576p50"},
		{"VIDEO_STD_576I", "576i50"},
		{"VIDEO_STD_1080I60", "1080i60"},
		{"VIDEO_STD_720P60", "720p60"},
		{"VIDEO_STD_1080P24", "1080p24"},
		{"VIDEO_STD_1080P25", "1080p25"},
		{"VIDEO_STD_1080P30", "1080p30"},
		{"VIDEO_STD_1080P50", "1080p50"},
		{"VIDEO_STD_PC", "PC"},
	};
#else
// giga
/*
pal 
ntsc 
480i 
576i 
480p 
576p 
720p50 
720p 
1080i50 
1080i
*/
	const char *aVideoSystems[][2] = {
		{"VIDEO_STD_PAL", "pal"},
		{"VIDEO_STD_NTSC", "ntsc"},
		{"VIDEO_STD_480I60", "480i"},
		{"VIDEO_STD_576I50", "576i"},
		{"VIDEO_STD_480P60", "480p"},
		{"VIDEO_STD_576P50", "576p"},
		{"VIDEO_STD_720P50", "720p50"},
		{"VIDEO_STD_720P60", "720p"},
		{"VIDEO_STD_1080P50", "1080i50"},
		{"VIDEO_STD_1080P60", "1080i"},
	};
#endif

	dprintf(DEBUG_INFO, "%s:%s - video_system=%s\n", FILENAME, __FUNCTION__, aVideoSystems[video_system][0]);	

#if !defined (PLATFORM_GENERIC)	
	int fd = open("/proc/stb/video/videomode", O_RDWR);
	write(fd, aVideoSystems[video_system][1], strlen(aVideoSystems[video_system][1]));
	close(fd);
#endif	

	return 0;
}

/* set hdmi space colour */
int cVideo::SetSpaceColour(int colour_space)
{
#ifdef __sh__  
	const char *aCOLORSPACE[] = {
		"hdmi_rgb",
		"hdmi_yuv",
		"hdmi_422"
	};
#else
	const char *aCOLORSPACE[] = {
		"Edid(Auto)",
		"Hdmi_Rgb",
		"Itu_R_BT_709",
		"unknow"
	};
	
#endif
	
	dprintf(DEBUG_INFO, "%s:%s - mode=%s\n", FILENAME, __FUNCTION__, aCOLORSPACE[colour_space]);	

#if !defined (PLATFORM_GENERIC)
#ifdef __sh__
	int fd = open("/proc/stb/avs/0/colorformat", O_RDWR);
#else
	int fd = open("/proc/stb/video/hdmi_colorspace", O_RDWR);
#endif	
	
	write(fd, aCOLORSPACE[colour_space], strlen(aCOLORSPACE[colour_space]));
	
	close(fd);
#endif	

	return 0;
}

#ifdef __sh__
/* set streamtype */
/*
 * List of possible container types - used to select demux..  If stream_source is VIDEO_SOURCE_DEMUX
 * then default is TRANSPORT, if stream_source is VIDEO_SOURCE_MEMORY then default is PES
 */
int cVideo::SetStreamType(stream_type_t type)
{  
	const char * aSTREAMTYPE[] = {
		"STREAM_TYPE_NONE",     /* Deprecated */
		"STREAM_TYPE_TRANSPORT",/* Use latest PTI driver so it can be Deprecated */
		"STREAM_TYPE_PES",
		"STREAM_TYPE_ES",       /* Deprecated */
		"STREAM_TYPE_PROGRAM",  /* Deprecated */
		"STREAM_TYPE_SYSTEM",   /* Deprecated */
		"STREAM_TYPE_SPU",      /* Deprecated */
		"STREAM_TYPE_NAVI",     /* Deprecated */
		"STREAM_TYPE_CSS",      /* Deprecated */
		"STREAM_TYPE_AVI",      /* Deprecated */
		"STREAM_TYPE_MP3",      /* Deprecated */
		"STREAM_TYPE_H264",     /* Deprecated */
		"STREAM_TYPE_ASF",      /* Needs work so it can be deprecated */
		"STREAM_TYPE_MP4",      /* Deprecated */
		"STREAM_TYPE_RAW",     
	};
		
	dprintf(DEBUG_INFO, "%s:%s - Streamtype=%s\n", FILENAME, __FUNCTION__, aSTREAMTYPE[type]);	

	if (ioctl(video_fd, VIDEO_SET_STREAMTYPE, type) < 0)
		perror("VIDEO_SET_STREAMTYPE");
	
	StreamType = type;

	return 0;
}

//int cVideo::SetStreamType(VIDEO_FORMAT type) 
int cVideo::SetEncoding(video_encoding_t type)
{  
	const char *aENCODING[] = {
		"VIDEO_ENCODING_AUTO",
		"VIDEO_ENCODING_MPEG1",
		"VIDEO_ENCODING_MPEG2",
		"VIDEO_ENCODING_MJPEG",
		"VIDEO_ENCODING_DIVX3",
		"VIDEO_ENCODING_DIVX4",
		"VIDEO_ENCODING_DIVX5",
		"VIDEO_ENCODING_MPEG4P2",
		"VIDEO_ENCODING_H264",
		"VIDEO_ENCODING_WMV",
		"VIDEO_ENCODING_VC1",
		"VIDEO_ENCODING_RAW",
		"VIDEO_ENCODING_H263",
		"VIDEO_ENCODING_FLV1",
		"VIDEO_ENCODING_VP6",
		"VIDEO_ENCODING_RMV",
		"VIDEO_ENCODING_DIVXHD",
		"VIDEO_ENCODING_AVS",
		"VIDEO_ENCODING_NONE",
		"VIDEO_ENCODING_PRIVATE"
	};
	
	dprintf(DEBUG_INFO, "%s:%s - Encoding=%s\n", FILENAME, __FUNCTION__, aENCODING[type]);	
		
	if (ioctl(video_fd, VIDEO_SET_ENCODING, type) < 0)
		perror("VIDEO_SET_ENCODING");
	
	EncodingType = type;	

	return 0;
}
#else
void cVideo::SetStreamType(VIDEO_FORMAT type) 
{
	if(video_fd < 0)
		return;
	
	const char *aVIDEOFORMAT[] = {
		"VIDEO_STREAMTYPE_MPEG2",
		"VIDEO_STREAMTYPE_MPEG4_H264",
		"VIDEO_STREAMTYPE_MPEG4_H263",
		"VIDEO_STREAMTYPE_VC1",
		"VIDEO_STREAMTYPE_MPEG4_Part2",
		"VIDEO_STREAMTYPE_VC1_SM",
		"VIDEO_STREAMTYPE_MPEG1",
		"VIDEO_STREAMTYPE_DIVX311"
	};

	dprintf(DEBUG_INFO, "%s:%s - type=%s\n", FILENAME, __FUNCTION__, aVIDEOFORMAT[type]);

	if (ioctl( video_fd, VIDEO_SET_STREAMTYPE, type) < 0)
		perror("VIDEO_SET_STREAMTYPE");	
}
#endif

/* set sync mode */
void cVideo::SetSyncMode(int mode)
{
#ifdef __sh__  
        int clock=0;
	
	const char *aAVSYNCTYPE[] = {
		"AVSYNC_DISABLED",
		"AVSYNC_ENABLED",
		"AVSYNC_AUDIO_IS_MASTER"
	};

        const char* av_modes[] = {
		"disapply",
		"apply"
	};

        const char* master_clock[] = {
		"video",
		"audio"
	};
      	
	dprintf(DEBUG_INFO, "%s:%s - mode=%s\n", FILENAME, __FUNCTION__, aAVSYNCTYPE[mode]);	

	int fd = open("/proc/stb/stream/policy/AV_SYNC", O_RDWR);

        if (fd > 0)  
        {
           	if ((mode == 0) || (mode == 1))
	   	{
	      		write(fd, av_modes[mode], strlen(av_modes[mode]));
	      		clock = 0;
	   	} 
		else
           	{
	      		write(fd, av_modes[1], strlen(av_modes[1]));
	      		clock = 1;
	   	}
	   	close(fd);
        }
	else
	   	printf("error %m\n");
		
        dprintf(DEBUG_INFO, "%s:%s - set master clock = %s\n", FILENAME, __FUNCTION__, master_clock[clock]);	

	fd = open("/proc/stb/stream/policy/MASTER_CLOCK", O_RDWR);
        if (fd > 0)  
        {
	   	write(fd, master_clock[clock], strlen(master_clock[clock]));
	   	close(fd);
        }
	else
	   	printf("error %m\n");
#endif	
}

// setInput
void cVideo::SetInput(int val)
{ 
	const char *input[] = {"encoder", "scart", "aux"};
	const char *sb[] = {"on", "off", "off"};
	
	printf("cVideo::SetInput: %s\n", input[val]);	

#if !defined (PLATFORM_GENERIC)	
	int fd_avs_input = open("/proc/stb/avs/0/input", O_RDWR);
	
	if( fd_avs_input < 0)
		perror("cannot open /proc/stb/avs/0/input");

	if(fd_avs_input > 0)
	{
		write(fd_avs_input, input[val], strlen(input[val]));
	
		close(fd_avs_input);
	}
		
	int fd_sb = open("/proc/stb/avs/0/standby", O_RDWR);
	
	if(fd_sb < 0)
		perror("cannot open /proc/stb/avs/0/standby");
	
	if(fd_sb > 0)
	{
		write(fd_sb, sb[val], strlen(sb[val]));
		close(fd_sb);
	}
#endif	
}

/* Pig */
void cVideo::Pig(int x, int y, int w, int h, int osd_w, int osd_h)
{ 
	//ugly we just resize the video display
	dprintf(DEBUG_INFO, "%s:%s - x=%d y=%d w=%d h=%d\n", FILENAME, __FUNCTION__, x, y, w, h);
	
	int _x, _y, _w, _h;
	/* the target "coordinates" seem to be in a PAL sized plane
	 * TODO: check this in the driver sources */
	int xres = 720;
	int yres = 576;
	
	if (x == -1 && y == -1 && w == -1 && h == -1)
	{
		_w = xres;
		_h = yres;
		_x = 0;
		_y = 0;
	}
	else
	{
		_x = x * xres / osd_w;
		_w = w * xres / osd_w;
		_y = y * yres / osd_h;
		_h = h * yres / osd_h;
	}
	
#if !defined (PLATFORM_GENERIC)	
	FILE* fd;

	fd = fopen("/proc/stb/vmpeg/0/dst_left", "w");
	fprintf(fd, "%x", _x);
	fclose(fd);

	fd = fopen("/proc/stb/vmpeg/0/dst_top", "w");
	fprintf(fd, "%x", _y);
	fclose(fd);

	fd = fopen("/proc/stb/vmpeg/0/dst_width", "w");
	fprintf(fd, "%x", _w);
	fclose(fd);

	fd = fopen("/proc/stb/vmpeg/0/dst_height", "w");
	fprintf(fd, "%x", _h);
	fclose(fd);
#endif	
}

/* set wss */
/*
0: off
1: auto
2: auto(4:3_off)
3: 4:3_full_format
4: 16:9_full_format
5: 14:9_letterbox_center
6: 14:9_letterbox_top
7: 16:9_letterbox_center
8: 16:9_letterbox_top
9: >16:9_letterbox_center
10: 14:9_full_format
*/
//void cVideo::SetWideScreen(bool onoff)
void cVideo::SetWideScreen(int val) // 0 = auto, 1 = auto(4:3_off)
{
#ifdef __sh__  
	const char * wss[] = {
		"off", 
		"auto", 
		"auto(4:3_off)", 
	};
#else
	const char * wss[] = {
		"off", 
		"auto", 
		"auto(4:3_off)", 
		"4:3_full_format", 		//not used
		"16:9_full_format",		//not used
		"14:9_letterbox_center",	//not used 
		"14:9_letterbox_top", 	//not used
		"16:9_letterbox_center",	//not used
		"16:9_letterbox_top", 	//not used
		">16:9_letterbox_center", 	//not used
		"14:9_full_format"		//not used
	};
#endif
	
	dprintf(DEBUG_INFO, "%s:%s - mode=%s\n", FILENAME, __FUNCTION__, wss[val]);

#if !defined (PLATFORM_GENERIC)	
	int fd = open("/proc/stb/denc/0/wss", O_RDWR);
	
	write(fd, wss[val], strlen(wss[val]));

	close(fd);
#endif	
}

/* set video mode */
void cVideo::SetAnalogMode(int mode)
{
  /*
  rgb 
  cvbs 
  yuv 
  */
#ifdef __sh__  
  	const char *aANALOGMODE[] = {
		"rgb",
		"cvbs",
		"svideo"
		"yuv"
	};
#else
	const char *aANALOGMODE[] = {
		"cvbs",
		"rgb",
		"yuv"
	};
#endif
	
	dprintf(DEBUG_INFO, "%s:%s - mode=%s\n", FILENAME, __FUNCTION__, aANALOGMODE[mode]);	

#if !defined (PLATFORM_GENERIC)	
	int fd = open("/proc/stb/avs/0/colorformat", O_RDWR);
	
	write(fd, aANALOGMODE[mode], strlen(aANALOGMODE[mode]));
	
	close(fd);
#endif	
}

/* blank on freeze */
int cVideo::getBlank(void) 
{ 
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	struct video_status status;

	if( ioctl(video_fd, VIDEO_GET_STATUS, &status) < 0)
		perror("VIDEO_GET_STATUS");

	return status.video_blank;
}

/* set blank */
int cVideo::setBlank(int enable) 
{  
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	return ioctl(video_fd, VIDEO_SET_BLANK, enable);
}

/* get play state */
int cVideo::getPlayState(void) 
{ 
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	return playstate; 
}

/* set source */
int cVideo::setSource(video_stream_source_t source)
{
	if(video_fd < 0)
		return -1;
	
	const char *aVIDEOSTREAMSOURCE[] = {
		"VIDEO_SOURCE_DEMUX",
		"VIDEO_SOURCE_MEMORY",
	};
		
	dprintf(DEBUG_INFO, "%s:%s - source=%s\n", FILENAME, __FUNCTION__, aVIDEOSTREAMSOURCE[source]);	
	
	return ioctl(video_fd, VIDEO_SELECT_SOURCE, source);
}

// set speed normal
#ifdef __sh__
int cVideo::setSpeedNormal()
{
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	return ioctl(video_fd, VIDEO_SET_SPEED, DVB_SPEED_NORMAL_PLAY);
}
#endif

int64_t cVideo::GetPTS(void)
{
	if(video_fd < 0)
		return -1;
	
	int64_t pts = 0;
	if (ioctl(video_fd, VIDEO_GET_PTS, &pts) < 0)
		perror("%s: GET_PTS failed");
	
	return pts;
}

