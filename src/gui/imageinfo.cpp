/*
	Neutrino-GUI  -   DBoxII-Project


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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/imageinfo.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>

#include <daemonc/remotecontrol.h>

#include <system/flashtool.h>
#include <video_cs.h>

#include "svn_version.h"


#define SVN_REV "SVN Rev.:"
#define GIT_REV "GIT Build:"

#include <gui/pictureviewer.h>
extern CPictureViewer * g_PicViewer;

extern cVideo * videoDecoder;

extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

CImageInfo::CImageInfo()
{
	frameBuffer = CFrameBuffer::getInstance();

	font_head   = SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME;;
	font_small  = SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL;
	font_info   = SNeutrinoSettings::FONT_TYPE_MENU;

	hheight     = g_Font[font_head]->getHeight();
	iheight     = g_Font[font_info]->getHeight();
	sheight     = g_Font[font_small]->getHeight();

	max_width = frameBuffer->getScreenWidth(true);
	max_height =  frameBuffer->getScreenHeight(true);

	width = frameBuffer->getScreenWidth() - 10;
	height = frameBuffer->getScreenHeight() - 10;

	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CImageInfo::~CImageInfo()
{   
	videoDecoder->Pig(-1, -1, -1, -1);
}

int CImageInfo::exec(CMenuTarget* parent, const std::string &)
{
	if (parent)
 		parent->hide();

	paint();

	paint_pig( (width - width/3), y, width/3, height/3);	

#if !defined USE_OPENGL
	frameBuffer->blit();	
#endif

	neutrino_msg_t msg;

	while (1)
	{
		neutrino_msg_data_t data;
		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd_MS(100);
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if (msg <= CRCInput::RC_MaxRC)
		{
			break;
		}

		if ( msg >  CRCInput::RC_MaxRC && msg != CRCInput::RC_timeout)
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
		}
		
#if !defined USE_OPENGL
		frameBuffer->blit();	
#endif		
	}

	hide();

	return menu_return::RETURN_REPAINT;
}

void CImageInfo::hide()
{  
	videoDecoder->Pig(-1, -1, -1, -1);
	
	frameBuffer->paintBackgroundBoxRel(0 , 0, max_width, max_height);

#if !defined USE_OPENGL
	frameBuffer->blit();
#endif	
}

void CImageInfo::paint_pig(int x, int y, int w, int h)
{
	frameBuffer->paintBackgroundBoxRel(x, y, w, h);	
		
	//dont pig if we have 1980 x 1080
#ifdef __sh__	
	int xres, yres, framerate;
	videoDecoder->getPictureInfo(xres, yres, framerate);
	
	if(xres <= 1280)	
		videoDecoder->Pig( x, y, w, h );
#else
	videoDecoder->Pig( x, y, w, h );
#endif	
}

void CImageInfo::paintLine(int xpos, int font, const char* text)
{
	char buf[100];
	sprintf((char*) buf, "%s", text);
	
	g_Font[font]->RenderString(xpos, ypos, width-10, buf, COL_INFOBAR, 0, true);
}

void CImageInfo::paint()
{
	const char * head_string;
	char imagedate[18] = "";
 	int  xpos = x+10;

	ypos = y+5;

	head_string = g_Locale->getText(LOCALE_IMAGEINFO_HEAD);

	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8, head_string);

	frameBuffer->paintBoxRel(0, 0, max_width, max_height, COL_INFOBAR_PLUS_0);
	g_Font[font_head]->RenderString(xpos, ypos+ hheight+1, width, head_string, COL_MENUHEAD, 0, true);

	ypos += hheight;
	ypos += (iheight >>1);


	CConfigFile config('\t');
	if( !config.loadConfig("/var/etc/.version"));
		printf("/var/etc/.version not found using default.\n");

	const char * imagename = config.getString("imagename", "NeutrinoHD2").c_str();
	const char * homepage  = config.getString("homepage",  "http://www.dgstation-forum.org").c_str();
	const char * creator   = config.getString("creator",   "mohousch").c_str();
	const char * version   = config.getString("version",   "1200201205091849").c_str();
	const char * docs      = config.getString("docs",   "http://wiki.neutrino-hd.de").c_str();
	const char * forum     = config.getString("forum",   "http://www.dgstation-forum.org").c_str();
#ifdef SVNVERSION
	const char * builddate     = config.getString("builddate",     SVNVERSION).c_str();
#else
	const char * builddate     = config.getString("builddate",     BUILT_DATE).c_str();
#endif	

	static CFlashVersionInfo versionInfo(version);
	const char * releaseCycle = versionInfo.getReleaseCycle();
	const char * imageType = versionInfo.getType();
	sprintf((char*) imagedate, "%s  %s", versionInfo.getDate(), versionInfo.getTime());

	// image name
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_IMAGE));
	paintLine(xpos+125, font_info, imagename);

	// image date
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_DATE));
	paintLine(xpos+125, font_info, imagedate);

	// release cycle
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_VERSION));
	paintLine(xpos+125, font_info, releaseCycle);
	
	// svn/git built date
	ypos += iheight;
#ifdef SVNVERSION
	paintLine(xpos    , font_info, SVN_REV);
#else
	paintLine(xpos    , font_info, GIT_REV);
#endif
	paintLine(xpos + 125, font_info, builddate );	
	
	// image type
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_TYPE));
	paintLine(xpos+125, font_info, imageType);

	// image creator
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_CREATOR));
	paintLine(xpos+125, font_info, creator);

	// homepage
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_HOMEPAGE));
	paintLine(xpos+125, font_info, homepage);

	/* doko */
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_DOKUMENTATION));
	paintLine(xpos+125, font_info, docs);

	// forum
	ypos += iheight;
	paintLine(xpos    , font_info, g_Locale->getText(LOCALE_IMAGEINFO_FORUM));
	paintLine(xpos+125, font_info, forum);

	// license
	ypos += iheight;
	paintLine(xpos, font_info,g_Locale->getText(LOCALE_IMAGEINFO_LICENSE));
	paintLine(xpos+125, font_small, "This program is free software; you can redistribute it and/or modify");

	ypos += sheight;
	paintLine(xpos+125, font_small, "it under the terms of the GNU General Public License as published by");

	ypos += sheight;
	paintLine(xpos+125, font_small, "the Free Software Foundation; either version 2 of the License, or");

	ypos += sheight;
	paintLine(xpos+125, font_small, "(at your option) any later version.");

	ypos += sheight;
	paintLine(xpos+125, font_small, "This program is distributed in the hope that it will be useful,");

	ypos += sheight;
	paintLine(xpos+125, font_small, "but WITHOUT ANY WARRANTY; without even the implied warranty of");

	ypos += sheight;
	paintLine(xpos+125, font_small, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");

	ypos += sheight;
	paintLine(xpos+125, font_small, "See the GNU General Public License for more details.");

	ypos += sheight;
	paintLine(xpos+125, font_small, "You should have received a copy of the GNU General Public License");

	ypos += sheight;
	paintLine(xpos+125, font_small, "along with this program; if not, write to the Free Software");

	ypos += sheight;
	paintLine(xpos+125, font_small, "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.");	
}
