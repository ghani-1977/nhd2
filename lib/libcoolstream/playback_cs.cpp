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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <errno.h>

#include <sys/stat.h>

#include "dmx_cs.h"
#include "audio_cs.h"
#include "video_cs.h"

#include "playback_cs.h"

#include <driver/framebuffer.h>
#include <system/debug.h>


static const char * FILENAME = "[playback_cs.cpp]";


/*
1- Open()
2- Start()
3- SetPosition()
4- setSpeed()
*/

cPlayback::cPlayback(int num)
{ 
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
#if defined (ENABLE_GSTREAMER)
	m_gst_playbin = NULL;
#else
	player = NULL;
#endif	
	
	mAudioStream = 0;
	mSpeed = 0;

	playing = false;
	playstate = STATE_STOP;
}

cPlayback::~cPlayback()
{  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
}

//Used by Fileplay
bool cPlayback::Open()
{
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__ );
	
#if !defined (ENABLE_GSTREAMER)
	player = (Context_t*)malloc(sizeof(Context_t));

	//init player
	if(player) 
	{
		player->playback	= &PlaybackHandler;
		player->output		= &OutputHandler;
		player->container	= &ContainerHandler;
		player->manager		= &ManagerHandler;
	}

	// registration of output devices
	if(player && player->output) 
	{
		player->output->Command(player,OUTPUT_ADD, (void*)"audio");
		player->output->Command(player,OUTPUT_ADD, (void*)"video");
//#if defined (ENABLE_LIBASS)		
		player->output->Command(player,OUTPUT_ADD, (void*)"subtitle");
//#endif		
	}

	// subtitle
//#if defined (ENABLE_LIBASS)
	SubtitleOutputDef_t out;

	out.screen_width = CFrameBuffer::getInstance()->getScreenWidth();
	out.screen_height = CFrameBuffer::getInstance()->getScreenHeight();
	out.framebufferFD = CFrameBuffer::getInstance()->getFileHandle();
	out.destination   = (unsigned char *)CFrameBuffer::getInstance()->getFrameBufferPointer();
	out.destStride    = CFrameBuffer::getInstance()->getStride();;
	out.shareFramebuffer = 1;
    
	player->output->subtitle->Command(player, (OutputCmd_t)OUTPUT_SET_SUBTITLE_OUTPUT, (void*) &out);
//#endif // libass
#endif

	return true;
}

//Used by Fileplay
void cPlayback::Close(void)
{  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);

	Stop();	
}

// start
bool cPlayback::Start(char * filename, unsigned short vpid, int vtype, unsigned short apid, bool ac3)
{
	printf("%s:%s - filename=%s\n", FILENAME, __FUNCTION__, filename);

	mAudioStream = 0;

#if defined (ENABLE_GSTREAMER)
	int argc = 1;
	int flags = 0x47; //(GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO | GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_TEXT);
	gchar *uri;
		
	gst_init(&argc, &filename);
	uri = g_filename_to_uri(filename, NULL, NULL);
	m_gst_playbin = gst_element_factory_make("playbin2", "playbin");
	g_object_set(G_OBJECT (m_gst_playbin), "uri", uri, NULL);
	g_object_set(G_OBJECT (m_gst_playbin), "flags", flags, NULL);
	g_free(uri);
#else
	//create playback path
	char file[400] = {""};
	bool isHTTP = false;

	if(!strncmp("http://", filename, 7))
	{
            isHTTP = true;
	}
	else if(!strncmp("vlc://", filename, 6))
	{
            isHTTP = true;
	}
	else if(!strncmp("file://", filename, 7))
	{
	 
	}
	else if(!strncmp("upnp://", filename, 7))
	{
            isHTTP = true;
	}
	else
	    strcat(file, "file://");
	
	strcat(file, filename);

	//open file
	if(player && player->playback && player->playback->Command(player, PLAYBACK_OPEN, file) >= 0) 
	{
		/* play it baby  */
		if(player && player->output && player->playback) 
		{
        		player->output->Command(player, OUTPUT_OPEN, NULL);
			
			if (player->playback->Command(player, PLAYBACK_PLAY, NULL) == 0 ) // playback.c uses "int = 0" for "true"
			{
				playing = true;
				playstate = STATE_PLAY;
			}
		}		
	}
#endif

	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	return true;
}

bool cPlayback::Play(void)
{
	dprintf(DEBUG_INFO, "%s:%s playing %d\n", FILENAME, __FUNCTION__, playing);	

	if(playing == true) 
		return true;
	
#if !defined (ENABLE_GSTREAMER)
	/* play it baby  */
	if(player && player->output && player->playback) 
	{
        	player->output->Command(player, OUTPUT_OPEN, NULL);
			
		if (player->playback->Command(player, PLAYBACK_PLAY, NULL) == 0 ) // playback.c uses "int = 0" for "true"
		{
			playing = true;
			playstate = STATE_PLAY;
		}
	}
#endif	

	return playing;
}

bool cPlayback::SyncAV(void)
{
	dprintf(DEBUG_INFO, "%s:%s playing %d\n", FILENAME, __FUNCTION__, playing);	

	if(playing == false ) 
		return false;
	
#if !defined (ENABLE_GSTREAMER)
	if(player && player->output && player->playback) 
	{
        	player->output->Command(player, OUTPUT_AVSYNC, NULL);
	}
#endif	

	return true;
}

bool cPlayback::Stop(void)
{ 
	dprintf(DEBUG_INFO, "%s:%s playing %d\n", FILENAME, __FUNCTION__, playing);	

	if(playing == false) 
		return false;	

#if defined (ENABLE_GSTREAMER)
	if(m_gst_playbin)
	{
		gst_element_set_state(m_gst_playbin, GST_STATE_NULL);
	}
#else
	if(player && player->playback && player->output) 
	{
		player->playback->Command(player, PLAYBACK_STOP, NULL);
		player->output->Command(player, OUTPUT_CLOSE, NULL);
	}

	if(player && player->output) 
	{
		player->output->Command(player,OUTPUT_DEL, (void*)"audio");
		player->output->Command(player,OUTPUT_DEL, (void*)"video");
//#if defined (ENABLE_LIBASS)		
		player->output->Command(player,OUTPUT_DEL, (void*)"subtitle");
//#endif		
	}

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_CLOSE, NULL);


	if(player != NULL)
	{
		free(player);

		player = NULL;
	}
#endif

	playing = false;
	
	dprintf(DEBUG_INFO, "%s:%s playing %d\n", FILENAME, __FUNCTION__, playing);
	
	playstate = STATE_STOP;

	return true;
}

bool cPlayback::SetAPid(unsigned short pid, bool ac3)
{
	dprintf(DEBUG_INFO, "%s:%s curpid:%d nextpid:%d\n", FILENAME, __FUNCTION__, mAudioStream, pid);

#if !defined (ENABLE_GSTREAMER)
	int track = pid;

	if(pid != mAudioStream)
	{
		if(player && player->playback)
				player->playback->Command(player, PLAYBACK_SWITCH_AUDIO, (void*)&track);

		mAudioStream = pid;
	}
#endif	

	return true;
}

bool cPlayback::SetSpeed(int speed)
{  
	dprintf(DEBUG_INFO, "%s:%s speed %d\n", FILENAME, __FUNCTION__, speed);	

	if(playing == false) 
		return false;

#if defined (ENABLE_GSTREAMER)
	if(speed == 0)
		if(m_gst_playbin)
			gst_element_set_state(m_gst_playbin, GST_STATE_PAUSED);
		
	if(speed == 1)
		if(m_gst_playbin)
			gst_element_set_state(m_gst_playbin, GST_STATE_PLAYING);
#else
	if(player && player->playback) 
	{
		if(speed > 1) 		//forwarding
		{
			//
			if (speed > 7) 
				speed = 7;

			player->playback->Command(player, PLAYBACK_FASTFORWARD, (void*)&speed);
			playstate = STATE_FF;
		}
		else if(speed == 0)	//pausing
		{
			player->playback->Command(player, PLAYBACK_PAUSE, NULL);
			playstate = STATE_PAUSE;
		}
		else if (speed < 0)	//backwarding
		{
			//
			if (speed < -7) 
				speed = -7;
			
			player->playback->Command(player, PLAYBACK_FASTBACKWARD, (void*)&speed);
			playstate = STATE_REW;
		}
		else if(speed == 1) 	//continue
		{
			player->playback->Command(player, PLAYBACK_CONTINUE, NULL);
			playstate = STATE_PLAY;
		}
	}
#endif

	mSpeed = speed;

	return true;
}

bool cPlayback::SetSlow(int slow)
{  
	dprintf(DEBUG_INFO, "%s:%s playing %d\n", FILENAME, __FUNCTION__, playing);	

	if(playing == false) 
		return false;

#if !defined (ENABLE_GSTREAMER)
	if(player && player->playback) 
	{
		player->playback->Command(player, PLAYBACK_SLOWMOTION, (void*)&slow);
		playstate = STATE_SLOW;
	}
#endif	

	mSpeed = slow;

	return true;
}

bool cPlayback::GetSpeed(int &speed) const
{
	speed = mSpeed;

	return true;
}

// in milliseconds
bool cPlayback::GetPosition(int &position, int &duration)
{
	if(playing == false) 
		return false;	

#if defined (ENABLE_GSTREAMER)
	//PTS
	unsigned long long int pts = 0;
	
	GstFormat fmt = GST_FORMAT_TIME; //Returns time in nanosecs
	
	if(m_gst_playbin)
	{
		gst_element_query_position(m_gst_playbin, &fmt, &pts);
		position = pts / 1000000000.0;
		//debug(150, "Pts = %02d:%02d:%02d (%llu.0000 sec)", (int)((sec / 60) / 60) % 60, (int)(sec / 60) % 60, (int)sec % 60, sec);
	}
	
	// length
	double length = 0;
	
	GstFormat fmt = GST_FORMAT_TIME; //Returns time in nanosecs
	gint64 len;

	if(m_gst_playbin)
	{
		gst_element_query_duration(m_gst_playbin, &fmt, &len);
		length = len / 1000000000.0;
		if(length < 0) length = 0;
		
		duration = (int)(length*1000);
		//debug(150, "Length = %02d:%02d:%02d (%.4f sec)", (int)((length / 60) / 60) % 60, (int)(length / 60) % 60, (int)length % 60, length);
	}
#else
	if (player && player->playback && !player->playback->isPlaying) 
	{	  
		dprintf(DEBUG_INFO, "cPlayback::%s !!!!EOF!!!! < -1\n", __func__);	
	
		return false;
	} 

	//PTS
	unsigned long long int vpts = 0;

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_PTS, &vpts);

	if(vpts <= 0) 
	{
		//printf("ERROR: vpts==0\n");
		//return false;
	} 
	else 
	{
		/* len is in nanoseconds. we have 90 000 pts per second. */
		position = vpts/90;
	}

	//LENGTH
	double length = 0;

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_LENGTH, &length);

	if(length <= 0) 
	{
		duration = duration+1000;
		//printf("ERROR: duration==0\n");
		//return false;
	} 
	else 
	{
		duration = (int)(length*1000);
	}
#endif	
	
	return true;
}

bool cPlayback::SetPosition(int position, bool absolute)
{
	if(playing == false) 
		return false;

#if defined (ENABLE_GSTREAMER)
	gint64 time_nanoseconds;
	gint64 pos;
	GstFormat fmt = GST_FORMAT_TIME;
		
	if(m_gst_playbin)
	{
		gst_element_query_position(m_gst_playbin, &fmt, &pos);
		time_nanoseconds = pos + (position * 1000000000);
		if(time_nanoseconds < 0) time_nanoseconds = 0;
		gst_element_seek(m_gst_playbin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time_nanoseconds, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
	}
#else
	float pos = (position/1000.0);

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_SEEK, (void*)&pos);
#endif

	return true;
}

void cPlayback::FindAllPids(uint16_t *apids, unsigned short *ac3flags, uint16_t *numpida, std::string * language)
{ 
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);

#if !defined (ENABLE_GSTREAMER)
	// audio pids
	if(player && player->manager && player->manager->audio) 
	{
		char ** TrackList = NULL;
		player->manager->audio->Command(player, MANAGER_LIST, &TrackList);

		if (TrackList != NULL) 
		{
			printf("AudioTrack List\n");
			int i = 0,j=0;

			for (i = 0, j=0; TrackList[i] != NULL; i+=2,j++) 
			{
				printf("\t%s - %s\n", TrackList[i], TrackList[i+1]);
				apids[j]=j;

				// atUnknown, atMPEG, atMP3, atAC3, atDTS, atAAC, atPCM, atOGG, atFLAC

				if( (!strncmp("A_MPEG/L3",   TrackList[i+1], 9)) || (!strncmp("A_MP3",   TrackList[i+1], 5)) )
					ac3flags[j] = 4;
				else if(!strncmp("A_AC3",       TrackList[i+1], 5))
					ac3flags[j] = 1;
				else if(!strncmp("A_DTS",       TrackList[i+1], 5))
					ac3flags[j] = 6;
				else if(!strncmp("A_AAC",       TrackList[i+1], 5))
					ac3flags[j] = 5;
				else if(!strncmp("A_PCM",       TrackList[i+1], 5))
					ac3flags[j] = 0; 	//todo
				else if(!strncmp("A_VORBIS",    TrackList[i+1], 8))
					ac3flags[j] = 0;	//todo
				else if(!strncmp("A_FLAC",      TrackList[i+1], 6))
					ac3flags[j] = 0;	//todo
				else
					ac3flags[j] = 0;	//todo

				language[j]=TrackList[i];
				
				free(TrackList[i]);
				free(TrackList[i+1]);
			}
			free(TrackList);
			*numpida=j;
		}
	}
#endif	
}

void cPlayback::getMeta()
{
	if(playing) 
		return;
	
#if !defined (ENABLE_GSTREAMER)
	char *tags[] =
        {
                "Title",
                "Artist",
                "Album",
                "Year",
                "Genre",
                "Comment",
                "Track",
                "Copyright",
                "TestLibEplayer",
                NULL
        };
		
        int i = 0;
        while (tags[i] != NULL)
        {
		char* tag = tags[i];
                player->playback->Command(player, PLAYBACK_INFO, &tag);

                if (tag != NULL)
                    	printf("\t%s:\t%s\n",tags[i], tag);
                else
                    	printf("\t%s:\tNULL\n",tags[i]);
                i++;
       }
#endif       
}

