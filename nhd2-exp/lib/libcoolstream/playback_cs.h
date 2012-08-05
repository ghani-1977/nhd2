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


#if defined (ENABLE_LIBEPLAYER3)
#include <libeplayer3/include/common.h>
#include <libeplayer3/include/subtitle.h>
#include <linux/fb.h>


extern OutputHandler_t		OutputHandler;
extern PlaybackHandler_t	PlaybackHandler;
extern ContainerHandler_t	ContainerHandler;
extern ManagerHandler_t		ManagerHandler;
#endif

#if defined (ENABLE_GSTREAMER)
#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
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
#if defined (ENABLE_LIBEPLAYER3)
		Context_t * player;
#elif defined (ENABLE_GSTREAMER)
		GstElement *m_gst_playbin;
#endif
		bool playing;

		playstate_t playstate;

		int mSpeed;
		int mAudioStream;

	public:
		bool Open();
		void Close(void);
		bool Start(char * filename, unsigned short vpid, int vtype, unsigned short apid, bool ac3);
		
		bool Play(void);
		bool SyncAV(void);
		
		bool Stop(void);
		bool SetAPid(unsigned short pid, bool ac3);
		bool SetSpeed(int speed);
		bool SetSlow(int slow);
		bool GetSpeed(int &speed) const;
		bool GetPosition(int &position, int &duration);

		bool SetPosition(int position, bool absolute = false);

		void FindAllPids(uint16_t *apids, unsigned short *ac3flags, uint16_t *numpida, std::string *language);

		cPlayback(int num = 0);
		~cPlayback();
		void getMeta();
};

#endif
