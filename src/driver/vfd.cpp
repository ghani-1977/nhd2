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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <driver/vfd.h>

#include <global.h>
#include <neutrino.h>
#include <system/settings.h>

#include <fcntl.h>
#include <sys/timeb.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <system/debug.h>


//konfetti: let us share the device with evremote and fp_control
//it does currently not support more than one user (see e.g. micom)
#if defined (PLATFORM_DUCKBOX) || defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
bool blocked = false;

void CVFD::openDevice()
{ 
        if (!blocked)
	{
		fd = open("/dev/vfd", O_RDWR);
		if(fd < 0)
		{
			printf("failed to open vfd\n");
			fd = open("/dev/fplarge", O_RDWR);
			if (fd < 0)
			    printf("failed to open fplarge\n");
		}
		else
			blocked = true;
	}
}

void CVFD::closeDevice()
{ 
	if (fd)
	{
		close(fd);
		blocked = false;
	}
	
	fd = -1;
}
#endif

// constructor
CVFD::CVFD()
{
#ifdef VFD_UPDATE
        m_fileList = NULL;
        m_fileListPos = 0;
        m_fileListHeader = "";
        m_infoBoxText = "";
        m_infoBoxAutoNewline = 0;
        m_progressShowEscape = 0;
        m_progressHeaderGlobal = "";
        m_progressHeaderLocal = "";
        m_progressGlobal = 0;
        m_progressLocal = 0;
#endif // VFD_UPDATE

// xtrend 5XXX has no vfd und dreambox has lcd
#if defined (PLATFORM_XTREND) || defined (PLATFORM_DREAMBOX)
	has_vfd = 0;
#else
	has_vfd = 1;
#endif	
	
	text[0] = 0;
	clearClock = 0;

	vfd_scrollText = 0;
}

CVFD::~CVFD()
{ 
//#if !defined (PLATFORM_DUCKBOX) && !defined (PLATFORM_CUBEREVO) && !defined (PLATFORM_CUBEREVO_MINI) && !defined (PLATFORM_CUBEREVO_MINI2) && !defined (PLATFORM_CUBEREVO_MINI_FTA) && !defined (PLATFORM_CUBEREVO_250HD) && !defined (PLATFORM_CUBEREVO_2000HD) && !defined (PLATFORM_CUBEREVO_9500HD)
//	if(fd > 0)
//		close(fd);
//#endif	
}

CVFD * CVFD::getInstance()
{
	static CVFD * lcdd = NULL;

	if(lcdd == NULL) 
	{
		lcdd = new CVFD();
	}

	return lcdd;
}

void CVFD::count_down() 
{
	if (timeout_cnt > 0) 
	{
		timeout_cnt--;
		if (timeout_cnt == 0) 
		{
			if (atoi(g_settings.lcd_setting_dim_brightness) > 0) 
			{
				// save lcd brightness, setBrightness() changes global setting
				int b = g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS];
				setBrightness(atoi(g_settings.lcd_setting_dim_brightness));
				g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS] = b;
			} 
			else 
			{
				//setPower(0);
			}
		}
	} 
}

void CVFD::wake_up() 
{
//	if (atoi(g_settings.lcd_setting_dim_time) > 0) 
//	{
//		timeout_cnt = atoi(g_settings.lcd_setting_dim_time);
//		atoi(g_settings.lcd_setting_dim_brightness) > 0 ?setBrightness(g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS]) : setPower(1);
//	}
//	else
//		setPower(1);
}

void * CVFD::TimeThread(void *)
{
	while(1) 
	{
		sleep(1);
		struct stat buf;
                if (stat("/tmp/vfd.locked", &buf) == -1) 
		{
                        CVFD::getInstance()->showTime();
                        CVFD::getInstance()->count_down();
                } 
		else
                        CVFD::getInstance()->wake_up();
	}

	return NULL;
}

void CVFD::init()
{
	brightness = -1;
	
	// set mode tv/radio
	setMode(MODE_TVRADIO);

	// time thread
	if (pthread_create (&thrTime, NULL, TimeThread, NULL) != 0 ) 
	{
		perror("CVFD::init: pthread_create(TimeThread)");
		return ;
	}
}

void CVFD::setlcdparameter(int dimm, const int power)
{
	if(!has_vfd) 
		return;

	brightness = dimm;

	dprintf(DEBUG_DEBUG, "CVFD::setlcdparameter dimm %d power %d\n", dimm, power);
	
#if defined (PLATFORM_DUCKBOX) || defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
        struct vfd_ioctl_data data;
	data.start_address = dimm;
	
	if(dimm < 1)
		dimm = 1;
	brightness = dimm;
	
	openDevice();
	
	if( ioctl(fd, VFDBRIGHTNESS, &data) < 0)  
		perror("VFDBRIGHTNESS");
	
	closeDevice();
#endif		
}

void CVFD::setlcdparameter(void)
{
	if(!has_vfd) 
		return;

	last_toggle_state_power = g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER];
	
	setlcdparameter( (mode == MODE_STANDBY) ? g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] : g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS], last_toggle_state_power);
}

void CVFD::showServicename(const std::string & name) // UTF-8
{
	if(!has_vfd) 
		return;

	dprintf(DEBUG_DEBUG, "CVFD::showServicename: %s\n", name.c_str());
	
	servicename = name;
	
	if (mode != MODE_TVRADIO)
		return;

	#if 0
	if(g_settings.lcd_setting[SNeutrinoSettings::LCD_SCROLL_TEXT] == 1)
	{
		int len = strlen((char *) servicename.c_str());

		if(len>14)
		{
			ShowScrollText((char *) servicename.c_str());
		}
		else
		{
			if(vfd_scrollText != 0)
			{
				pthread_cancel(vfd_scrollText);
				pthread_join(vfd_scrollText, NULL);
		
				vfd_scrollText = 0;
			}
	
			ShowText((char *) servicename.c_str());
		}
	}
	else
	#endif
	{
		ShowText( (char *) servicename.c_str() );
	}

	wake_up();
}

void CVFD::showTime(bool force)
{
	if(!has_vfd)
		return;

	if (showclock) 
	{
		if (mode == MODE_STANDBY) 
		{
			char timestr[21];
			struct timeb tm;
			struct tm * t;
			static int hour = 0, minute = 0;

			ftime(&tm);
			t = localtime(&tm.time);

			if(force || ((hour != t->tm_hour) || (minute != t->tm_min))) 
			{
				hour = t->tm_hour;
				minute = t->tm_min;
				strftime(timestr, 20, "%H:%M", t);
				ShowText((char *) timestr);
			}
		} 
	}

	if (CNeutrinoApp::getInstance ()->recordingstatus) 
	{
		if(clearClock) 
		{
			clearClock = 0;
		} 
		else 
		{
			clearClock = 1;
		}
	} 
	else if(clearClock) 
	{ 
		// in case icon ON after record stopped
		clearClock = 0;
	}
}

void CVFD::showRCLock(int duration)
{
}

#if ENABLE_LCD
void CVFD::showVolume(const char vol, const bool perform_update)
{
	static int oldpp = 0;
	if(!has_vfd) 
		return;

	ShowIcon(VFD_ICON_MUTE, muted);
	if(vol == volume)
		return;

	volume = vol;
	wake_up();	

	if ((mode == MODE_TVRADIO) && g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME]) 
	{
#if !defined (PLATFORM_DUCKBOX)	&& !defined (PLATFORM_CUBEREVO) && !defined (PLATFORM_CUBEREVO_MINI) && !defined (PLATFORM_CUBEREVO_MINI2) && !defined (PLATFORM_CUBEREVO_MINI_FTA) && !defined (PLATFORM_CUBEREVO_250HD) && !defined (PLATFORM_CUBEREVO_2000HD) && !defined (PLATFORM_CUBEREVO_9500HD)  
		int pp = (int) round((double) vol * (double) 8 / (double) 100);
		if(pp > 8) pp = 8;

		if(oldpp != pp) 
		{
			dprintf(DEBUG_DEBUG, "CVFD::showVolume: %d, bar %d\n", (int) vol, pp);
			
			int i;
			int j = 0x00000200;
			for(i = 0; i < pp; i++) 
			{
				ShowIcon((vfd_icon) j, true);
				j /= 2;
			}
			for(;i < 8; i++) 
			{
				ShowIcon((vfd_icon) j, false);
				j /= 2;
			}
			oldpp = pp;
		}
#else
		int pp = (int) round((double) vol / (double) 2);
		int i;
		int j = pp / 5;
		// v-lines 0-5 = {0x10,0x11,0x12,0x13,0x14,0x15}
		char c1[1] = {0x11};
		char c2[1] = {0x12}; 
		char c3[1] = {0x13};
		char c4[1] = {0x14};
		char c5[1] = {0x15};
		char VolumeBar[15];
		memset (VolumeBar,0,sizeof VolumeBar);
		strcpy(VolumeBar,"   ");
		
		for(i=1; i <= j; i++)
		{
			strncat(VolumeBar,c5,1);
		}
		
		i = pp % 5;
		switch (i)
		{
			case 1:
				strncat(VolumeBar,c1,1);
				break;
			case 2:
				strncat(VolumeBar,c2,1);
				break;
			case 3:
				strncat(VolumeBar,c3,1);
				break;
			case 4:
				strncat(VolumeBar,c4,1);
				break;
		}
		dprintf(DEBUG_DEBUG,"CVFD::showVolume: vol %d - pp %d - fullblocks %d - mod %d - %s\n", vol, pp, j, i, VolumeBar);
		ShowText(VolumeBar);
#endif		
	}	
}

void CVFD::showPercentOver(const unsigned char perc, const bool perform_update)
{
	if(!has_vfd) 
		return;

	if ((mode == MODE_TVRADIO) && !(g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME])) 
	{
		if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] == 0) 
		{
#if !defined (PLATFORM_DUCKBOX) && !defined (PLATFORM_CUBEREVO) && !defined (PLATFORM_CUBEREVO_MINI) && !defined (PLATFORM_CUBEREVO_MINI2) && !defined (PLATFORM_CUBEREVO_MINI_FTA) && !defined (PLATFORM_CUBEREVO_250HD) && !defined (PLATFORM_CUBEREVO_2000HD) && !defined (PLATFORM_CUBEREVO_9500HD)
			ShowIcon(VFD_ICON_FRAME, true);
			int pp;
			if(perc == 255)
				pp = 0;
			else
				pp = (int) round((double) perc * (double) 8 / (double) 100);

			dprintf(DEBUG_DEBUG, "CVFD::showPercentOver: %d, bar %d\n", (int) perc, pp);

			if(pp > 8) 
				pp = 8;

			if(pp != percentOver) 
			{
				int i;
				int j = 0x00000200;
				for(i = 0; i < pp; i++) 
				{
					ShowIcon((vfd_icon) j, true);
					j /= 2;
				}

				for(;i < 8; i++) 
				{
					ShowIcon((vfd_icon) j, false);
					j /= 2;
				}
				percentOver = pp;
			}
#endif			
		}
	}	
}
#endif

void CVFD::showMenuText(const int position, const char * text, const int highlight, const bool utf_encoded)
{
	if(!has_vfd) 
		return;

	if (mode != MODE_MENU_UTF8)
		return;

	ShowText((char *) text);
	wake_up();
}

void CVFD::showAudioTrack(const std::string & artist, const std::string & title, const std::string & album)
{
	if(!has_vfd) 
		return;

	if (mode != MODE_AUDIO) 
		return;

	dprintf(DEBUG_DEBUG, "CVFD::showAudioTrack: %s\n", title.c_str());
	
	ShowText((char *) title.c_str());
	wake_up();
}

void CVFD::showAudioPlayMode(AUDIOMODES m)
{
	if(!has_vfd) 
		return;

	switch(m) 
	{
		case AUDIO_MODE_PLAY:
			ShowIcon(VFD_ICON_PLAY, true);
			ShowIcon(VFD_ICON_PAUSE, false);
			break;
			
		case AUDIO_MODE_STOP:
			ShowIcon(VFD_ICON_PLAY, false);
			ShowIcon(VFD_ICON_PAUSE, false);
			break;
			
		case AUDIO_MODE_PAUSE:
			ShowIcon(VFD_ICON_PLAY, false);
			ShowIcon(VFD_ICON_PAUSE, true);
			break;
			
		case AUDIO_MODE_FF:
			break;
			
		case AUDIO_MODE_REV:
			break;
	}

	wake_up();
}

/*void CVFD::showAudioProgress(const char perc, bool isMuted)
{
	if(!has_vfd) 
		return;

#ifdef HAVE_LCD
	if (mode == MODE_AUDIO) 
	{
		int dp = int( perc/100.0*61.0+12.0);
		if(isMuted) 
		{
			if(dp > 12) 
			{
				display.draw_line(12, 56, dp-1, 56, CLCDDisplay::PIXEL_OFF);
				display.draw_line(12, 58, dp-1, 58, CLCDDisplay::PIXEL_OFF);
			}
			else
				display.draw_line (12,55,72,59, CLCDDisplay::PIXEL_ON);
		}
	}
#endif
}*/

void CVFD::setMode(const MODES m, const char * const title)
{
	if(!has_vfd) 
		return;

	//if(mode == MODE_AUDIO)
	//	ShowIcon(VFD_ICON_MP3, false);
#if 0
	else if(mode == MODE_STANDBY) 
	{
		ShowIcon(VFD_ICON_COL1, false);
		ShowIcon(VFD_ICON_COL2, false);
	}
#endif

	// sow title
	if(strlen(title))
	{
		ShowText((char *) title);
	}

	mode = m;

	setlcdparameter();

	switch (m) 
	{
		case MODE_TVRADIO:
#if ENABLE_LCD
			switch (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME])
			{
				case 0:
					showPercentOver(percentOver, false);
					break;
				case 1:
					showVolume(volume, false);
					break;

				case 2:
					showVolume(volume, false);
					showPercentOver(percentOver, false);
					break;
			}
#endif			

#if !defined (PLATFORM_CUBEREVO_250HD) && !defined (PLATFORM_GIGABLUE)	
			showServicename(servicename);
#endif

#if !defined (PLATFORM_DUCKBOX) 
			ShowIcon(VFD_ICON_TV, true);
#endif			
			showclock = true;
			//showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
			break;

		case MODE_AUDIO:
		{
			ShowIcon(VFD_ICON_MP3, true);
#if !defined (PLATFORM_DUCKBOX)			
			ShowIcon(VFD_ICON_TV, false);
#endif			
			showAudioPlayMode(AUDIO_MODE_STOP);
#if ENABLE_LCD			
			showVolume(volume, false);
#endif			
			showclock = true;
			ShowIcon(VFD_ICON_HD, false);
			ShowIcon(VFD_ICON_DOLBY, false);
			//showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
			break;
		}

		case MODE_SCART:
#if !defined (PLATFORM_DUCKBOX)		  
			ShowIcon(VFD_ICON_TV, false);
#endif

#if ENABLE_LCD
			showVolume(volume, false);
#endif			
			showclock = true;
			//showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
			break;

		case MODE_MENU_UTF8:
			showclock = false;
			//fonts.menutitle->RenderString(0,28, 140, title, CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
			break;

		case MODE_SHUTDOWN:
			//ShowIcon(VFD_ICON_TV, false);
			/* clear all symbols */
			Clear();
			showclock = false;
			break;

		case MODE_STANDBY:
#if !defined (PLATFORM_DUCKBOX)
			ShowIcon(VFD_ICON_TV, false);
#endif			
			showclock = true;
			showTime(true);      	/* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
						/* "showTime()" clears the whole lcd in MODE_STANDBY */
			break;
		
		case MODE_PIC:
#if !defined (PLATFORM_DUCKBOX)		  
			ShowIcon(VFD_ICON_TV, false);
#endif			
			ShowIcon(VFD_ICON_HD, false);
			ShowIcon(VFD_ICON_DOLBY, false);
			
			showclock = false;
			break;
			
		case MODE_TS:
#if !defined (PLATFORM_DUCKBOX)		  
			ShowIcon(VFD_ICON_TV, false);
#endif			
			showclock = false;
			break;
		
#if ENABLE_LCD
		case MODE_FILEBROWSER:
			ShowIcon(VFD_ICON_TV, false);
			showclock = true;
			display.draw_fill_rect(-1, -1, 120, 64, CLCDDisplay::PIXEL_OFF); // clear lcd
			showFilelist();
			break;
			
		case MODE_PROGRESSBAR:
			showclock = false;
			display.load_screen(&(background[BACKGROUND_SETUP]));
			showProgressBar();
			break;
			
		case MODE_PROGRESSBAR2:
			showclock = false;
			display.load_screen(&(background[BACKGROUND_SETUP]));
			showProgressBar2();
			break;
			
		case MODE_INFOBOX:
			showclock = false;
			showInfoBox();
			break;
			
#endif // ENABLE_LCD
	}

	wake_up();
}

void CVFD::setBrightness(int bright)
{
	if(!has_vfd) 
		return;

	g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS] = bright;
	
	setlcdparameter();
}

int CVFD::getBrightness()
{
	//FIXME for old neutrino.conf
	if(g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS] > DEFAULT_LCD_BRIGHTNESS)
		g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS] = DEFAULT_LCD_BRIGHTNESS;

	return g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS];
}

void CVFD::setBrightnessStandby(int bright)
{
	if(!has_vfd) 
		return;

	g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] = bright;
	setlcdparameter();
}

int CVFD::getBrightnessStandby()
{
	//FIXME for old neutrino.conf
	if(g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] > DEFAULT_LCD_STANDBYBRIGHTNESS )
		g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] = DEFAULT_LCD_STANDBYBRIGHTNESS;
	
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS];
}

void CVFD::setPower(int power)
{
	if(!has_vfd) 
		return;

#if defined (PLATFORM_DUCKBOX) || defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
	struct vfd_ioctl_data data;
	data.start_address = power;
	
	openDevice();
	
	if( ioctl(fd, VFDPWRLED, &data) < 0)  
		perror("VFDPWRLED");
	
	closeDevice();
#endif
}

void CVFD::setFPTime(void)
{
	if(!has_vfd)
		return;

#if defined (PLATFORM_DUCKBOX) || defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
	openDevice();
	
	if( ioctl(fd, VFDSETTIME) < 0)  
		perror("VFDPWRLED");
	
	closeDevice();
#endif
}

int CVFD::getPower()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER];
}

void CVFD::togglePower(void)
{
	if(!has_vfd) 
		return;

	last_toggle_state_power = 1 - last_toggle_state_power;
	setlcdparameter((mode == MODE_STANDBY) ? g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] : g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS], last_toggle_state_power);
}

void CVFD::setMuted(bool mu)
{
	if(!has_vfd) 
		return;
	
	muted = mu;
#if ENABLE_LCD	
	showVolume(volume);
#endif	
}

void CVFD::resume()
{
	if(!has_vfd) 
		return;
}

void CVFD::pause()
{
	if(!has_vfd) 
		return;
}

void CVFD::Lock()
{
	if(!has_vfd) 
		return;

	creat("/tmp/vfd.locked", 0);
}

void CVFD::Unlock()
{
	if(!has_vfd) 
		return;

	unlink("/tmp/vfd.locked");
}

void CVFD::Clear()
{
	if(!has_vfd) 
		return;
	
#if defined (PLATFORM_GIGABLUE)
	ShowText("    "); // 4 empty digits
#elif defined (PLATFORM_DUCKBOX) || defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
        //struct vfd_ioctl_data data;
	//data.start = 0x01;
	//data.length = 0x0;
	
	struct vfd_ioctl_data data;
   
	data.start_address = 0;
	
	openDevice();
	
	if( ioctl(fd, /*VFDDISPLAYCLR*/VFDDISPLAYWRITEONOFF, &data) < 0)
		perror("VFDDISPLAYCLR");
	
	closeDevice();
#endif
}

void CVFD::ShowIcon(vfd_icon icon, bool show)
{
	dprintf(DEBUG_DEBUG, "CVFD::ShowIcon %s %x\n", show ? "show" : "hide", (int) icon);

#if defined (PLATFORM_DUCKBOX) || defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
	openDevice();

	struct vfd_ioctl_data data;

	data.data[0] = (icon - 1) & 0x1F;
	data.data[4] = show ? 1 : 0;
	
	if( ioctl(fd, VFDICONDISPLAYONOFF, &data) < 0)
		perror("VFDICONDISPLAYONOFF");
	
	closeDevice();
#endif
}

void* CVFD::ThreadScrollText(void * arg)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	int vfd = open("/dev/vfd", O_RDWR);

	int i;
	char *str = (char *)arg;
	int len = strlen(str);
	char out[15];

	memset(out, 0, 15);

	int retries = 1;

	while(retries--)
	{
		usleep(500000);
			
		for (i=0; i<=(len-14); i++) 
		{ 
			// scroll text till end
			memset(out, ' ', 14);
			memcpy(out, str+i, 14);
			write(vfd, out, strlen(out));
			usleep(500000);
		}
	
		memcpy(out, str, 14); // display first 13 chars after scrolling
		write(vfd, out, strlen(out));
	}
	
	close(vfd);
	
	pthread_exit(0);

	return NULL;
}

void CVFD::ShowScrollText(char *str)
{
	int len = strlen(str);
	int ret;

	//stop scrolltextthread
	if(vfd_scrollText != 0)
	{
		pthread_cancel(vfd_scrollText);
		pthread_join(vfd_scrollText, NULL);

		vfd_scrollText = 0;
	}

	//scroll text thread
	pthread_create(&vfd_scrollText, NULL, ThreadScrollText, (void *)str);
}

void CVFD::ShowText(char *str)
{
	dprintf(DEBUG_DEBUG, "CVFD::ShowText: [%s]\n", str);

#if defined (PLATFORM_DUCKBOX) || defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
	int len = strlen(str);
	int i;
	
	for(i = len-1; i > 0; i--) 
	{
		if(str[i] == ' ')
			str[i] = 0;
		else
			break;
	}
	  
	openDevice();
	
	printf("CVFD::ShowText >%s< - %d\n", str, len);
	
	if( write(fd , str, len > 16? 16 : len ) < 0)
		perror("write to vfd failed");
	
	closeDevice();
#elif defined (PLATFORM_GIGABLUE)	
	FILE *f;
	if((f = fopen("/proc/vfd","w")) == NULL) 
	{
		printf("CVFD::ShowText cannotopen /proc/vfd\n");
	
		return;
	}
	
	fprintf(f,"%s", str);
	
	fclose(f);
#endif
}

void CVFD::setFan(bool enable)
{
#if defined (PLATFORM_DUCKBOX) || defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)  
	//openDevice();
	
	//if( ioctl(fd, VFDSETFAN, enable) < 0)  
	//	perror("VFDPWRLED");
	
	//closeDevice();
#endif	
}

#if ENABLE_LCD
/*****************************************************************************************/
// showInfoBox
/*****************************************************************************************/
#define LCD_WIDTH 120
#define LCD_HEIGTH 64

#define EPG_INFO_FONT_HEIGHT 9
#define EPG_INFO_SHADOW_WIDTH 1
#define EPG_INFO_LINE_WIDTH 1
#define EPG_INFO_BORDER_WIDTH 2

#define EPG_INFO_WINDOW_POS 4
#define EPG_INFO_LINE_POS       EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH
#define EPG_INFO_BORDER_POS EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH + EPG_INFO_LINE_WIDTH
#define EPG_INFO_TEXT_POS       EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH + EPG_INFO_LINE_WIDTH + EPG_INFO_BORDER_WIDTH

#define EPG_INFO_TEXT_WIDTH LCD_WIDTH - (2*EPG_INFO_WINDOW_POS)

// timer 0: OFF, timer>0 time to show in seconds,  timer>=999 endless
void CVFD::showInfoBox(const char * const title, const char * const text ,int autoNewline,int timer)
{
#ifdef HAVE_LCD
	if(!has_vfd) 
		return;
        //printf("[lcdd] Info: \n");
        if(text != NULL)
                m_infoBoxText = text;
        if(text != NULL)
                m_infoBoxTitle = title;
        if(timer != -1)
                m_infoBoxTimer = timer;
        if(autoNewline != -1)
                m_infoBoxAutoNewline = autoNewline;

        //printf("[lcdd] Info: %s,%s,%d,%d\n",m_infoBoxTitle.c_str(),m_infoBoxText.c_str(),m_infoBoxAutoNewline,m_infoBoxTimer);
        if( mode == MODE_INFOBOX && !m_infoBoxText.empty())
        {
                // paint empty box
                display.draw_fill_rect (EPG_INFO_WINDOW_POS, EPG_INFO_WINDOW_POS,       LCD_WIDTH-EPG_INFO_WINDOW_POS+1,          LCD_HEIGTH-EPG_INFO_WINDOW_POS+1,    CLCDDisplay::PIXEL_OFF);
                display.draw_fill_rect (EPG_INFO_LINE_POS,       EPG_INFO_LINE_POS,     LCD_WIDTH-EPG_INFO_LINE_POS-1,    LCD_HEIGTH-EPG_INFO_LINE_POS-1,        CLCDDisplay::PIXEL_ON);
                display.draw_fill_rect (EPG_INFO_BORDER_POS, EPG_INFO_BORDER_POS,       LCD_WIDTH-EPG_INFO_BORDER_POS-3,  LCD_HEIGTH-EPG_INFO_BORDER_POS-3, CLCDDisplay::PIXEL_OFF);

                // paint title
                if(!m_infoBoxTitle.empty())
                {
                        int width = fonts.menu->getRenderWidth(m_infoBoxTitle.c_str(),true);
                        if(width > 100)
                                width = 100;
                        int start_pos = (120-width) /2;
                        display.draw_fill_rect (start_pos, EPG_INFO_WINDOW_POS-4,       start_pos+width+5,        EPG_INFO_WINDOW_POS+10,    CLCDDisplay::PIXEL_OFF);
                        fonts.menu->RenderString(start_pos+4,EPG_INFO_WINDOW_POS+5, width+5, m_infoBoxTitle.c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
                }

                // paint info
                std::string text_line;
                int line;
                int pos = 0;
                int length = m_infoBoxText.size();
                for(line = 0; line < 5; line++)
                {
                        text_line.clear();
                        while ( m_infoBoxText[pos] != '\n' &&
                                        ((fonts.menu->getRenderWidth(text_line.c_str(), true) < EPG_INFO_TEXT_WIDTH-10) || !m_infoBoxAutoNewline )&&
                                        (pos < length)) // UTF-8
                        {
                                if ( m_infoBoxText[pos] >= ' ' && m_infoBoxText[pos] <= '~' )  // any char between ASCII(32) and ASCII (126)
                                        text_line += m_infoBoxText[pos];
                                pos++;
                        }
                        //printf("[lcdd] line %d:'%s'\r\n",line,text_line.c_str());
                        fonts.menu->RenderString(EPG_INFO_TEXT_POS+1,EPG_INFO_TEXT_POS+(line*EPG_INFO_FONT_HEIGHT)+EPG_INFO_FONT_HEIGHT+3, EPG_INFO_TEXT_WIDTH, text_line.c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
                        if ( m_infoBoxText[pos] == '\n' )
                                pos++; // remove new line
                }
                displayUpdate();
        }
#endif
}

/*****************************************************************************************/
//showFilelist
/*****************************************************************************************/
#define BAR_POS_X               114
#define BAR_POS_Y                10
#define BAR_POS_WIDTH     6
#define BAR_POS_HEIGTH   40

void CVFD::showFilelist(int flist_pos,CFileList* flist,const char * const mainDir)
{
#ifdef HAVE_LCD
	if(!has_vfd) 
		return;
        //printf("[lcdd] FileList\n");
        if(flist != NULL)
                m_fileList = flist;
        if(flist_pos != -1)
                m_fileListPos = flist_pos;
        if(mainDir != NULL)
                m_fileListHeader = mainDir;

        if (mode == MODE_FILEBROWSER && m_fileList != NULL && m_fileList->size() > 0)
        {
                dprintf(DEBUG_DEBUG, "[lcdd] FileList:OK\n");
                int size = m_fileList->size();

                display.draw_fill_rect(-1, -1, 120, 52, CLCDDisplay::PIXEL_OFF); // clear lcd

                if(m_fileListPos > size)
                        m_fileListPos = size-1;

                int width = fonts.menu->getRenderWidth(m_fileListHeader.c_str(), true);
                if(width>110)
                        width=110;
                fonts.menu->RenderString((120-width)/2, 11, width+5, m_fileListHeader.c_str(), CLCDDisplay::PIXEL_ON);

                //printf("list%d,%d\r\n",m_fileListPos,(*m_fileList)[m_fileListPos].Marked);
                std::string text;
                int marked;
                if(m_fileListPos >  0)
                {
                        if ( (*m_fileList)[m_fileListPos-1].Marked == false )
                        {
                                text ="";
                                marked = CLCDDisplay::PIXEL_ON;
                        }
                        else
                        {
                                text ="*";
                                marked = CLCDDisplay::PIXEL_INV;
                        }
                        text += (*m_fileList)[m_fileListPos-1].getFileName();
                        fonts.menu->RenderString(1, 12+12, BAR_POS_X+5, text.c_str(), marked);
                }
                if(m_fileListPos <  size)
                {
                        if ((*m_fileList)[m_fileListPos-0].Marked == false )
                        {
                                text ="";
                                marked = CLCDDisplay::PIXEL_ON;
                        }
                        else
                        {
                                text ="*";
                                marked = CLCDDisplay::PIXEL_INV;
                        }
                        text += (*m_fileList)[m_fileListPos-0].getFileName();
                        fonts.time->RenderString(1, 12+12+14, BAR_POS_X+5, text.c_str(), marked);
                }
                if(m_fileListPos <  size-1)
                {
                        if ((*m_fileList)[m_fileListPos+1].Marked == false )
                        {
                                text ="";
                                marked = CLCDDisplay::PIXEL_ON;
                        }
                        else
                        {
                                text ="*";
                                marked = CLCDDisplay::PIXEL_INV;
                        }
                        text += (*m_fileList)[m_fileListPos+1].getFileName();
                        fonts.menu->RenderString(1, 12+12+14+12, BAR_POS_X+5, text.c_str(), marked);
                }
                // paint marker
                int pages = (((size-1)/3 )+1);
                int marker_length = (BAR_POS_HEIGTH-2) / pages;
                if(marker_length <4)
                        marker_length=4;// not smaller than 4 pixel
                int marker_offset = ((BAR_POS_HEIGTH-2-marker_length) * m_fileListPos) /size  ;
                //printf("%d,%d,%d\r\n",pages,marker_length,marker_offset);

                display.draw_fill_rect (BAR_POS_X,   BAR_POS_Y,   BAR_POS_X+BAR_POS_WIDTH,   BAR_POS_Y+BAR_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
                display.draw_fill_rect (BAR_POS_X+1, BAR_POS_Y+1, BAR_POS_X+BAR_POS_WIDTH-1, BAR_POS_Y+BAR_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
                display.draw_fill_rect (BAR_POS_X+1, BAR_POS_Y+1+marker_offset, BAR_POS_X+BAR_POS_WIDTH-1, BAR_POS_Y+1+marker_offset+marker_length, CLCDDisplay::PIXEL_ON);

                displayUpdate();
        }
#endif
}

//showProgressBar
#define PROG_GLOB_POS_X 10
#define PROG_GLOB_POS_Y 30
#define PROG_GLOB_POS_WIDTH 100
#define PROG_GLOB_POS_HEIGTH 20
void CVFD::showProgressBar(int global, const char * const text,int show_escape,int timer)
{
#ifdef HAVE_LCD
	if(!has_vfd) 
		return;
        if(text != NULL)
                m_progressHeaderGlobal = text;

        if(timer != -1)
                m_infoBoxTimer = timer;

        if(global >= 0)
        {
                if(global > 100)
                        m_progressGlobal = 100;
                else
                        m_progressGlobal = global;
        }

        if(show_escape != -1)
                m_progressShowEscape = show_escape;

        if (mode == MODE_PROGRESSBAR)
        {
                //printf("[lcdd] prog:%s,%d,%d\n",m_progressHeaderGlobal.c_str(),m_progressGlobal,m_progressShowEscape);
                // Clear Display
                display.draw_fill_rect (0,12,120,64, CLCDDisplay::PIXEL_OFF);

                // paint progress header
                int width = fonts.menu->getRenderWidth(m_progressHeaderGlobal.c_str(),true);
                if(width > 100)
                        width = 100;
                int start_pos = (120-width) /2;
                fonts.menu->RenderString(start_pos, 12+12, width+10, m_progressHeaderGlobal.c_str(), CLCDDisplay::PIXEL_ON,0,true);

                // paint global bar
                int marker_length = (PROG_GLOB_POS_WIDTH * m_progressGlobal)/100;

                display.draw_fill_rect (PROG_GLOB_POS_X,                                 PROG_GLOB_POS_Y,   PROG_GLOB_POS_X+PROG_GLOB_POS_WIDTH,   PROG_GLOB_POS_Y+PROG_GLOB_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
                display.draw_fill_rect (PROG_GLOB_POS_X+1+marker_length, PROG_GLOB_POS_Y+1, PROG_GLOB_POS_X+PROG_GLOB_POS_WIDTH-1, PROG_GLOB_POS_Y+PROG_GLOB_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);

                // paint foot
                if(m_progressShowEscape  == true)
                {
                        fonts.menu->RenderString(90, 64, 40, "Home", CLCDDisplay::PIXEL_ON);
                }
                displayUpdate();
        }
#endif
}

// showProgressBar2
#define PROG2_GLOB_POS_X 10
#define PROG2_GLOB_POS_Y 24
#define PROG2_GLOB_POS_WIDTH 100
#define PROG2_GLOB_POS_HEIGTH 10

#define PROG2_LOCAL_POS_X 10
#define PROG2_LOCAL_POS_Y 37
#define PROG2_LOCAL_POS_WIDTH PROG2_GLOB_POS_WIDTH
#define PROG2_LOCAL_POS_HEIGTH PROG2_GLOB_POS_HEIGTH

void CVFD::showProgressBar2(int local,const char * const text_local ,int global ,const char * const text_global ,int show_escape )
{
#ifdef HAVE_LCD
	if(!has_vfd) return;
        //printf("[lcdd] prog2\n");
        if(text_local != NULL)
                m_progressHeaderLocal = text_local;

        if(text_global != NULL)
                m_progressHeaderGlobal = text_global;

        if(global >= 0)
        {
                if(global > 100)
                        m_progressGlobal =100;
                else
                        m_progressGlobal = global;
        }
        if(local >= 0)
        {
                if(local > 100)
                        m_progressLocal =100;
                else
                        m_progressLocal = local;
        }
        if(show_escape != -1)
                m_progressShowEscape = show_escape;

        if (mode == MODE_PROGRESSBAR2)
        {

                //printf("[lcdd] prog2:%s,%d,%d\n",m_progressHeaderGlobal.c_str(),m_progressGlobal,m_progressShowEscape);
                // Clear Display
                display.draw_fill_rect (0,12,120,64, CLCDDisplay::PIXEL_OFF);

                // paint  global header
                int width = fonts.menu->getRenderWidth(m_progressHeaderGlobal.c_str(),true);
                if(width > 100)
                        width = 100;
                int start_pos = (120-width) /2;
                fonts.menu->RenderString(start_pos, PROG2_GLOB_POS_Y-3, width+10, m_progressHeaderGlobal.c_str(), CLCDDisplay::PIXEL_ON,0,true);

                // paint global bar
                int marker_length = (PROG2_GLOB_POS_WIDTH * m_progressGlobal)/100;

                display.draw_fill_rect (PROG2_GLOB_POS_X,                               PROG2_GLOB_POS_Y,   PROG2_GLOB_POS_X+PROG2_GLOB_POS_WIDTH,   PROG2_GLOB_POS_Y+PROG2_GLOB_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
                display.draw_fill_rect (PROG2_GLOB_POS_X+1+marker_length, PROG2_GLOB_POS_Y+1, PROG2_GLOB_POS_X+PROG2_GLOB_POS_WIDTH-1, PROG2_GLOB_POS_Y+PROG2_GLOB_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);

                // paint  local header
                width = fonts.menu->getRenderWidth(m_progressHeaderLocal.c_str(),true);
                if(width > 100)
                        width = 100;
                start_pos = (120-width) /2;
                fonts.menu->RenderString(start_pos, PROG2_LOCAL_POS_Y + PROG2_LOCAL_POS_HEIGTH +10 , width+10, m_progressHeaderLocal.c_str(), CLCDDisplay::PIXEL_ON,0,true);
                // paint local bar
                marker_length = (PROG2_LOCAL_POS_WIDTH * m_progressLocal)/100;

                display.draw_fill_rect (PROG2_LOCAL_POS_X,                              PROG2_LOCAL_POS_Y,   PROG2_LOCAL_POS_X+PROG2_LOCAL_POS_WIDTH,   PROG2_LOCAL_POS_Y+PROG2_LOCAL_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
                display.draw_fill_rect (PROG2_LOCAL_POS_X+1+marker_length,   PROG2_LOCAL_POS_Y+1, PROG2_LOCAL_POS_X+PROG2_LOCAL_POS_WIDTH-1, PROG2_LOCAL_POS_Y+PROG2_LOCAL_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
                // paint foot
                if(m_progressShowEscape  == true)
                {
                        fonts.menu->RenderString(90, 64, 40, "Home", CLCDDisplay::PIXEL_ON);
                }
                displayUpdate();
        }
#endif
}

#endif // ENABLE_LCD



