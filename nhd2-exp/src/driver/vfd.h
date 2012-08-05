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

//#define VFD_UPDATE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef VFD_UPDATE
// TODO Why is USE_FILE_OFFSET64 not defined, if file.h is included here????
#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64 1
#endif
#include "driver/file.h"
#endif // LCD_UPDATE

#include <pthread.h>
#include <string>

// neutrino common
typedef enum
{
	VFD_ICON_BAR8		= 0x00000004,
	VFD_ICON_BAR7		= 0x00000008,
	VFD_ICON_BAR6		= 0x00000010,
	VFD_ICON_BAR5		= 0x00000020,
	VFD_ICON_BAR4		= 0x00000040,
	VFD_ICON_BAR3		= 0x00000080,
	VFD_ICON_BAR2		= 0x00000100,
	VFD_ICON_BAR1		= 0x00000200,
	VFD_ICON_FRAME		= 0x00000400,
	VFD_ICON_HDD		= 0x00000800,
	VFD_ICON_MUTE		= 0x00001000,
	VFD_ICON_DOLBY		= 0x00002000,
	VFD_ICON_POWER		= 0x00004000,
	VFD_ICON_TIMESHIFT	= 0x00008000,
	VFD_ICON_SIGNAL		= 0x00010000,
	VFD_ICON_TV		= 0x00020000,
	VFD_ICON_RADIO		= 0x00040000,
	VFD_ICON_HD		= 0x01000001,
	VFD_ICON_1080P		= 0x02000001,
	VFD_ICON_1080I		= 0x03000001,
	VFD_ICON_720P		= 0x04000001,
	VFD_ICON_480P		= 0x05000001,
	VFD_ICON_480I		= 0x06000001,
	VFD_ICON_USB		= 0x07000001,
	VFD_ICON_MP3		= 0x08000001,
	VFD_ICON_PLAY		= 0x09000001,
	VFD_ICON_COL1		= 0x09000002,
	VFD_ICON_PAUSE		= 0x0A000001,
	VFD_ICON_CAM1		= 0x0B000001,
	VFD_ICON_COL2		= 0x0B000002,
	VFD_ICON_CAM2		= 0x0C000001
} vfd_icon;


#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
/* cuberevo family boxes */
struct _symbol
{
	unsigned char onoff;
	unsigned char grid;
	unsigned char bit;
};
typedef struct _symbol symbol_t;

struct _vfd_ani
{
	unsigned char kind_cnt;	// 7 : on/off, 6 ~ 0 : kind
	unsigned char dir;
	unsigned char width;
	unsigned char delay;
	
};
typedef struct _vfd_ani vfd_ani_t;

enum vfd_led_type
{
	LED_SET,			/* LEDžŠ ÄÑ°í ²öŽÙ. */
	LED_WAIT,			/* LED¿¡ ±âŽÙž®¶óŽÂ Ç¥œÃžŠ ÇÑŽÙ. */
	LED_WARN,			/* LED¿¡ °æ°í Ç¥œÃžŠ ÇÑŽÙ. */
};

struct _vfd_led
{
	enum vfd_led_type type;
	char val;
};
typedef struct _vfd_led vfd_led_t;

/*
 * ioctl commands
 */
#define TYPE_FRONT		'f'
enum
{
	FRONT_SET_TIME,
	FRONT_GET_TIME,
	FRONT_SET_ALARM,
	FRONT_CLR_ALARM,
	FRONT_GET_ALARM,
	FRONT_GET_ALARM_TIME,
	FRONT_SET_SEG,
	FRONT_SET_SEGS,
	FRONT_GET_AWAKE_ALARM,
	FRONT_GET_AWAKE_AC,
	FRONT_SET_REBOOT,
	FRONT_SET_POWEROFF,
	FRONT_SET_WARMON,
	FRONT_SET_WARMOFF,
	FRONT_SET_WRITERAM,
	FRONT_GET_READRAM,
	FRONT_GET_MICOMVER,
	FRONT_SET_VFDLED,
	FRONT_SET_VFDTIME,
	FRONT_SET_VFDBRIGHT,

	FRONT_FAN_ON,
	FRONT_FAN_OFF,
	FRONT_RFMOD_ON,
	FRONT_RFMOD_OFF,
	FRONT_SET_SEGBUF,
	FRONT_SET_SEGUPDATE,
	FRONT_SET_SCROLLPT,
	FRONT_SET_SEGSCROLL,
	FRONT_SET_ANIMATION,
	FRONT_SET_BUFCLR,

	FRONT_SET_TIMEMODE,
	FRONT_SET_SYMBOL,	/* supported from dotmatrix type front */
	FRONT_SET_1GSYMBOL,
	FRONT_SET_CLRSYMBOL,
};

#define FRONT_IOCS_SYMBOL	_IOW (TYPE_FRONT,FRONT_SET_SYMBOL, symbol_t)
#define FRONT_IOCS_1GSYMBOL	_IOW (TYPE_FRONT,FRONT_SET_1GSYMBOL, symbol_t)
#define FRONT_IOCC_CLRSYMBOL	_IO  (TYPE_FRONT,FRONT_SET_CLRSYMBOL)
#define FRONT_IOCS_ANIMATION	_IOW (TYPE_FRONT,FRONT_SET_ANIMATION, vfd_ani_t)
#define FRONT_IOCS_VFDLED	_IOW (TYPE_FRONT,FRONT_SET_VFDLED, vfd_led_t)
#define FRONT_IOCS_VFDBRIGHT	_IOW (TYPE_FRONT,FRONT_SET_VFDBRIGHT, unsigned char)
#define FRONT_IOCC_FAN_ON	_IO  (TYPE_FRONT,FRONT_FAN_ON )
#define FRONT_IOCC_FAN_OFF	_IO  (TYPE_FRONT,FRONT_FAN_OFF )
#define FRONT_IOCC_SEGUPDATE	_IO  (TYPE_FRONT,FRONT_SET_SEGUPDATE )
#define FRONT_IOCC_BUFCLR	_IO  (TYPE_FRONT,FRONT_SET_BUFCLR )
#endif

#if defined (PLATFORM_DUCKBOX)
struct vfd_ioctl_data {
	unsigned char start_address;
	unsigned char data[64];
	unsigned char length;
};

#define VFDDISPLAYCHARS		0xc0425a00
#define VFDWRITECGRAM		0x40425a01
#define VFDBRIGHTNESS		0xc0425a03
#define VFDICONGETSTATE		0xc0425a0b

/* added by zeroone, also used in nuvoton.h; set PowerLed Brightness on HDBOX*/
#define VFDPWRLED		0xc0425a04

//light on off
#define VFDDISPLAYWRITEONOFF	0xc0425a05
#define VFDDRIVERINIT		0xc0425a08
#define VFDICONDISPLAYONOFF	0xc0425a0a

/* ufs912 */
#define VFDGETVERSION		0xc0425af7
#define VFDLEDBRIGHTNESS	0xc0425af8
#define VFDGETWAKEUPMODE	0xc0425af9

#define VFDGETTIME		0xc0425afa
#define VFDSETTIME		0xc0425afb
#define VFDSTANDBY		0xc0425afc
#define VFDREBOOT		0xc0425afd
#define VFDSETLED		0xc0425afe

/* ufs912, 922, hdbox ->unset compat mode */
#define VFDSETMODE		0xc0425aff
#define VFDDISPLAYCLR		0xc0425b00
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
#ifdef VFD_UPDATE
                ,       MODE_FILEBROWSER,
                        MODE_PROGRESSBAR,
                        MODE_PROGRESSBAR2,
                        MODE_INFOBOX
#endif // LCD_UPDATE
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

#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
		int vfd_symbol( unsigned char grid, unsigned char bit, unsigned char onoff );
		int vfd_1gsymbol( unsigned char grid, unsigned char bit, unsigned char onoff );
		int vfd_symbol_clear();
		void vfd_symbol_sat(int onoff);
		void vfd_symbol_ter(int onoff);
		void vfd_symbol_480i(int onoff);
		void vfd_symbol_480p(int onoff);
		void vfd_symbol_576i(int onoff);
		void vfd_symbol_576p(int onoff);
		void vfd_symbol_720p(int onoff);
		void vfd_symbol_1080i(int onoff);
		void vfd_symbol_1080p(int onoff);
		void vfd_symbol_power(int onoff);
		void vfd_symbol_radio(int onoff);
		void vfd_symbol_tv(int onoff);
		void vfd_symbol_file(int onoff);
		void vfd_symbol_rec(int onoff);
		void vfd_symbol_timeshift(int onoff);
		void vfd_symbol_play(int onoff);
		void vfd_symbol_schedule(int onoff);
		void vfd_symbol_hd(int onoff);
		void vfd_symbol_usb(int onoff);
		void vfd_symbol_lock(int onoff);
		void vfd_symbol_dolby(int onoff);
		void vfd_symbol_pause(int onoff);
		void vfd_symbol_mute(int onoff);
		void vfd_symbol_t1(int onoff);
		void vfd_symbol_t2(int onoff);
		void vfd_symbol_mp3(int onoff);
		void vfd_symbol_repeat(int onoff);
		void do_vfd_sym( char *s, int onoff );
		void vfd_set_icon(vfd_icon icon, bool show);
#endif

	public:

		~CVFD();
		bool has_lcd;
		bool has_vfd;
		bool has_led;
		
		void setlcdparameter(void);

		static CVFD * getInstance();
		void init();
		void setMode(const MODES m, const char * const title = "");
		void showServicename(const std::string & name); // UTF-8
		void showTime(bool force = false);
		/** blocks for duration seconds */
		void showRCLock(int duration = 2);
		void showVolume(const char vol, const bool perform_update = true);
		void showPercentOver(const unsigned char perc, const bool perform_update = true);
		void showMenuText(const int position, const char * text, const int highlight = -1, const bool utf_encoded = false);
		void showAudioTrack(const std::string & artist, const std::string & title, const std::string & album);
		void showAudioPlayMode(AUDIOMODES m=AUDIO_MODE_PLAY);
		void showAudioProgress(const char perc, bool isMuted);
		void setBrightness(int);
		int getBrightness();

		void setBrightnessStandby(int);
		int getBrightnessStandby();

		void setPower(int);
		int getPower();
		void togglePower(void);

		void setInverse(int);
		int getInverse();

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
		
#if defined (PLATFORM_DUCKBOX)
		void openDevice();
		void closeDevice();
#endif

#ifdef VFD_UPDATE
        private:
                CFileList* m_fileList;
                int m_fileListPos;
                std::string m_fileListHeader;

                std::string m_infoBoxText;
                std::string m_infoBoxTitle;
                int m_infoBoxTimer;   // for later use
                bool m_infoBoxAutoNewline;

                bool m_progressShowEscape;
                std::string  m_progressHeaderGlobal;
                std::string  m_progressHeaderLocal;
                int m_progressGlobal;
                int m_progressLocal;
        public:
                MODES getMode(void){return mode;};

                void showFilelist(int flist_pos = -1,CFileList* flist = NULL,const char * const mainDir=NULL);
                void showInfoBox(const char * const title = NULL,const char * const text = NULL,int autoNewline = -1,int timer = -1);
                void showProgressBar(int global = -1,const char * const text = NULL,int show_escape = -1,int timer = -1);
                void showProgressBar2(int local = -1,const char * const text_local = NULL,int global = -1,const char * const text_global = NULL,int show_escape = -1);
#endif // LCD_UPDATE

};


#endif
