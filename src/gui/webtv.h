/*
	$Id: webtv.h 2013/09/03 10:45:30 mohousch Exp $

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

#ifndef __webtv_setup_h__
#define __webtv_setup_h__

#include <sys/types.h>
#include <string.h>
#include <vector>

#include <sys/stat.h>

#include <driver/file.h>

#include <xmlinterface.h>


class CWebTV
{
	private:
		enum {
			WEBTV,
			NETZKINO,
			USER,
			DIVERS = 255
		};
		
		struct webtv_channels {
			char * title;
			char * url;
			char * description;
			char * locked;		// for parentallock
		};

		xmlDocPtr parser;
		bool readChannellist(std::string filename);
		
		std::vector<webtv_channels *> channels;
		
		/* gui */
		CFrameBuffer * frameBuffer;
		
		int            	width;
		int            	height;
		int            	x;
		int            	y;
		
		int            	theight; 	// title font height
		int            	fheight; 	// foot font height (buttons???)
		
		unsigned int   	selected;
		unsigned int   	liststart;
		int		buttonHeight;
		unsigned int	listmaxshow;
		unsigned int	numwidth;
		int 		info_height;
		unsigned int 	selected_playing;
		unsigned int oldselected;
		unsigned int mode;
		
		void paintDetails(int index);
		void clearItem2DetailsLine ();
		void paintItem2DetailsLine (int pos, int ch_index);
		void paintItem(int pos);
		void paint();
		void paintHead();
		void hide();
		
		bool qZap;
		
	public:
		CWebTV();
		~CWebTV();
		int exec();
		
		int Show();
		
		CFileList filelist;
		CFile * getSelectedFile();
		
		void quickZap(int key);
		
		void showFileInfoWebTV();
};
#endif
