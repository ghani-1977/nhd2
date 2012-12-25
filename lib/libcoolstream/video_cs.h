/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _VIDEO_CS_H
#define _VIDEO_CS_H

#include <sys/types.h>

#include <linux/dvb/video.h>
#include <driver/framebuffer.h>

// stm_ioctl
#ifdef __sh__
#include <stm_ioctls.h>
#endif

// bcm
/*   
[%s][%d]: VIDEO_SET_ID
[%s][%d]: VIDEO_SET_STREAMTYPE(%d)
[%s] (%d) -> NEXUS_VideoCodec_eMpeg2(%d)
[%s] (%d) -> NEXUS_VideoCodec_eH264(%d)
[%s] (%d) -> NEXUS_VideoCodec_eH263(%d)
[%s] (%d) -> NEXUS_VideoCodec_eVc1(%d)
[%s] (%d) -> NEXUS_VideoCodec_eMpeg4Part2(%d)
[%s] (%d) -> NEXUS_VideoCodec_eVc1SimpleMain(%d)
[%s] (%d) -> NEXUS_VideoCodec_eMpeg1(%d)
[%s] (%d) -> NEXUS_VideoCodec_eDivx311(%d)
*/ 
#ifndef __sh__
typedef enum {
	VIDEO_STREAMTYPE_MPEG2,
	VIDEO_STREAMTYPE_MPEG4_H264,
	VIDEO_STREAMTYPE_MPEG4_H263,
	VIDEO_STREAMTYPE_VC1,
	VIDEO_STREAMTYPE_MPEG4_Part2,
	VIDEO_STREAMTYPE_VC1_SM,
	VIDEO_STREAMTYPE_MPEG1,
	VIDEO_STREAMTYPE_DIVX311
}VIDEO_FORMAT;
#endif

// video mode
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
PC
//spark7162
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
1080p59 
1080p60 
PC
*/
enum {
	VIDEO_STD_PAL,
	VIDEO_STD_1080I50,
	VIDEO_STD_720P50,
	VIDEO_STD_576P50,
	VIDEO_STD_576I50,
	VIDEO_STD_1080I60,
	VIDEO_STD_720P60,
	VIDEO_STD_1080P24,
	VIDEO_STD_1080P25,
	VIDEO_STD_1080P30,
	VIDEO_STD_1080P50,
	VIDEO_STD_PC
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
enum {
	VIDEO_STD_PAL,
	VIDEO_STD_NTSC,
	VIDEO_STD_480I60,
	VIDEO_STD_576I50,
	VIDEO_STD_480P60,
	VIDEO_STD_576P50,
	VIDEO_STD_720P50,
	VIDEO_STD_720P60,
	VIDEO_STD_1080I50,
	VIDEO_STD_1080I60
};
#endif

// aspect ratio
enum {
	ASPECTRATIO_43,
	ASPECTRATIO_169,
	ASPECTRATIO_AUTO
};

// policy
#ifdef __sh__
enum {
	VIDEOFORMAT_LETTERBOX,
	VIDEOFORMAT_PANSCAN,
	VIDEOFORMAT_FULLSCREEN,
	VIDEOFORMAT_PANSCAN2
};
#else
enum {
	VIDEOFORMAT_LETTERBOX,
	VIDEOFORMAT_PANSCAN,
	VIDEOFORMAT_PANSCAN2,
	VIDEOFORMAT_FULLSCREEN
};
#endif

// input
enum {
	INPUT_ENCODER,
	INPUT_SCART,
	INPUT_AUX
};

// analoge mode
#ifdef __sh__
enum {
	ANALOG_RGB,
	ANALOG_CVBS,
	ANALOG_SVIDEO,
	ANALOG_YUV
};
#else
enum {
	ANALOG_RGB,
	ANALOG_CVBS,
	ANALOG_YUV
};
#endif

// color space
#ifdef __sh__
enum {
	HDMI_RGB,
	HDMI_YUV,
	HDMI_422
};
#else
enum {
	HDMI_AUTO,
	HDMI_RGB,
	HDMI_ITU_R_BT_709,
	HDMI_UNKNOW
};
#endif

// wss
/*
off 
auto 
auto(4:3_off) 
4:3_full_format 
16:9_full_format 
14:9_letterbox_center 
14:9_letterbox_top 
16:9_letterbox_center 
16:9_letterbox_top 
>16:9_letterbox_center 
14:9_full_format
*/
enum {
	WSS_OFF,
	WSS_AUTO,
	WSS_43_OFF,
	WSS_43_FULL,
	WSS_169_FULL,
	WSS_149_LETTERBOX_CENTER,
	WSS_149_LETTERBOX_TOP,
	WSS_169_LETTERBOX_CENTER,
	WSS_169_LETTERBOX_TOP,
	WSS_169_LETTERBOX_CENTER_RIGHT,
	WSS_149_FULL
};


class cVideo
{
	private:
		int video_fd;
		int video_num;

		video_play_state_t playstate;

#ifdef __sh__		
		// encoding
		video_encoding_t EncodingType;
		
		// streamtype
		stream_type_t StreamType;
#else
		VIDEO_FORMAT StreamType;
#endif		

	public:
		/* constructor & destructor */
		cVideo();
		~cVideo(void);

		/* aspect ratio */
		int getAspectRatio(void);
		void getPictureInfo(int &width, int &height, int &rate);
		int setAspectRatio(int ratio, int format);		

		/* blank on freeze */
		int getBlank(void);
		int setBlank(int enable);

		/* get play state */
		int getPlayState(void);

		/* video stream source */
		int setSource(video_stream_source_t source);

		//
		int Start();
		int Stop(bool blank = true);
		bool Pause(void);
		bool Resume(void);
				
		int Flush(void);
		int setSlowMotion(int repeat);
		int setFastForward(int skip);

		bool Open(int num = 0);
		bool Close();
		
		/* set video_system */
		int SetVideoSystem(int video_system);

		int SetSpaceColour(int space_colour);
		
#ifdef __sh__		
		int SetStreamType(stream_type_t type);
		int SetEncoding(video_encoding_t type);
		stream_type_t GetStreamType(void) { return StreamType; };
#else
		void SetStreamType(VIDEO_FORMAT type);
		VIDEO_FORMAT GetStreamType(void) { return StreamType; };
#endif

		void SetSyncMode(int mode);

		void SetInput(int val);
		
		void Pig(int x, int y, int w, int h, int osd_w = CFrameBuffer::getInstance()->getScreenWidth(true), int osd_h = CFrameBuffer::getInstance()->getScreenHeight(true));

		void SetWideScreen(int val);
		void SetAnalogMode(int mode); //analog
		
		// speed normal
#ifdef __sh__		
		int setSpeedNormal();
#endif		
		
		int64_t GetPTS(void);
};

#endif
