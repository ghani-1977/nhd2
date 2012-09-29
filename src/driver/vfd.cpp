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
// xtrend 5XXX has no vfd
#if defined (PLATFORM_XTREND)
	has_lcd = 0;
#else
	has_lcd = 1;
#endif
	
	fd = -1;
	
	text[0] = 0;
	clearClock = 0;

	vfd_scrollText = 0;
}

CVFD::~CVFD()
{ 
	
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
			//if (atoi(g_settings.lcd_setting_dim_brightness) > 0) 
			if (g_settings.lcd_setting_dim_brightness > 0) 
			{
				// save lcd brightness, setBrightness() changes global setting
				int b = g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS];
				//setBrightness(atoi(g_settings.lcd_setting_dim_brightness));
				setBrightness(g_settings.lcd_setting_dim_brightness);
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
	if(!has_lcd) 
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
	if(!has_lcd) 
		return;

	last_toggle_state_power = g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER];
	
	setlcdparameter( (mode == MODE_STANDBY) ? g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] : g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS], last_toggle_state_power);
}

void CVFD::showServicename(const std::string & name) // UTF-8
{
	if(!has_lcd) 
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
	if(!has_lcd)
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

void CVFD::showMenuText(const int position, const char * text, const int highlight, const bool utf_encoded)
{
	if(!has_lcd) 
		return;

	if (mode != MODE_MENU_UTF8)
		return;

	ShowText((char *) text);
	wake_up();
}

void CVFD::showAudioTrack(const std::string & artist, const std::string & title, const std::string & album)
{
	if(!has_lcd) 
		return;

	if (mode != MODE_AUDIO) 
		return;

	dprintf(DEBUG_DEBUG, "CVFD::showAudioTrack: %s\n", title.c_str());
	
	ShowText((char *) title.c_str());
	wake_up();
}

void CVFD::showAudioPlayMode(AUDIOMODES m)
{
	if(!has_lcd) 
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

void CVFD::setMode(const MODES m, const char * const title)
{
	if(!has_lcd) 
		return;

	//if(mode == MODE_AUDIO)
	//	ShowIcon(VFD_ICON_MP3, false);

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
	}

	wake_up();
}

void CVFD::setBrightness(int bright)
{
	if(!has_lcd) 
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
	if(!has_lcd) 
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
	if(!has_lcd) 
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
	if(!has_lcd)
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
	if(!has_lcd) 
		return;

	last_toggle_state_power = 1 - last_toggle_state_power;
	setlcdparameter((mode == MODE_STANDBY) ? g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] : g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS], last_toggle_state_power);
}

void CVFD::setMuted(bool mu)
{
	if(!has_lcd) 
		return;
	
	muted = mu;	
}

void CVFD::resume()
{
	if(!has_lcd) 
		return;
}

void CVFD::pause()
{
	if(!has_lcd) 
		return;
}

void CVFD::Lock()
{
	if(!has_lcd) 
		return;

	creat("/tmp/vfd.locked", 0);
}

void CVFD::Unlock()
{
	if(!has_lcd) 
		return;

	unlink("/tmp/vfd.locked");
}

void CVFD::Clear()
{
	if(!has_lcd) 
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

