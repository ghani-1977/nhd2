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

#ifndef __settings__
#define __settings__

#include <system/localize.h>
#include <configfile.h>
#include <zapit/client/zapitclient.h>

#include <string>

#include <gui/widget/menue.h>
#include <gui/widget/icons.h>



struct SNeutrinoSettings
{
	// VIDEO
	int video_Mode;
	int analog_mode;	//scart

	int hdmi_color_space;

	int video_Ratio;
	int video_Format;

	int wss_mode;
	
	unsigned char contrast;
	unsigned char saturation;
	unsigned char brightness;
	unsigned char tint;
	// END VIDEO

	// AUDIO
	int audio_AnalogMode;
	int audio_DolbyDigital;

	int hdmi_dd;

	int avsync;
	
	int ac3_delay;
	int pcm_delay;
	
	// subs
	int auto_lang;
	int auto_subs;
	char pref_lang[3][30];
	char pref_subs[3][30];
	// END AUDIO

	// PARENTALLOCK
	int parentallock_prompt;
	int parentallock_lockage;
	char parentallock_pincode[5];
	// END PARENTALLOCK

	// NETWORK
#define NETWORK_NFS_NR_OF_ENTRIES 8
	std::string network_nfs_ip[NETWORK_NFS_NR_OF_ENTRIES];
	char network_nfs_mac[NETWORK_NFS_NR_OF_ENTRIES][31];
	char network_nfs_local_dir[NETWORK_NFS_NR_OF_ENTRIES][100];
	char network_nfs_dir[NETWORK_NFS_NR_OF_ENTRIES][100];
	int  network_nfs_automount[NETWORK_NFS_NR_OF_ENTRIES];
	char network_nfs_mount_options1[NETWORK_NFS_NR_OF_ENTRIES][31];
	char network_nfs_mount_options2[NETWORK_NFS_NR_OF_ENTRIES][31];
	int  network_nfs_type[NETWORK_NFS_NR_OF_ENTRIES];
	char network_nfs_username[NETWORK_NFS_NR_OF_ENTRIES][31];
	char network_nfs_password[NETWORK_NFS_NR_OF_ENTRIES][31];

	std::string network_ntpserver;
	std::string network_ntprefresh;
	int network_ntpenable;
	// END NETWORK

	// RECORDING
	char record_safety_time_before[3];
	char record_safety_time_after[3];

	int  recording_type;

	char network_nfs_recordingdir[100];

	int auto_timeshift;
	int temp_timeshift;
	int auto_delete;

	int record_hours;

	unsigned char recording_audio_pids_default;
	int recording_audio_pids_std;
	int recording_audio_pids_alt;
	int recording_audio_pids_ac3;

	int recording_epg_for_filename;
	int recording_save_in_channeldir;
	
	int  recording_zap_on_announce;
	// END RECORDING

	// MOVIEPLAYER
	char network_nfs_moviedir[100];
	
	std::string streaming_server_ip;
	// END MOVIEPLAYER

	// OSD
	unsigned char gtx_alpha;
	
	char language[25];

	unsigned char menu_Head_alpha;
	unsigned char menu_Head_red;
	unsigned char menu_Head_green;
	unsigned char menu_Head_blue;

	unsigned char menu_Head_Text_alpha;
	unsigned char menu_Head_Text_red;
	unsigned char menu_Head_Text_green;
	unsigned char menu_Head_Text_blue;

	unsigned char menu_Content_alpha;
	unsigned char menu_Content_red;
	unsigned char menu_Content_green;
	unsigned char menu_Content_blue;

	unsigned char menu_Content_Text_alpha;
	unsigned char menu_Content_Text_red;
	unsigned char menu_Content_Text_green;
	unsigned char menu_Content_Text_blue;

	unsigned char menu_Content_Selected_alpha;
	unsigned char menu_Content_Selected_red;
	unsigned char menu_Content_Selected_green;
	unsigned char menu_Content_Selected_blue;

	unsigned char menu_Content_Selected_Text_alpha;
	unsigned char menu_Content_Selected_Text_red;
	unsigned char menu_Content_Selected_Text_green;
	unsigned char menu_Content_Selected_Text_blue;

	unsigned char menu_Content_inactive_alpha;
	unsigned char menu_Content_inactive_red;
	unsigned char menu_Content_inactive_green;
	unsigned char menu_Content_inactive_blue;

	unsigned char menu_Content_inactive_Text_alpha;
	unsigned char menu_Content_inactive_Text_red;
	unsigned char menu_Content_inactive_Text_green;
	unsigned char menu_Content_inactive_Text_blue;

	unsigned char infobar_alpha;
	unsigned char infobar_red;
	unsigned char infobar_green;
	unsigned char infobar_blue;

	unsigned char infobar_Text_alpha;
	unsigned char infobar_Text_red;
	unsigned char infobar_Text_green;
	unsigned char infobar_Text_blue;

	char	font_file[100];

#define TIMING_SETTING_COUNT 6
	enum TIMING_SETTINGS {
		TIMING_MENU        = 0,
		TIMING_CHANLIST    = 1,
		TIMING_EPG         = 2,
		TIMING_INFOBAR     = 3,
		TIMING_FILEBROWSER = 4,
		TIMING_NUMERICZAP  = 5
	};

	int  timing       [TIMING_SETTING_COUNT]   ;
	char timing_string[TIMING_SETTING_COUNT][4];

#define FONT_TYPE_COUNT 22
	enum FONT_TYPES {
		FONT_TYPE_MENU                =  0,
		FONT_TYPE_MENU_TITLE          =  1,
		FONT_TYPE_MENU_INFO           =  2,
		FONT_TYPE_EPG_TITLE           =  3,
		FONT_TYPE_EPG_INFO1           =  4,
		FONT_TYPE_EPG_INFO2           =  5,
		FONT_TYPE_EPG_DATE            =  6,
		FONT_TYPE_EVENTLIST_TITLE     =  7,
		FONT_TYPE_EVENTLIST_ITEMLARGE =  8,
		FONT_TYPE_EVENTLIST_ITEMSMALL =  9,
		FONT_TYPE_EVENTLIST_DATETIME  = 10,
		FONT_TYPE_GAMELIST_ITEMLARGE  = 11,
		FONT_TYPE_GAMELIST_ITEMSMALL  = 12,
		FONT_TYPE_CHANNELLIST         = 13,
		FONT_TYPE_CHANNELLIST_DESCR   = 14,
		FONT_TYPE_CHANNELLIST_NUMBER  = 15,
		FONT_TYPE_CHANNEL_NUM_ZAP     = 16,
		FONT_TYPE_INFOBAR_NUMBER      = 17,
		FONT_TYPE_INFOBAR_CHANNAME    = 18,
		FONT_TYPE_INFOBAR_INFO        = 19,
		FONT_TYPE_INFOBAR_SMALL       = 20,
		FONT_TYPE_FILEBROWSER_ITEM    = 21
	};

	int screen_StartX;
	int screen_StartY;
	int screen_EndX;
	int screen_EndY;
	int screen_width;
	int screen_height;
	
	int volume_pos;
	int help_bar;
	int help_txt;
	int menutitle_vfd;
	// END OSD

	// KEYS
	char repeat_blocker[4];
	char repeat_genericblocker[4];

	int key_tvradio_mode;

	int key_channelList_pageup;
	int key_channelList_pagedown;
	int key_channelList_cancel;
	int key_channelList_reload;
	int key_channelList_addrecord;
	int key_channelList_addremind;

	int key_quickzap_up;
	int key_quickzap_down;

	int key_bouquet_up;
	int key_bouquet_down;

	int key_subchannel_up;
	int key_subchannel_down;

	int key_zaphistory;
	int key_lastchannel;
	int key_list_start;
	int key_list_end;
	
	//pip
	int key_pip;
	int key_pip_subchannel;

	int menu_left_exit;

	int mpkey_rewind;
	int mpkey_forward;
	int mpkey_pause;
	int mpkey_stop;
	int mpkey_play;
	int mpkey_audio;
	int mpkey_time;
	int mpkey_bookmark;

	int key_timeshift;
	
	int key_unlock;
	// END KEYBINDING

	// USERMENU
        typedef enum
        {
                BUTTON_RED = 0,  // Do not change ordering of members, add new item just before BUTTON_MAX!!!
                BUTTON_GREEN = 1,
                BUTTON_YELLOW = 2,
                BUTTON_BLUE = 3,
#if defined (PLATFORM_GIGABLUE)  
		BUTTON_F1 = 4,
		BUTTON_F2 = 5,
		BUTTON_F3 = 6,
		BUTTON_F4 = 7,
#endif
                BUTTON_MAX   // MUST be always the last in the list
        }USER_BUTTON;

        typedef enum
        {
                ITEM_NONE = 0, // Do not change ordering of members, add new item just before ITEM_MAX!!!
                ITEM_BAR = 1,
                ITEM_EPG_LIST = 2,
                ITEM_EPG_SUPER = 3,
                ITEM_EPG_INFO = 4,
                ITEM_EPG_MISC = 5,
                ITEM_AUDIO_SELECT = 6,
                ITEM_SUBCHANNEL = 7,
                ITEM_RECORD = 8,
                ITEM_MOVIEPLAYER_MB = 9,
                ITEM_TIMERLIST = 10,
                ITEM_REMOTE = 11,
                ITEM_FAVORITS = 12,
                ITEM_TECHINFO = 13,
                ITEM_PLUGIN = 14,
                ITEM_VTXT = 15,
                ITEM_GAME = 16,
#if ENABLE_GRAPHLCD                
                ITEM_GLCD = 17,
#endif                
                ITEM_MAX   // MUST be always the last in the list
        }USER_ITEM;
	
        std::string usermenu_text[BUTTON_MAX];
        int usermenu[BUTTON_MAX][ITEM_MAX];  // (USER_ITEM)  [button][position in Menue] = feature item
	// END KEYS

	// AUDIOPLAYER
	char network_nfs_audioplayerdir[100];

	int   audioplayer_display;
	int   audioplayer_follow;
	char  audioplayer_screensaver[3];
	int   audioplayer_highprio;
	int   audioplayer_select_title_by_name;
	int   audioplayer_repeat_on;
	int   audioplayer_show_playlist;
	int   audioplayer_enable_sc_metadata;
	
	//shoutcast(without gui setup)
	std::string shoutcast_dev_id;
	// END AUDIOPLAYER

	// PICVIEWER
	char network_nfs_picturedir[100];
	char   picviewer_slide_time[3];
	int    picviewer_scaling;
	// END PICVIEWER

	// MISC OPTS
	int shutdown_real;
	int shutdown_real_rcdelay;
        char shutdown_count[4];
	int power_standby;
	int infobar_sat_display;
	int infobar_subchan_disp_pos;
	int rotor_swap;
	int mb_preview;
	char timezone[150];

	int radiotext_enable;
	
	std::string logos_dir;

	// channellist
	int channellist_epgtext_align_right;
	int channellist_extended;
	int zap_cycle;
	int sms_channel;
	int virtual_zap_mode;
	int channellist_ca;
	int make_hd_list;

	// epg
	int epg_save;
	std::string epg_cache;
	std::string epg_old_events;
	std::string epg_max_events;
	std::string epg_extendedcache;
	std::string epg_dir;

	//filebrowser
	int filesystem_is_utf8;
	int filebrowser_showrights;
	int filebrowser_sortmethod;
	int filebrowser_denydirectoryleave;

	//zapit setup
	int lastChannelMode;
	std::string StartChannelTV;
	std::string StartChannelRadio;
	t_channel_id startchanneltv_id;
	t_channel_id startchannelradio_id;
	int startchanneltv_nr;
	int startchannelradio_nr;
	int uselastchannel;
	
	// inter without GUI setup
	char current_volume;
	int channel_mode;
	// MISC OPTS

	// HDD
	int	hdd_sleep;
	int	hdd_noise;
	// END HDD

	// UPDATE
	char	update_dir[100];
	int softupdate_mode;
	char softupdate_url_file[31];
	char softupdate_proxyserver[31];
	char softupdate_proxyusername[31];
	char softupdate_proxypassword[31];
	// END UPDATE

	// VFD
#define LCD_SETTING_COUNT 8
	enum LCD_SETTINGS {
		LCD_BRIGHTNESS         = 0,
		LCD_STANDBY_BRIGHTNESS = 1,
		LCD_CONTRAST           = 2,
		LCD_POWER              = 3,
		LCD_INVERSE            = 4,
		LCD_SHOW_VOLUME        = 5,
		LCD_AUTODIMM           = 6,
		LCD_SCROLL_TEXT		= 7
	};

	int lcd_setting[LCD_SETTING_COUNT];
	char lcd_setting_dim_time[4];
	char lcd_setting_dim_brightness[4];
	// END VFD
	
#if ENABLE_GRAPHLCD
        int		glcd_enable;
        uint32_t	glcd_color_fg;
        uint32_t	glcd_color_bg;
        uint32_t	glcd_color_bar;
        std::string	glcd_font;
        int		glcd_percent_channel;
        int		glcd_percent_epg;
        int		glcd_percent_bar;
        int		glcd_percent_time;
        int		glcd_mirror_osd;
        int		glcd_time_in_standby;
#endif	

#define FILESYSTEM_ENCODING_TO_UTF8(a) (g_settings.filesystem_is_utf8 ? (a) : ZapitTools::Latin1_to_UTF8(a).c_str())
#define UTF8_TO_FILESYSTEM_ENCODING(a) (g_settings.filesystem_is_utf8 ? (a) : ZapitTools::UTF8_to_Latin1(a).c_str())
#define FILESYSTEM_ENCODING_TO_UTF8_STRING(a) (g_settings.filesystem_is_utf8 ? (a) : Latin1_to_UTF8(a))	
};

/* some default Values */
extern const int               default_timing     [TIMING_SETTING_COUNT];
extern const neutrino_locale_t timing_setting_name[TIMING_SETTING_COUNT];

// lcdd
#define DEFAULT_LCD_BRIGHTNESS			0x07
#define DEFAULT_LCD_STANDBYBRIGHTNESS		0x07
#define DEFAULT_LCD_CONTRAST			0x0F
#define DEFAULT_LCD_POWER			0x01
#define DEFAULT_LCD_INVERSE			0x00
#define DEFAULT_LCD_AUTODIMM			0x00
#define DEFAULT_LCD_SHOW_VOLUME			0x01
#define DEFAULT_LCD_SCROLL_TEXT			0x00

// corners (osd)
#define RADIUS_LARGE		16
#define RADIUS_MID      	8
#define RADIUS_SMALL    	4

#define BORDER_LEFT		10
#define BORDER_RIGHT		10
#define SCROLLBAR_WIDTH		15
/* end default values */

// recording
const int RECORDING_OFF    = 0;
const int RECORDING_SERVER = 1;
const int RECORDING_VCR    = 2;
const int RECORDING_FILE   = 3;

// parentallock
const int PARENTALLOCK_PROMPT_NEVER          = 0;
const int PARENTALLOCK_PROMPT_ONSTART        = 1;
const int PARENTALLOCK_PROMPT_CHANGETOLOCKED = 2;
const int PARENTALLOCK_PROMPT_ONSIGNAL       = 3;


#endif
