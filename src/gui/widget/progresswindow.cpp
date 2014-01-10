/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: progresswindow.cpp 2013/10/12 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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

#include "progresswindow.h"

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>

#include <gui/color.h>


CProgressWindow::CProgressWindow()
{
	frameBuffer = CFrameBuffer::getInstance();
	hheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	width       = w_max (600, 0);
	height      = h_max(hheight+5*mheight, 20);

	global_progress = local_progress = 101;
	statusText = "";

	x = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - width ) >> 1 );
	y = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - height) >> 1 );

	caption = NONEXISTANT_LOCALE;
}

void CProgressWindow::setTitle(const neutrino_locale_t title)
{
	caption = title;

#ifdef LCD_UPDATE
	CVFD::getInstance()->showProgressBar2(-1, NULL, -1, g_Locale->getText(caption)); // set global text in VFD
#endif // VFD_UPDATE
}


void CProgressWindow::showGlobalStatus(const unsigned int prog)
{
	if (global_progress == prog)
		return;

	global_progress = prog;

	int pos = x + 10;

	if(global_progress != 0)
	{
		if (global_progress > 100)
			global_progress = 100;

		pos += int( float(width - 20)/100.0 * global_progress);
		//vordergrund
		frameBuffer->paintBox(x + 10, globalstatusY, pos, globalstatusY + 10, COL_MENUCONTENT_PLUS_7);
	}
	
	//hintergrund
	frameBuffer->paintBox(pos, globalstatusY, x + width - 10, globalstatusY + 10, COL_MENUCONTENT_PLUS_2);
	
#if !defined USE_OPENGL
	frameBuffer->blit();
#endif	

#ifdef LCD_UPDATE
	CVFD::getInstance()->showProgressBar2(-1, NULL, global_progress);
#endif // VFD_UPDATE
}

void CProgressWindow::showLocalStatus(const unsigned int prog)
{
	if (local_progress == prog)
		return;

	local_progress = prog;

	int pos = x + 10;

	if (local_progress != 0)
	{
		if (local_progress > 100)
			local_progress = 100;

		pos += int( float(width - 20)/100.0 * local_progress);
		
		//vordergrund
		frameBuffer->paintBox(x + 10, localstatusY, pos, localstatusY + 10, COL_MENUCONTENT_PLUS_7);
	}
	
	//hintergrund
	frameBuffer->paintBox(pos, localstatusY, x + width - 10, localstatusY + 10, COL_MENUCONTENT_PLUS_2);
	
#if !defined USE_OPENGL
	frameBuffer->blit();
#endif	

#ifdef LCD_UPDATE
	CVFD::getInstance()->showProgressBar2(local_progress);
#endif // VFD_UPDATE
}

void CProgressWindow::showStatusMessageUTF(const std::string & text)
{
	statusText = text;
	frameBuffer->paintBox(x, statusTextY-mheight, x+width, statusTextY,  COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, statusTextY, width-20, text, COL_MENUCONTENT, 0, true); // UTF-8
	
#if !defined USE_OPENGL
	frameBuffer->blit();
#endif	

#ifdef LCD_UPDATE
	CVFD::getInstance()->showProgressBar2(-1, text.c_str()); // set local text in VFD
#endif // VFD_UPDATE
}

unsigned int CProgressWindow::getGlobalStatus(void)
{
	return global_progress;
}


void CProgressWindow::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height);

#if !defined USE_OPENGL
	frameBuffer->blit();
#endif
}

void CProgressWindow::paint()
{
	// title
	int ypos = y;
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	
	// icon
	int icon_w = 0;
	int icon_h = 0;
	
	frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &icon_w, &icon_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + 8, ypos + 8);
	
	// caption
	if (caption != NONEXISTANT_LOCALE)
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + 8 + icon_w + 5, ypos + hheight, width- 10, g_Locale->getText(caption), COL_MENUHEAD, 0, true); // UTF-8
		
	// footer	
	frameBuffer->paintBoxRel(x, ypos+ hheight, width, height- hheight, COL_MENUCONTENT_PLUS_0, RADIUS_MID, CORNER_BOTTOM);

	// msg
	ypos += hheight + (mheight >>1);
	statusTextY = ypos+mheight;
	showStatusMessageUTF(statusText);

	// local status
	ypos += mheight;
	localstatusY = ypos + mheight - 20;
	showLocalStatus(0);
	ypos += mheight +10;

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + 10, ypos + mheight, width - 10, g_Locale->getText(LOCALE_FLASHUPDATE_GLOBALPROGRESS), COL_MENUCONTENT, 0, true); // UTF-8
	ypos+= mheight;

	// global status
	globalstatusY = ypos + mheight - 20;
	ypos += mheight >>1;
	showGlobalStatus(global_progress);
}

int CProgressWindow::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	if(parent)
		parent->hide();
	
	paint();
	
#if !defined USE_OPENGL
	frameBuffer->blit();
#endif	

	return menu_return::RETURN_REPAINT;
}
