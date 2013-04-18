/*
  Neutrino-GUI  -   DBoxII-Project

  Copyright (C) 2003,2004 gagga
  Homepage: http://www.giggo.de/dbox

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

#ifndef __movieplayergui__
#define __movieplayergui__

#include <config.h>
#include <configfile.h>

#include "driver/framebuffer.h"
#include "gui/filebrowser.h"
#include "gui/widget/menue.h"
#include "gui/moviebrowser.h"
#include "gui/movieinfo.h"

#include "gui/webtv.h"
          	
#include <stdio.h>

#include <string>
#include <vector>


class CMoviePlayerGui : public CMenuTarget
{
	public:
		enum state
		{
			STOPPED     =  0,
			PLAY        =  1,
			PAUSE       =  2,
			FF          =  3,
			REW         =  4,
			SLOW        =  5,
			SOFTRESET   = 99
		};

	private:
		void Init(void);
		CFrameBuffer * frameBuffer;
		int            m_LastMode;	
		const char     * filename;
		bool		stopped;

		std::string Path_local;
		std::string Path_vlc;
		std::string Path_vlc_settings;
		std::string Path_dvd;
		std::string Path_blueray;

		CFileBrowser * filebrowser;
		CMovieBrowser * moviebrowser;
		CWebTV * webtv;

		void PlayFile();
		void cutNeutrino();
		void restoreNeutrino();

		CFileFilter tsfilefilter;
		CFileFilter vlcfilefilter;

		void showHelpTS(void);
		void showFileInfoVLC(void);
		void showFileInfoWebTV();
		
	public:
		CMoviePlayerGui();
		~CMoviePlayerGui();
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CAPIDSelectExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget * parent, const std::string & actionKey);
};

// MovieInfoViewer class
class CMovieInfoViewer
{
	private:
		CFrameBuffer * frameBuffer;
		
		bool visible;
		int m_xstart, m_xend, m_y, m_height, m_width, twidth;

		void GetDimensions();
		
		int BoxStartX, BoxStartY, BoxEndY, BoxEndX;
		int BoxWidth, BoxHeight;
		
		//CProgressBar * timescale;

	public:
		CMovieInfoViewer();
		~CMovieInfoViewer();
		void show(int Position);
		void updatePos(short runningPercent);
		void hide();
		bool IsVisible() {return visible;}
};

#endif
