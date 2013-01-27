/*
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

#ifndef _AUDIO_CS_H_
#define _AUDIO_CS_H_

#include <linux/dvb/audio.h>

#include <config.h>

// stm_ioctl
#ifdef __sh__
#include <stm_ioctls.h>
#endif


// bcm
/*
[%s] (%d) NEXUS_AudioCodec_eMpeg 
[%s] (%d) NEXUS_AudioCodec_eAc3 
[%s] (%d) NEXUS_AudioCodec_eDts 
[%s] (%d) NEXUS_AudioCodec_eAac 
[%s] (%d) NEXUS_AudioCodec_eAacPlus 
[%s] (%d) NEXUS_AudioCodec_eLpcmDvd 
[%s] (%d) NEXUS_AudioCodec_eMp3
*/
#ifndef __sh__
typedef enum {
	AUDIO_STREAMTYPE_AC3 = 0,
	AUDIO_STREAMTYPE_MPEG,
	AUDIO_STREAMTYPE_DTS,
	AUDIO_STREAMTYPE_LPCMDVD = 6,
	AUDIO_STREAMTYPE_AAC = 8,
	AUDIO_STREAMTYPE_AACPLUS,
	AUDIO_STREAMTYPE_MP3
}AUDIO_FORMAT;
#endif

// av sync
enum {
	AVSYNC_OFF,
	AVSYNC_ON,
	AVSYNC_AM
};

// ac3
enum {
	AC3_PASSTHROUGH,
	AC3_DOWNMIX
};


class cAudio
{
	private:
		int audio_fd;
		int audio_num;
		
		// for pcm playback
#ifdef __sh__		
		int uNoOfChannels;
		int uSampleRate;
		int uBitsPerSample;
		int bLittleEndian;
#elif !defined (ENABLE_GSTREAMER)
		int clipfd;
#endif		
		
		bool Muted;
#ifdef __sh__
		audio_encoding_t EncodingType;		
		stream_type_t StreamType;
#else
		AUDIO_FORMAT StreamType;
#endif
	
		int volume;
		
		int m_pcm_delay;
		int m_ac3_delay;

	public:
		// construct & destruct
		cAudio();
		~cAudio(void);
		
		// shut up
		int SetMute(int enable);

		// volume, min = 0, max = 255
		int setVolume(unsigned int left, unsigned int right);
		int getVolume(void) { return volume;}

		// start and stop audio
		int Start(void);
		int Stop(void);
		bool Pause();
		bool Resume();
#ifdef __sh__		
		void SetStreamType(stream_type_t type);
		void SetEncoding(audio_encoding_t type);
				
		audio_encoding_t GetStreamType(void) { return EncodingType; }
#else
		void SetStreamType(AUDIO_FORMAT type);
		AUDIO_FORMAT GetStreamType(void) { return StreamType; }
#endif
		
		void SetSyncMode(int Mode);

		bool Open(int num = 0);
		bool Close();

		// flush	
		int Flush(void);	

		// select channels
		int setChannel(int channel);
		
		// for pcm playback
		int PrepareClipPlay(int NoOfChannels, int SampleRate, int BitsPerSample, int LittleEndian);
		int WriteClip(unsigned char * buffer, int size);
		int StopClip();		
		
		void SetHdmiDD(int ac3);
		
		// audio stream source		
		int setSource(audio_stream_source_t source);
		
		int setHwPCMDelay(int delay);
		int setHwAC3Delay(int delay);
};

#endif

