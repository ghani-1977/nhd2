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

#if ENABLE_GSTREAMER
#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>


GstElement * m_gst_playbin = NULL;
GstElement * audioSink = NULL;
GstElement * videoSink = NULL;
gchar * uri = NULL;
GstTagList * m_stream_tags = 0;
static int end_eof = 0;
#endif

#if defined ENABLE_GSTREAMER
gint match_sinktype(GstElement *element, gpointer type)
{
	return strcmp(g_type_name(G_OBJECT_TYPE(element)), (const char*)type);
}

GstBusSyncReply Gst_bus_call(GstBus * bus, GstMessage *msg, gpointer user_data)
{
	//gstplaydata_t * playdata = (gstplaydata_t *) user_data;
	cPlayback * self=(cPlayback*) user_data; 

	switch (GST_MESSAGE_TYPE(msg)) 
	{
		case GST_MESSAGE_EOS: {
			g_message("End-of-stream");
			end_eof = 1;
			//self->Stop(); //FIXME: dont call Close hier GUI player call Close after EOF.
			break;
		}
		
		case GST_MESSAGE_ERROR: 
		{
			GError *err;
			gst_message_parse_error(msg, &err, NULL);
			g_error("%s", err->message);
			g_error_free(err);

			//g_main_loop_quit(loop);
			end_eof = 1; 		// NOTE: just to exit
			//self->Close(); 	//FIXME: dont call Close hier GUI player call Close after EOF.
			break;
		}
		
		case GST_MESSAGE_INFO:
		{
			gchar *debug;
			GError *inf;
	
			gst_message_parse_info (msg, &inf, &debug);
			g_free (debug);
			g_error_free(inf);
			break;
		}
		
		case GST_MESSAGE_TAG:
		{
			GstTagList *tags, *result;
			gst_message_parse_tag(msg, &tags);
	
			result = gst_tag_list_merge(m_stream_tags, tags, GST_TAG_MERGE_REPLACE);
			if (result)
			{
				if (m_stream_tags)
					gst_tag_list_free(m_stream_tags);
				m_stream_tags = result;
			}
	
			const GValue *gv_image = gst_tag_list_get_value_index(tags, GST_TAG_IMAGE, 0);
			if ( gv_image )
			{
				GstBuffer *buf_image;
				buf_image = gst_value_get_buffer (gv_image);
				int fd = open("/tmp/.id3coverart", O_CREAT|O_WRONLY|O_TRUNC, 0644);
				int ret = write(fd, GST_BUFFER_DATA(buf_image), GST_BUFFER_SIZE(buf_image));
				close(fd);
				printf("cPlayback::state /tmp/.id3coverart %d bytes written\n", ret);
			}
			gst_tag_list_free(tags);
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

	return GST_BUS_DROP;
}
#endif

cPlayback::cPlayback(int num)
{ 
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);

#if ENABLE_GSTREAMER
	gst_init(NULL, NULL);
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
	//FIXME: all deleting stuff is done in Close()
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
		player->output->Command(player,OUTPUT_ADD, (void*)"subtitle");		
	}

	// subtitle
	SubtitleOutputDef_t out;

	out.screen_width = CFrameBuffer::getInstance()->getScreenWidth();
	out.screen_height = CFrameBuffer::getInstance()->getScreenHeight();
	out.framebufferFD = CFrameBuffer::getInstance()->getFileHandle();
	out.destination   = (unsigned char *)CFrameBuffer::getInstance()->getFrameBufferPointer();
	out.destStride    = CFrameBuffer::getInstance()->getStride();;
	out.shareFramebuffer = 1;
    
	player->output->subtitle->Command(player, (OutputCmd_t)OUTPUT_SET_SUBTITLE_OUTPUT, (void*) &out);
#endif

	return true;
}

// used by movieplay
void cPlayback::Close(void)
{  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
#if ENABLE_GSTREAMER
	// disconnect bus handler
	if (m_gst_playbin)
	{
		// disconnect sync handler callback
		GstBus * bus = gst_pipeline_get_bus(GST_PIPELINE (m_gst_playbin));
		gst_bus_set_sync_handler(bus, NULL, NULL);
		gst_object_unref(bus);
		
		printf("GST bus handler closed\n");
	}
	
	Stop();
	
	if (m_stream_tags)
		gst_tag_list_free(m_stream_tags);

	// close gst
	if (m_gst_playbin)
	{
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
	Stop();
	
	if(player && player->output) 
	{
		player->output->Command(player, OUTPUT_CLOSE, NULL);
		player->output->Command(player,OUTPUT_DEL, (void*)"audio");
		player->output->Command(player,OUTPUT_DEL, (void*)"video");		
		player->output->Command(player,OUTPUT_DEL, (void*)"subtitle");	
	}

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_CLOSE, NULL);


	if(player != NULL)
	{
		free(player);

		player = NULL;
	}
#endif	
}

// start
bool cPlayback::Start(char * filename)
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
	
	if (isHTTP)
		uri = g_uri_escape_string(filename, G_URI_RESERVED_CHARS_GENERIC_DELIMITERS, true);
	else
		uri = g_filename_to_uri(filename, NULL, NULL);
	
	// create gst pipeline
	m_gst_playbin = gst_element_factory_make("playbin2", "playbin");

	if(m_gst_playbin)
	{
		g_object_set(G_OBJECT (m_gst_playbin), "uri", uri, NULL);
		g_object_set(G_OBJECT (m_gst_playbin), "flags", flags, NULL);	
	
		//gstbus handler
		GstBus * bus = gst_pipeline_get_bus( GST_PIPELINE(m_gst_playbin) );
		gst_bus_set_sync_handler(bus, Gst_bus_call, NULL);
		gst_object_unref(bus); 
		
		// state playing
		gst_element_set_state(GST_ELEMENT(m_gst_playbin), GST_STATE_PLAYING);
		
		playing = true;
		playstate = STATE_PLAY;
	}
	else
	{
		printf("failed to create GStreamer pipeline!, sorry we can not play\n");
		playing = false;
		
		return false;
	}
	
	g_free(uri);
	
	// set buffer size
	int m_buffer_size = 5*1024*1024;
	g_object_set(G_OBJECT (m_gst_playbin), "buffer-size", m_buffer_size, NULL);
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
	
#if defined (ENABLE_GSTREAMER)
	if(m_gst_playbin)
	{
		gst_element_set_state(GST_ELEMENT(m_gst_playbin), GST_STATE_PLAYING);
		
		playing = true;
		playstate = STATE_PLAY;
	}
#else
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

	dprintf(DEBUG_INFO, "%s:%s playing %d\n", FILENAME, __FUNCTION__, playing);

	return playing;
}

bool cPlayback::Stop(void)
{ 
	if(playing == false) 
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s playing %d\n", FILENAME, __FUNCTION__, playing);

#if defined (ENABLE_GSTREAMER)
	// stop
	if(m_gst_playbin)
	{
		gst_element_set_state(m_gst_playbin, GST_STATE_NULL);
	}
#else
	if(player && player->playback && player->output) 
		player->playback->Command(player, PLAYBACK_STOP, NULL);
	
	if(player && player->container && player->container->selectedContainer)
		player->container->selectedContainer->Command(player, CONTAINER_STOP, NULL);
#endif

	playing = false;
	
	dprintf(DEBUG_INFO, "%s:%s playing %d\n", FILENAME, __FUNCTION__, playing);
	
	playstate = STATE_STOP;

	return true;
}

bool cPlayback::SetAPid(unsigned short pid)
{
	dprintf(DEBUG_INFO, "%s:%s curpid:%d nextpid:%d\n", FILENAME, __FUNCTION__, mAudioStream, pid);
	
#if ENABLE_GSTREAMER
	int current_audio;
	
	if(pid != mAudioStream)
	{
		g_object_set (G_OBJECT (m_gst_playbin), "current-audio", pid, NULL);
		printf("%s: switched to audio stream %i\n", __FUNCTION__, pid);
		mAudioStream = pid;
	}

#else
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

#if ENABLE_GSTREAMER
void cPlayback::trickSeek(int ratio)
{
	bool validposition = false;
	gint64 pos = 0;
	int position;
	int duration;
	
	if( GetPosition(position, duration) )
	{
		validposition = true;
		pos = position;
	}

	gst_element_set_state(m_gst_playbin, GST_STATE_PLAYING);
			
	if (validposition)
	{
		if(ratio >= 0.0)
			gst_element_seek(m_gst_playbin, ratio, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SKIP), GST_SEEK_TYPE_SET, pos, GST_SEEK_TYPE_SET, -1);
		else
			gst_element_seek(m_gst_playbin, ratio, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SKIP), GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, pos);
	}
}
#endif

bool cPlayback::SetSpeed(int speed)
{  
	dprintf(DEBUG_INFO, "%s:%s speed %d\n", FILENAME, __FUNCTION__, speed);	

	if(playing == false) 
		return false;

#if defined (ENABLE_GSTREAMER)
	if(m_gst_playbin)
	{	
		// pause
		if(speed == 0)
		{
			gst_element_set_state(m_gst_playbin, GST_STATE_PAUSED);
			//trickSeek(0);
			playstate = STATE_PAUSE;
		}
		// play/continue
		else if(speed == 1)
		{
			trickSeek(1);
			//gst_element_set_state(m_gst_playbin, GST_STATE_PLAYING);
			//
			playstate = STATE_PLAY;
		}
		//ff
		else if(speed > 1)
		{
			trickSeek(speed);
			//
			playstate = STATE_FF;
		}
		//rf
		else if(speed < 0)
		{
			trickSeek(speed);
			//
			playstate = STATE_REW;
		}
	}
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

#if ENABLE_GSTREAMER
	if(m_gst_playbin)
	{
		trickSeek(0.5);
	}
#else
	if(player && player->playback) 
	{
		player->playback->Command(player, PLAYBACK_SLOWMOTION, (void*)&slow);
	}
#endif

	playstate = STATE_SLOW;

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

#if ENABLE_GSTREAMER

	//EOF
	if(end_eof)
	{
		end_eof = 0;
		return false;
	}
	
	if(m_gst_playbin)
	{
		//position
		GstFormat fmt = GST_FORMAT_TIME; //Returns time in nanosecs
		
		gint64 pts = 0;
		unsigned long long int sec = 0;
		
		gst_element_query_position(m_gst_playbin, &fmt, &pts);
		position = pts /  1000000.0;
	
		// duration
		GstFormat fmt_d = GST_FORMAT_TIME; //Returns time in nanosecs
		double length = 0;
		gint64 len;

		gst_element_query_duration(m_gst_playbin, &fmt_d, &len);
		length = len / 1000000.0;
		if(length < 0) 
			length = 0;
		
		duration = (int)(length);
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

	/* len is in nanoseconds. we have 90 000 pts per second. */
	position = vpts/90;

	// duration
	double length = 0;

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_LENGTH, &length);
	
	if(length < 0) 
		length = 0;

	duration = (int)(length*1000);
#endif
	
	return true;
}

bool cPlayback::SetPosition(int position)
{
	if(playing == false) 
		return false;
	
#if ENABLE_GSTREAMER
	gint64 time_nanoseconds;
	gint64 pos;
	GstFormat fmt = GST_FORMAT_TIME;
		
	if(m_gst_playbin)
	{
		gst_element_query_position(m_gst_playbin, &fmt, &pos);
		time_nanoseconds = pos + (position * 1000000.0);
		if(time_nanoseconds < 0) 
			time_nanoseconds = 0;
		
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

#if defined (ENABLE_GSTREAMER)
	if(m_gst_playbin)
	{
		gint i, n_audio = 0;
		//GstStructure * structure = NULL;
		
		// get audio
		g_object_get (m_gst_playbin, "n-audio", &n_audio, NULL);
		printf("%s: %d audio\n", __FUNCTION__, n_audio);
		
		if(n_audio == 0)
			return;
		
		for (i = 0; i < n_audio; i++)
		{
			// apids
			apids[i]=i;
			
			GstPad * pad = 0;
			g_signal_emit_by_name (m_gst_playbin, "get-audio-pad", i, &pad);
			GstCaps * caps = gst_pad_get_negotiated_caps(pad);
			if (!caps)
				continue;
			
			GstStructure * structure = gst_caps_get_structure(caps, 0);
			//const gchar *g_type = gst_structure_get_name(structure);
		
			//if (!structure)
				//return atUnknown;
			//ac3flags[0] = 0;

			// ac3flags
			if ( gst_structure_has_name (structure, "audio/mpeg"))
			{
				gint mpegversion, layer = -1;
				
				if (!gst_structure_get_int (structure, "mpegversion", &mpegversion))
					//return atUnknown;
					ac3flags[i] = 0;

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
						ac3flags[i] = 4;
					case 2:
						//return atAAC;
						ac3flags[i] = 5;
					case 4:
						//return atAAC;
						ac3flags[i] = 5;
					default:
						//return atUnknown;
						ac3flags[i] = 0;
				}
			}
			else if ( gst_structure_has_name (structure, "audio/x-ac3") || gst_structure_has_name (structure, "audio/ac3") )
				//return atAC3;
				ac3flags[i] = 1;
			else if ( gst_structure_has_name (structure, "audio/x-dts") || gst_structure_has_name (structure, "audio/dts") )
				//return atDTS;
				ac3flags[i] = 6;
			else if ( gst_structure_has_name (structure, "audio/x-raw-int") )
				//return atPCM;
				ac3flags[i] = 0;
			
			gst_caps_unref(caps);
		}
		
		// numpids
		*numpida=i;
	}
#else
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



