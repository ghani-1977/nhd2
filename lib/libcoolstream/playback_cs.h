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

#ifndef __PLAYBACK_CS_H
#define __PLAYBACK_CS_H

#include <string>
#include <stdint.h>
#include <vector>

#include <config.h>


#if !ENABLE_GSTREAMER
#include <common.h>
#include <subtitle.h>
#include <linux/fb.h>


extern OutputHandler_t		OutputHandler;
extern PlaybackHandler_t	PlaybackHandler;
extern ContainerHandler_t	ContainerHandler;
extern ManagerHandler_t		ManagerHandler;
#endif

typedef enum {
	STATE_STOP,
	STATE_PLAY,
	STATE_PAUSE,
	STATE_FF,
	STATE_REW,
	STATE_SLOW
} playstate_t;


class cPlayback
{
	private:
#if !ENABLE_GSTREAMER
		Context_t * player;
#endif
		bool playing;

		int mSpeed;
		int mAudioStream;

	public:
		playstate_t playstate;
		
		bool Open();
		void Close(void);
		bool Start(char * filename);
		
		bool Play(void);
		bool SyncAV(void);
		
		bool Stop(void);
		bool SetAPid(unsigned short pid);

#if ENABLE_GSTREAMER
		void trickSeek(int ratio);
#endif		
		bool SetSpeed(int speed);
		bool SetSlow(int slow);
		bool GetSpeed(int &speed) const;
		bool GetPosition(int &position, int &duration);
		bool SetPosition(int position);
		void FindAllPids(uint16_t *apids, unsigned short *ac3flags, uint16_t *numpida, std::string *language);

		cPlayback(int num = 0);
		~cPlayback();
		void getMeta();		
};

#endif
