/*
	WebTV menue

	Copyright (C) 2012 martii

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include <xmlinterface.h>


#define DEFAULT_WEBTV_XMLFILE 		CONFIGDIR "/webtv.xml"

class CWebTV : public CMenuTarget
{
	private:
		xmlDocPtr parser;
		bool readXml();
		
		std::vector<std::pair<char*, char*> > channels;
		
		int            	width;
		int            	height;
		int            	x;
		int            	y;
		
		int            	theight; 	// Fonthoehe Playlist-Titel
		int            	fheight; 	// Fonthoehe Playlist-Inhalt
		
		unsigned int   	selected;
		unsigned int   	liststart;
		int		buttonHeight;
		unsigned int	listmaxshow;
		unsigned int	numwidth;
		
		//
		CFrameBuffer * frameBuffer;
		
		int            m_LastMode;
		
		int info_height;
		
		void paintDetails(int index);
		void clearItem2DetailsLine ();
		void paintItem2DetailsLine (int pos, int ch_index);
		void paintItem(int pos);
		void paint();
		void paintHead();
		void hide();
		//
		
	public:
		CWebTV();
		~CWebTV();
		int exec(CMenuTarget * parent, const std::string & actionKey);
		int Show();
};
#endif
