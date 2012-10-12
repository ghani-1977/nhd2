/*
	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/



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

#ifndef __vfd__
#define __vfd__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>
#include <string>


// duckbox
// token from micom
enum {
	ICON_MIN,             // 0x00
	ICON_STANDBY,
	ICON_SAT,
	ICON_REC,
	ICON_TIMESHIFT,
	ICON_TIMER,           // 0x05
	ICON_HD,
	ICON_USB,
	ICON_SCRAMBLED,
	ICON_DOLBY,
	ICON_MUTE,            // 0x0a
	ICON_TUNER1,
	ICON_TUNER2,
	ICON_MP3,
	ICON_REPEAT,
	ICON_Play,            // 0x0f
	ICON_TER,            
	ICON_FILE,
	ICON_480i,
	ICON_480p,
	ICON_576i,            // 0x14
	ICON_576p,
	ICON_720p,
	ICON_1080i,
	ICON_1080p,
	ICON_Play_1,          // 0x19 
	ICON_RADIO,   
	ICON_TV,      
	ICON_PAUSE,   
	ICON_MAX
};

// neutrino common
typedef enum
{
	VFD_ICON_MUTE		= ICON_MUTE,
	VFD_ICON_DOLBY		= ICON_DOLBY,
	VFD_ICON_POWER		= ICON_STANDBY,
	VFD_ICON_TIMESHIFT	= ICON_REC,
	VFD_ICON_TV		= ICON_TV,
	VFD_ICON_RADIO		= ICON_TV,
	VFD_ICON_HD		= ICON_HD,
	VFD_ICON_1080P		= ICON_1080p,
	VFD_ICON_1080I		= ICON_1080i,
	VFD_ICON_720P		= ICON_720p,
	VFD_ICON_480P		= ICON_480p,
	VFD_ICON_480I		= ICON_480i,
	VFD_ICON_USB		= ICON_USB,
	VFD_ICON_MP3		= ICON_MP3,
	VFD_ICON_PLAY		= ICON_Play,
	VFD_ICON_PAUSE		= ICON_PAUSE,
	VFD_ICON_LOCK 		= ICON_SCRAMBLED,
} vfd_icon;

#if defined (PLATFORM_DUCKBOX) || defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD) || defined (PLATFORM_SPARK7162)
#define VFDBRIGHTNESS         0xc0425a03
#define VFDPWRLED             0xc0425a04 /* added by zeroone, also used in fp_control/global.h ; set PowerLed Brightness on HDBOX*/
#define VFDDRIVERINIT         0xc0425a08
#define VFDICONDISPLAYONOFF   0xc0425a0a
#define VFDDISPLAYWRITEONOFF  0xc0425a05
#define VFDDISPLAYCHARS       0xc0425a00

#define VFDCLEARICONS	      0xc0425af6
#define VFDSETRF              0xc0425af7
#define VFDSETFAN             0xc0425af8
#define VFDGETWAKEUPMODE      0xc0425af9
#define VFDGETTIME            0xc0425afa
#define VFDSETTIME            0xc0425afb
#define VFDSTANDBY            0xc0425afc
#define VFDREBOOT	      0xc0425afd
#define VFDSETLED             0xc0425afe
#define VFDSETMODE            0xc0425aff

#define VFDGETWAKEUPTIME      0xc0425b00
#define VFDGETVERSION         0xc0425b01
#define VFDSETDISPLAYTIME     0xc0425b02
#define VFDSETTIMEMODE        0xc0425b03

struct vfd_ioctl_data {
	unsigned char start_address;
	unsigned char data[64];
	unsigned char length;
};
#endif


class CVFD
{
	public:

		enum MODES
		{
			MODE_TVRADIO,
			MODE_SCART,
			MODE_SHUTDOWN,
			MODE_STANDBY,
			MODE_MENU_UTF8,
			MODE_AUDIO,
			MODE_PIC,
			MODE_TS,
		};

		enum AUDIOMODES
		{
			AUDIO_MODE_PLAY,
			AUDIO_MODE_STOP,
			AUDIO_MODE_FF,
			AUDIO_MODE_PAUSE,
			AUDIO_MODE_REV
		};


	private:
		MODES			mode;

		std::string		servicename;
		char			volume;
		unsigned char		percentOver;
		bool			muted;
		bool			showclock;
		pthread_t		thrTime;
		int                     last_toggle_state_power;
		bool			clearClock;
		unsigned int            timeout_cnt;
		int fd;
		/*int*/unsigned char brightness;
		char text[256];

		void wake_up();
		void count_down();

		CVFD();

		static void* TimeThread(void*);
		void setlcdparameter(int dimm, int power);
		
		// scroll text
		pthread_t vfd_scrollText;

	public:

		~CVFD();
		bool has_lcd;
		
		void setlcdparameter(void);

		static CVFD * getInstance();
		void init();
		void setMode(const MODES m, const char * const title = "");
		void showServicename(const std::string & name); // UTF-8
		void showTime(bool force = false);
		/** blocks for duration seconds */
		void showRCLock(int duration = 2);
		
		void showMenuText(const int position, const char * text, const int highlight = -1, const bool utf_encoded = false);
		void showAudioTrack(const std::string & artist, const std::string & title, const std::string & album);
		void showAudioPlayMode(AUDIOMODES m=AUDIO_MODE_PLAY);
	
		void setBrightness(int);
		int getBrightness();

		void setBrightnessStandby(int);
		int getBrightnessStandby();

		void setPower(int);
		int getPower();
		void togglePower(void);

		void setMuted(bool);

		void resume();
		void pause();
		void Lock();
		void Unlock();
		void Clear();
		void ShowIcon(vfd_icon icon, bool show);
		void ShowText(char * str);

		void ShowScrollText(char * str);

		static void* ThreadScrollText(void * arg);
		
		void setFan(bool enable);
		void setFPTime(void);
		
#if defined (PLATFORM_DUCKBOX) || defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD) || defined (PLATFORM_SPARK7162)
		void openDevice();
		void closeDevice();
#endif
};


#endif
