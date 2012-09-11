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

#include <pthread.h>
#include <syscall.h>

#include "dmx_cs.h"
#include "audio_cs.h"
#include "video_cs.h"

#include "playback_cs.h"

#include <driver/framebuffer.h>
#include <system/debug.h>



static const char * FILENAME = "[playback_cs.cpp]";


#if defined (ENABLE_GSTREAMER)
GstElement * m_gst_playbin = NULL;
GstBus * bus = NULL;;
GMainLoop * loop = NULL;
GstElement * audioSink = NULL;
GstElement * videoSink = NULL;
gchar * uri = NULL;
guint watchId = false;
#endif

#if defined ENABLE_GSTREAMER
gint match_sinktype(GstElement *element, gpointer type)
{
	return strcmp(g_type_name(G_OBJECT_TYPE(element)), (const char*)type);
}

gboolean Gst_bus_call(GstBus *bus, GstMessage *msg, gpointer user_data)
{
	//gstplaydata_t * playdata = (gstplaydata_t *) user_data;
	cPlayback * self=(cPlayback*) user_data; 

	switch (GST_MESSAGE_TYPE(msg)) 
	{
		case GST_MESSAGE_EOS: {
			g_message("End-of-stream");
			//g_main_loop_quit(loop);
			self->Close();
			break;
		}
		
		case GST_MESSAGE_ERROR: 
		{
			GError *err;
			gst_message_parse_error(msg, &err, NULL);
			g_error("%s", err->message);
			g_error_free(err);

			//g_main_loop_quit(loop);
			self->Close();
			break;
		}
		
		case GST_MESSAGE_STATE_CHANGED:
		{
			if(GST_MESSAGE_SRC(msg) != GST_OBJECT(m_gst_playbin))
				break;

			GstState old_state, new_state;
			gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
			
			if(old_state == new_state)
				break;
		
			printf("cPlayback::state transition %s -> %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
		
			GstStateChange transition = (GstStateChange)GST_STATE_TRANSITION(old_state, new_state);
		
			switch(transition)
			{
				case GST_STATE_CHANGE_NULL_TO_READY:
				{
				}	break;
				case GST_STATE_CHANGE_READY_TO_PAUSED:
				{
					GstIterator *children;
					if (audioSink)
					{
						gst_object_unref(GST_OBJECT(audioSink));
						audioSink = NULL;
					}
					
					if (videoSink)
					{
						gst_object_unref(GST_OBJECT(videoSink));
						videoSink = NULL;
					}
					children = gst_bin_iterate_recurse(GST_BIN(m_gst_playbin));
					audioSink = GST_ELEMENT_CAST(gst_iterator_find_custom(children, (GCompareFunc)match_sinktype, (gpointer)"GstDVBAudioSink"));
					videoSink = GST_ELEMENT_CAST(gst_iterator_find_custom(children, (GCompareFunc)match_sinktype, (gpointer)"GstDVBVideoSink"));
					gst_iterator_free(children);
				}	break;
				case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
				{
				}	break;
				case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
				{
				}	break;
				case GST_STATE_CHANGE_PAUSED_TO_READY:
				{
					if (audioSink)
					{
						gst_object_unref(GST_OBJECT(audioSink));
						audioSink = NULL;
					}
					if (videoSink)
					{
						gst_object_unref(GST_OBJECT(videoSink));
						videoSink = NULL;
					}
				}	break;
				case GST_STATE_CHANGE_READY_TO_NULL:
				{
				}	break;
			}
			break;
		}
		default:
			break;
	}

	return true;
}
#endif

cPlayback::cPlayback(int num)
{ 
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
#if defined (ENABLE_GSTREAMER)
	//m_gst_playbin = NULL;
	//bus = NULL;
	//uri = NULL;
	//audioSink = NULL;
	//videoSink = NULL;
	
	//loop = NULL;
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
#if defined (ENABLE_LIBASS)		
		player->output->Command(player,OUTPUT_ADD, (void*)"subtitle");
#endif		
	}

	// subtitle
#if defined (ENABLE_LIBASS)
	SubtitleOutputDef_t out;

	out.screen_width = CFrameBuffer::getInstance()->getScreenWidth();
	out.screen_height = CFrameBuffer::getInstance()->getScreenHeight();
	out.framebufferFD = CFrameBuffer::getInstance()->getFileHandle();
	out.destination   = (unsigned char *)CFrameBuffer::getInstance()->getFrameBufferPointer();
	out.destStride    = CFrameBuffer::getInstance()->getStride();;
	out.shareFramebuffer = 1;
    
	player->output->subtitle->Command(player, (OutputCmd_t)OUTPUT_SET_SUBTITLE_OUTPUT, (void*) &out);
#endif // libass
#endif

	return true;
}

// used by movieplay
void cPlayback::Close(void)
{  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);

	Stop();
	
#if 0
	if (m_gst_playbin)
	{
		// disconnect sync handler callback
		gst_bus_set_sync_handler(bus, NULL, NULL);
		
		// unref bus
		gst_object_unref(bus);
		bus = NULL;
	
		// unref loop
		//gst_object_unref(GST_OBJECT(loop));
		//loop = NULL;

		if (audioSink)
		{
			gst_object_unref(GST_OBJECT(audioSink));
			audioSink = NULL;
		}
		if (videoSink)
		{
			gst_object_unref(GST_OBJECT(videoSink));
			videoSink = NULL;
		}
	
		// unref m_gst_playbin
		gst_object_unref (GST_OBJECT (m_gst_playbin));
		printf("destruct!\n");
		
		m_gst_playbin = NULL;
	}
#endif	
}

// start
bool cPlayback::Start(char * filename, unsigned short vpid, int vtype, unsigned short apid, bool ac3)
{
	printf("%s:%s - filename=%s\n", FILENAME, __FUNCTION__, filename);

	mAudioStream = 0;
	
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

#if defined (ENABLE_GSTREAMER)
	int flags = 0x47; //(GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO | GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_TEXT);
	
	uri = g_filename_to_uri(filename, NULL, NULL);
	
	// init gstreamer
	int argc = 1;
	char** gstargv = &uri;
	gst_init(&argc, &gstargv);
	
	//loop = g_main_loop_new(NULL, FALSE);
	
	m_gst_playbin = gst_element_factory_make("playbin2", "playbin");
	
	g_object_set(G_OBJECT (m_gst_playbin), "uri", uri, NULL);
	g_object_set(G_OBJECT (m_gst_playbin), "flags", flags, NULL);	
	g_free(uri);

	if(m_gst_playbin)
	{
		//gstbus handler
		bus = gst_pipeline_get_bus( GST_PIPELINE(m_gst_playbin) );
		//gst_bus_set_sync_handler(bus , Gst_bus_call, NULL);
		guint watchId = gst_bus_add_watch(bus, Gst_bus_call, this);
		gst_object_unref(bus); 
			
		// main loop
		//g_main_loop_run(loop);
		
		//videosink
		//audiosink
		//subsink
		//streaming
		//buffering
		
		// state playing
		gst_element_set_state(GST_ELEMENT(m_gst_playbin), GST_STATE_PLAYING);
		
		playing = true;
		playstate = STATE_PLAY;
	}
	else
	{
		if (m_gst_playbin)
			gst_object_unref(GST_OBJECT(m_gst_playbin));
		
		printf("failed to create GStreamer pipeline!, sorry we can not play\n");
		playing = false;
		
		return false;
	}
	
	//g_free(uri);
	
	// set buffer size
	//int m_buffer_size = 5*1024*1024;
	//g_object_set(G_OBJECT (m_gst_playbin), "buffer-size", m_buffer_size, NULL);
#else
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
#else
	if(m_gst_playbin)
	{
		gst_element_set_state(GST_ELEMENT(m_gst_playbin), GST_STATE_PLAYING);
		
		playing = true;
		playstate = STATE_PLAY;
	}
#endif	

	return playing;
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
	
	// close gst
	if (m_gst_playbin)
	{
		// disconnect sync handler callback
		//gst_bus_set_sync_handler(bus, NULL, NULL);
		//gst_bus_set_sync_handler(gst_pipeline_get_bus(GST_PIPELINE (m_gst_playbin)), NULL, NULL);
		
		g_source_remove(watchId); 
		
		// unref bus
		//gst_object_unref(bus);
		bus = NULL;
		printf("GST bus handler closed\n");
	
		// unref loop
		//gst_object_unref(GST_OBJECT(loop));
		//loop = NULL;

		if (audioSink)
		{
			gst_object_unref(GST_OBJECT(audioSink));
			audioSink = NULL;
			
			printf("GST audio Sink closed\n");
		}
		
		if (videoSink)
		{
			gst_object_unref(GST_OBJECT(videoSink));
			videoSink = NULL;
			
			printf("GST video Sink closed\n");
		}
	
		// unref m_gst_playbin
		gst_object_unref (GST_OBJECT (m_gst_playbin));
		printf("GST playbin closed\n");
		
		m_gst_playbin = NULL;
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
#if defined (ENABLE_LIBASS)		
		player->output->Command(player,OUTPUT_DEL, (void*)"subtitle");
#endif		
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
#if 0
#if defined (ENABLE_GSTREAMER)

	//EOF
	
	//position
	//unsigned long long int pts = 0;
	gint64 pts = 0;
	//unsigned long long int pts = 0;
	unsigned long long int sec = 0;
	
	GstFormat fmt = GST_FORMAT_TIME; //Returns time in nanosecs
	
	if(m_gst_playbin)
	{
		gst_element_query_position(m_gst_playbin, &fmt, &pts);
		position = pts /  90.0/*1000000000.0*/;
		//printf("Pts = %02d:%02d:%02d (%llu.0000 sec)\n", (int)((sec / 60) / 60) % 60, (int)(sec / 60) % 60, (int)sec % 60, sec);
	
		// duration
		double length = 0;
		gint64 len;

		gst_element_query_duration(m_gst_playbin, &fmt, &len);
		length = len /*/  1000000000.0*/;
		if(length < 0) length = 0;
		
		duration = (int)(length*1000);
		//printf("Length = %02d:%02d:%02d (%.4f sec)\n", (int)((length / 60) / 60) % 60, (int)(length / 60) % 60, (int)length % 60, length);
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
#endif
	
	return true;
}

bool cPlayback::SetPosition(int position, bool absolute)
{
	if(playing == false) 
		return false;

#if 0	
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
#else
	#if 1
	if(m_gst_playbin)
	{
		GstStructure * structure = NULL;
		
		//if (!structure)
			//return atUnknown;
		//	ac3flags[0] = 0;

		if ( gst_structure_has_name (structure, "audio/mpeg"))
		{
			gint mpegversion, layer = -1;
			if (!gst_structure_get_int (structure, "mpegversion", &mpegversion))
				//return atUnknown;
				ac3flags[0] = 0;

			switch (mpegversion) 
			{
				case 1:
					/*
					{
						gst_structure_get_int (structure, "layer", &layer);
						if ( layer == 3 )
							return atMP3;
						else
							return atMPEG;
							ac3flags[0] = 4;
						break;
					}
					*/
					ac3flags[0] = 4;
				case 2:
					//return atAAC;
					ac3flags[0] = 5;
				case 4:
					//return atAAC;
					ac3flags[0] = 5;
				default:
					//return atUnknown;
					ac3flags[0] = 0;
			}
		}
		else if ( gst_structure_has_name (structure, "audio/x-ac3") || gst_structure_has_name (structure, "audio/ac3") )
			//return atAC3;
			ac3flags[0] = 1;
		else if ( gst_structure_has_name (structure, "audio/x-dts") || gst_structure_has_name (structure, "audio/dts") )
			//return atDTS;
			ac3flags[0] = 6;
		else if ( gst_structure_has_name (structure, "audio/x-raw-int") )
			//return atPCM;
			ac3flags[0] = 0;

		//return atUnknown;
	}
	#endif
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



