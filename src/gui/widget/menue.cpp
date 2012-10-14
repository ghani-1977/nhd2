/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/widget/menue.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/color.h>

#include <gui/widget/stringinput.h>

#include <global.h>
#include <neutrino.h>
#include <gui/widget/icons.h>

#include <cctype>


static int HEIGHT;


/* the following generic menu items are integrated into multiple menus at the same time */
CMenuSeparator CGenericMenuSeparator;
CMenuSeparator CGenericMenuSeparatorLine(CMenuSeparator::LINE);

CMenuForwarder CGenericMenuBack(LOCALE_MENU_BACK, true, NULL, NULL, NULL, CRCInput::RC_nokey, NEUTRINO_ICON_BUTTON_LEFT);
//CMenuForwarder CGenericMenuCancel(LOCALE_MENU_CANCEL, true, NULL, NULL, NULL, CRCInput::RC_nokey, NEUTRINO_ICON_BUTTON_HOME);
//CMenuForwarder CGenericMenuNext(LOCALE_MENU_NEXT, true, NULL, NULL, NULL, CRCInput::RC_nokey, NEUTRINO_ICON_BUTTON_HOME);

CMenuSeparator * const GenericMenuSeparator = &CGenericMenuSeparator;
CMenuSeparator * const GenericMenuSeparatorLine = &CGenericMenuSeparatorLine;

CMenuForwarder * const GenericMenuBack = &CGenericMenuBack;
//CMenuForwarder * const GenericMenuCancel = &CGenericMenuCancel;
//CMenuForwarder * const GenericMenuNext = &CGenericMenuNext;


// CMenuItem
void CMenuItem::init(const int X, const int Y, const int DX, const int OFFX)
{
	x    = X;
	y    = Y;
	dx   = DX;
	offx = OFFX;
}

void CMenuItem::setActive(const bool Active)
{
	active = Active;
	
	if (x != -1)
		paint();
}

// CMenuWidget
CMenuWidget::CMenuWidget()
{
        nameString = g_Locale->getText(NONEXISTANT_LOCALE);
	name = NONEXISTANT_LOCALE;
        iconfile = "";
        selected = -1;
        iconOffset = 0;
	offx = offy = 0;
	//savescreen	= false;
	//background	= NULL;
}

CMenuWidget::CMenuWidget(const neutrino_locale_t Name, const std::string & Icon, const int mwidth, const int mheight )
{
	name = Name;
        nameString = g_Locale->getText(NONEXISTANT_LOCALE);

	Init(Icon, mwidth, mheight);
}

CMenuWidget::CMenuWidget(const char* Name, const std::string & Icon, const int mwidth, const int mheight)
{
	name = NONEXISTANT_LOCALE;
        nameString = Name;

	Init(Icon, mwidth, mheight);
}

void CMenuWidget::Init(const std::string & Icon, const int mwidth, const int mheight)
{
        frameBuffer = CFrameBuffer::getInstance();
        iconfile = Icon;
        selected = -1;
        width = mwidth;
	
        if(width > (int) frameBuffer->getScreenWidth())
		width = frameBuffer->getScreenWidth();
	
      	height = mheight;
        wanted_height = mheight;

        current_page=0;
	offx = offy = 0;
}

void CMenuWidget::move(int xoff, int yoff)
{
	offx = xoff;
	offy = yoff;
}

CMenuWidget::~CMenuWidget()
{
	for(unsigned int count=0;count<items.size();count++) 
	{
		CMenuItem * item = items[count];
		
		if ((item != GenericMenuSeparator) && (item != GenericMenuSeparatorLine) && (item != GenericMenuBack))
			delete item;
	}

	items.clear();
	page_start.clear();
}

void CMenuWidget::addItem(CMenuItem* menuItem, const bool defaultselected)
{
	if (defaultselected)
		selected = items.size();
	
	items.push_back(menuItem);
}

bool CMenuWidget::hasItem()
{
	return !items.empty();
}

#define FADE_TIME 40000
int CMenuWidget::exec(CMenuTarget * parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int pos = 0;
	exit_pressed = false;

	if (parent)
		parent->hide();


	bool fadeIn = false /*g_settings.widget_fade*/;
	bool fadeOut = false;
	int fadeValue;
	uint32_t fadeTimer = 0;

        if ( fadeIn ) 
	{
		fadeValue = 0x10;
		
		frameBuffer->setBlendLevel(fadeValue);
        }
        else
		fadeValue = g_settings.gtx_alpha;
	
	//if(savescreen) 
	//{
	//	saveScreen();
	//}

	paint();
	
#ifdef FB_BLIT	
	frameBuffer->blit();
#endif	

	int retval = menu_return::RETURN_REPAINT;
	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU] == 0 ? 0xFFFF : g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

	if ( fadeIn )
		fadeTimer = g_RCInput->addTimer( FADE_TIME, false );
	
	//control loop
	do {
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

		if ( msg <= CRCInput::RC_MaxRC ) 
		{
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU] == 0 ? 0xFFFF : g_settings.timing[SNeutrinoSettings::TIMING_MENU]);
		}
		
		int handled= false;

		for (unsigned int i= 0; i< items.size(); i++) 
		{
			CMenuItem * titem = items[i];
			
			if ((titem->directKey != CRCInput::RC_nokey) && (titem->directKey == msg)) 
			{
				if (titem->isSelectable()) 
				{
					items[selected]->paint( false );
					selected= i;
					msg= CRCInput::RC_ok;
				} 
				else 
				{
					// swallow-key...
					handled = true;
				}
				break;
			}
		}

		if (!handled) 
		{
			switch (msg) 
			{
				case (NeutrinoMessages::EVT_TIMER):
					if(data == fadeTimer) 
					{
						if (fadeOut) 
						{ 
							// disappear
							fadeValue -= 0x10;
							if (fadeValue <= 0x10) 
							{
								fadeValue = g_settings.gtx_alpha;
								g_RCInput->killTimer (fadeTimer);
								msg = CRCInput::RC_timeout;
							} 
							else
								frameBuffer->setBlendLevel(fadeValue);
						} 
						else 
						{ 
							// appears
							fadeValue += 0x10;

							if (fadeValue >= g_settings.gtx_alpha)
							{
								fadeValue = g_settings.gtx_alpha;
								g_RCInput->killTimer(fadeTimer);
								fadeIn = false;
								frameBuffer->setBlendLevel(g_settings.gtx_alpha);
							} 
							else
								frameBuffer->setBlendLevel(fadeValue);
						}
					} 
					else 
					{
						if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
						{
							retval = menu_return::RETURN_EXIT_ALL;
							msg = CRCInput::RC_timeout;
						}
					}
					break;
					
				case (CRCInput::RC_page_up) :
				case (CRCInput::RC_page_down) :
					if(msg == CRCInput::RC_page_up) 
					{
						if(current_page) 
						{
							pos = (int) page_start[current_page] - 1;
							for (unsigned int count=pos ; count > 0; count--) 
							{
								CMenuItem* item = items[pos];
								if ( item->isSelectable() ) 
								{
									if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page])) 
									{
										items[selected]->paint( false );
										item->paint( true );
										selected = pos;
									} 
									else 
									{
										selected=pos;
										paintItems();
									}
									break;
								}
								pos--;
							}
						} 
						else 
						{
							pos = 0;
							for (unsigned int count=0; count < items.size(); count++) 
							{
								CMenuItem* item = items[pos];
								if ( item->isSelectable() ) 
								{
									if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page])) 
									{
										items[selected]->paint( false );
										item->paint( true );
										selected = pos;
									} 
									else 
									{
										selected=pos;
										paintItems();
									}
									break;
								}
								pos++;
							}
						}
					}
					else if(msg==CRCInput::RC_page_down) 
					{
						pos = (int) page_start[current_page + 1];// - 1;
						if(pos >= (int) items.size()) 
							pos = items.size()-1;
						for (unsigned int count=pos ; count < items.size(); count++) 
						{
							CMenuItem* item = items[pos];
							if ( item->isSelectable() ) 
							{
								if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page])) 
								{
									items[selected]->paint( false );
									item->paint( true );
									selected = pos;
								} 
								else 
								{
									selected=pos;
									paintItems();
								}
								break;
							}
							pos++;
						}
					}
					break;
					
				case (CRCInput::RC_up) :
				case (CRCInput::RC_down) :
					{
						//search next / prev selectable item
						for (unsigned int count=1; count< items.size(); count++) 
						{
							if ( msg == CRCInput::RC_up ) 
							{
								pos = selected - count;
								if ( pos < 0 )
									pos += items.size();
							}
							else if( msg == CRCInput::RC_down ) 
							{
								pos = (selected+ count)%items.size();
							}

							CMenuItem* item = items[pos];

							if ( item->isSelectable() ) 
							{
								if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
								{ 
									// Item is currently on screen
									//clear prev. selected
									items[selected]->paint( false );
									//select new
									item->paint( true );
									selected = pos;
								} 
								else 
								{
									selected=pos;
									paintItems();
								}
								break;
							}
						}
					}
					break;
					
				case (CRCInput::RC_left):
					if(!(items[selected]->can_arrow) || g_settings.menu_left_exit) 
					{
						msg = CRCInput::RC_timeout;
						break;
					}
					
				case (CRCInput::RC_right):
				case (CRCInput::RC_ok):
					{
						if(hasItem()) 
						{
							//exec this item...
							CMenuItem* item = items[selected];
							item->msg = msg;
							if ( fadeIn ) 
							{
								g_RCInput->killTimer(fadeTimer);

								frameBuffer->setBlendLevel(g_settings.gtx_alpha);

								fadeIn = false;
							}
							int rv = item->exec( this );
							switch ( rv ) 
							{
								case menu_return::RETURN_EXIT_ALL:
									retval = menu_return::RETURN_EXIT_ALL;
								case menu_return::RETURN_EXIT:
									msg = CRCInput::RC_timeout;
									break;
								case menu_return::RETURN_REPAINT:
									paint();
									break;
							}
						} 
						else
							msg = CRCInput::RC_timeout;
					}
					break;

				case (CRCInput::RC_home):
					exit_pressed = true;
					msg = CRCInput::RC_timeout;
					break;
					
				case (CRCInput::RC_timeout):
					break;

				case (CRCInput::RC_sat):
				case (CRCInput::RC_favorites):
					g_RCInput->postMsg (msg, 0);
					
				//close any menue on setup-key
				case (CRCInput::RC_setup):
					{
						msg = CRCInput::RC_timeout;
						retval = menu_return::RETURN_EXIT_ALL;
					}
					break;

				default:
					if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
					{
						retval = menu_return::RETURN_EXIT_ALL;
						msg = CRCInput::RC_timeout;
					}
			}
			
			if(msg == CRCInput::RC_timeout) 
			{
				if ( fadeIn ) 
				{
					g_RCInput->killTimer(fadeTimer);
					fadeIn = false;
				}
				
				if ((!fadeOut) && false /*g_settings.widget_fade*/) 
				{
					fadeOut = true;
					fadeTimer = g_RCInput->addTimer( FADE_TIME, false );
					timeoutEnd = CRCInput::calcTimeoutEnd( 1 );
					msg = 0;
					continue;
				}
			}

			if ( msg <= CRCInput::RC_MaxRC )
			{
				// recalculate timeout f�r RC-Tasten
				timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU] == 0 ? 0xFFFF : g_settings.timing[SNeutrinoSettings::TIMING_MENU]);
			}
		}
		
#ifdef FB_BLIT		
		frameBuffer->blit();
#endif		
	}
	while ( msg!=CRCInput::RC_timeout );
	
	hide();	

	if ( fadeIn || fadeOut ) 
	{
		g_RCInput->killTimer(fadeTimer);

		frameBuffer->setBlendLevel(g_settings.gtx_alpha);
	}

	if(!parent)
		CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

	for (unsigned int count = 0; count < items.size(); count++) 
	{
		CMenuItem* item = items[count];
		item->init(-1, 0, 0, 0);
	}
	
	return retval;
}

void CMenuWidget::hide()
{
	//if( savescreen && background)
	//	restoreScreen();//FIXME
	//else
	frameBuffer->paintBackgroundBoxRel(x, y, width + SCROLLBAR_WIDTH, height + ((RADIUS_MID * 3) + 1) + 5); //15=sb_width, ((RADIUS_MID * 3) + 1)= foot 
	
#ifdef FB_BLIT
	frameBuffer->blit();
#endif
}

void CMenuWidget::paint()
{
	const char * l_name;
	
	if(name == NONEXISTANT_LOCALE)
		l_name = nameString.c_str();
	else
        	l_name = g_Locale->getText(name);	

	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8 );

	height = wanted_height;

	if(height > ((int)frameBuffer->getScreenHeight() - 10))
		height = frameBuffer->getScreenHeight() - 10;

	int neededWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(l_name, true); // UTF-8

	if (neededWidth > width - 48) 
	{
		width = neededWidth + 49;
		
		if(width > (int)frameBuffer->getScreenWidth())
			width = frameBuffer->getScreenWidth();
	}

	// add items
	int hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	int itemHeightTotal = 0;
	int item_height = 0;
	int heightCurrPage = 0;
	page_start.clear();
	page_start.push_back(0);
	total_pages = 1;

	for (unsigned int i= 0; i< items.size(); i++) 
	{
		item_height = items[i]->getHeight();
		itemHeightTotal += item_height;
		heightCurrPage += item_height;

		if(heightCurrPage > (height - hheight)) 
		{
			page_start.push_back(i);
			total_pages++;
			heightCurrPage = item_height;
		}
	}

	page_start.push_back(items.size());

	// icon offset
	iconOffset = 0;

	for (unsigned int i= 0; i< items.size(); i++) 
	{
		if ((!(items[i]->iconName.empty())) || CRCInput::isNumeric(items[i]->directKey))
		{
			iconOffset = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
			break;
		}
	}

	// shrink menu if less items
	if(hheight + itemHeightTotal < height)
		height = hheight + itemHeightTotal;

	// coordinations
	x = offx + frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - width ) >> 1 );
	y = offy + frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - height) >> 1 );

	int sb_width;
	if(total_pages > 1)
		sb_width = SCROLLBAR_WIDTH;
	else
		sb_width = 0;

	//CMenuWidget Head
	frameBuffer->paintBoxRel(x, y, width + sb_width, hheight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	
	//paint icon
	int icon_w = 0;
	int icon_h = 0;
	
	frameBuffer->getIconSize(iconfile.c_str(), &icon_w, &icon_h);
	
	frameBuffer->paintIcon(iconfile, x + 8, y + hheight/2 - icon_h/2);
	
	// head title (centered)
	int stringstartposX = x /*+ 8 + icon_w*/ + (width >> 1) - ( neededWidth >> 1);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(stringstartposX, y + hheight + 1, width - BORDER_RIGHT - (stringstartposX - x), l_name, COL_MENUHEAD, 0, true); // UTF-8
	
	// paint separator
	int sp_height = 5;
	frameBuffer->paintBoxRel(x, y + hheight, width + sb_width, sp_height, COL_MENUCONTENTDARK_PLUS_0 );
	
	// recalculate total height
	height = height + sp_height;
	
	//paint foot
	frameBuffer->paintBoxRel(x, y + height, width + sb_width, (RADIUS_MID * 3) + 1 + 5, COL_MENUHEAD_PLUS_0 );
	
	// all height
	HEIGHT = y + height + 25 + 5;
	//
	
	//item_start_y
	item_start_y = y + hheight + sp_height;
	
	// paint items
	paintItems();
}

/* paint items */
void CMenuWidget::paintItems()
{
	int item_height = height - (item_start_y - y); // all items height
	
	//printf("CMenuWidget::paintItems: y(%d) item_start_y(%d) item_height(%d)\n", y, item_start_y, item_height);

	//Item not currently on screen
	if (selected >= 0)
	{
		while(selected < (int)page_start[current_page])
			current_page--;
		
		while(selected >= (int)page_start[current_page + 1])
			current_page++;
	}

	// paint right scroll bar if we have more then one page
	if(total_pages > 1)
	{
		int sbh= ((item_height - 4) / total_pages);

		//scrollbar
		frameBuffer->paintBoxRel(x + width, item_start_y, SCROLLBAR_WIDTH, item_height, COL_MENUCONTENT_PLUS_1); //15 = sb width
		frameBuffer->paintBoxRel(x + width + 2, item_start_y + 2 + current_page * sbh, 11, sbh, COL_MENUCONTENT_PLUS_3);
	}
	
	// paint items background
	frameBuffer->paintBoxRel(x, item_start_y, width, item_height + 5, COL_MENUCONTENTDARK_PLUS_0 );

	int ypos = item_start_y;
	
	for (unsigned int count = 0; count < items.size(); count++) 
	{
		CMenuItem * item = items[count];

		if ((count >= page_start[current_page]) && (count < page_start[current_page + 1])) 
		{
			item->init(x, ypos, width, iconOffset);
			
			if( (item->isSelectable()) && (selected == -1) ) 
			{
				ypos = item->paint(true);
				selected = count;
			} 
			else 
			{
				ypos = item->paint( selected == ((signed int) count) );
			}
		} 
		else 
		{
			/* x = -1 is a marker which prevents the item from being painted on setActive changes */
			item->init(-1, 0, 0, 0);
		}	
	} 
}

void CMenuWidget::saveScreen()
{
	#if 0
	//if(!savescreen)
	//	return;

	//delete[] background;
	
	if(background) 
	{
		delete [] background;
		background = NULL;
	}

	background = new fb_pixel_t[(width + SCROLLBAR_WIDTH)*(height + ((RADIUS_MID * 3) + 1) + 5)];
	
	if(background)
	{
		frameBuffer->SaveScreen(x, y, width + SCROLLBAR_WIDTH, height + ((RADIUS_MID * 3) + 1) + 5, background);
		//frameBuffer->blit();
	}
	#endif
}

void CMenuWidget::restoreScreen()
{
	#if 0
	if(background) 
	{
		if(savescreen)
			frameBuffer->RestoreScreen(x, y, width + SCROLLBAR_WIDTH, height + ((RADIUS_MID * 3) + 1) + 5, background);
		
		//delete [] background;
		//background = NULL;
	}
	#endif
}

void CMenuWidget::enableSaveScreen(bool enable)
{
	#if 0
	savescreen = enable;
	
	if(!enable && background) 
	{
		delete[] background;
		background = NULL;
	}
	#endif
}

//CMenuOptionNumberChooser
CMenuOptionNumberChooser::CMenuOptionNumberChooser(const neutrino_locale_t name, int * const OptionValue, const bool Active, const int min_value, const int max_value, CChangeObserver * const Observ, const int print_offset, const int special_value, const neutrino_locale_t special_value_name, const char * non_localized_name)
{
	height               = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionName           = name;
	active               = Active;
	optionValue          = OptionValue;

	lower_bound          = min_value;
	upper_bound          = max_value;

	display_offset       = print_offset;

	localized_value      = special_value;
	localized_value_name = special_value_name;

	optionString         = non_localized_name;
	can_arrow	= true;
	observ = Observ;
}

int CMenuOptionNumberChooser::exec(CMenuTarget*)
{
	if( msg == CRCInput::RC_left ) 
	{
		if (((*optionValue) > upper_bound) || ((*optionValue) <= lower_bound))
			*optionValue = upper_bound;
		else
			(*optionValue)--;
	} 
	else 
	{
		if (((*optionValue) >= upper_bound) || ((*optionValue) < lower_bound))
			*optionValue = lower_bound;
		else
			(*optionValue)++;
	}
	
	paint(true);
	
	if(observ)
		observ->changeNotify(optionName, optionValue);

	return menu_return::RETURN_NONE;
}

int CMenuOptionNumberChooser::paint(bool selected)
{
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;

	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	
	if (!active)
	{
		color   = COL_MENUCONTENTINACTIVE;
		bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}
	
	// paint item
	frameBuffer->paintBoxRel(x + BORDER_LEFT, y, dx - (BORDER_LEFT + BORDER_RIGHT), height, bgcolor); //FIXME

	const char * l_option;
	char option_value[11];

	if ((localized_value_name == NONEXISTANT_LOCALE) || ((*optionValue) != localized_value))
	{
		sprintf(option_value, "%d", ((*optionValue) + display_offset));
		l_option = option_value;
	}
	else
		l_option = g_Locale->getText(localized_value_name);

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_option, true); // UTF-8
	int stringstartposName = x + offx + BORDER_LEFT;
	int stringstartposOption = x + dx - stringwidth - BORDER_RIGHT - BORDER_RIGHT/2; //+ offx

	const char * l_optionName = (optionString != NULL) ? optionString : g_Locale->getText(optionName);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName,   y + height, dx - (stringstartposName - x), l_optionName, color, 0, true); // UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + height, dx - (stringstartposOption - x), l_option, color, 0, true); // UTF-8

	// help bar
	if(g_settings.help_bar)
	{
		if (selected)
		{
			// refresh
			CFrameBuffer::getInstance()->paintBoxRel(x, HEIGHT - 25, dx, 25, COL_MENUHEAD_PLUS_0);
			
			// paint left/right icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_LEFT, x + BORDER_LEFT, HEIGHT - 25);
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_RIGHT, x + BORDER_LEFT + 16 +5, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT + 16 + 5 + 16 + 5, HEIGHT, dx - (x + BORDER_LEFT + 16 + 5 + 16 + 5 -x), (const char *)"Navi", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			
			// ok icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x + dx/2 - 20, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + dx/2 -20 + 24 + 5, HEIGHT, dx - (x + dx/2 -20 + 24 + 5 - x), (const char *)"Enter", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			
			// exit icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_HOME, x + dx - BORDER_RIGHT - 24 - 30, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + dx - BORDER_RIGHT - 30, HEIGHT, dx -(x + dx - BORDER_RIGHT - 30 - x), (const char *)"Exit", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
		}
	}
	
	// menutitle on VFD
	if(g_settings.menutitle_vfd)
	{
		if(selected)
		{
			char str[256];
			snprintf(str, 255, "%s %s", l_optionName, l_option);

			CVFD::getInstance()->showMenuText(0, str, -1, true);
		}
	}

	return y + height;
}

// CMenuOptionChooser
CMenuOptionChooser::CMenuOptionChooser(const neutrino_locale_t OptionName, int * const OptionValue, const struct keyval * const Options, const unsigned Number_Of_Options, const bool Active, CChangeObserver * const Observ, const neutrino_msg_t DirectKey, const std::string & IconName, bool Pulldown)
{
	height            = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionNameString  = g_Locale->getText(OptionName);
	optionName        = OptionName;
	options           = Options;
	active            = Active;
	optionValue       = OptionValue;
	number_of_options = Number_Of_Options;
	observ            = Observ;
	directKey         = DirectKey;
	iconName          = IconName;
	can_arrow	= true;
	pulldown = Pulldown;
}

CMenuOptionChooser::CMenuOptionChooser(const char* OptionName, int * const OptionValue, const struct keyval * const Options, const unsigned Number_Of_Options, const bool Active, CChangeObserver * const Observ, const neutrino_msg_t DirectKey, const std::string & IconName, bool Pulldown)
{
	height            = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionNameString  = OptionName;
	optionName        = NONEXISTANT_LOCALE;
	options           = Options;
	active            = Active;
	optionValue       = OptionValue;
	number_of_options = Number_Of_Options;
	observ            = Observ;
	directKey         = DirectKey;
	iconName          = IconName;
	can_arrow	= true;
	pulldown = Pulldown;
}

void CMenuOptionChooser::setOptionValue(const int newvalue)
{
	*optionValue = newvalue;
}

int CMenuOptionChooser::getOptionValue(void) const
{
	return *optionValue;
}

//FIXME: need to save/restore pulldowned menus
int CMenuOptionChooser::exec(CMenuTarget*)
{
	bool wantsRepaint = false;
	int ret = menu_return::RETURN_NONE;

	// pulldown
	if( msg == CRCInput::RC_ok && pulldown) 
	{
		int select = -1;
		char cnt[5];
		CMenuWidget * menu = new CMenuWidget(optionNameString.c_str(), NEUTRINO_ICON_SETTINGS);
		menu->move(20, 0);
		CMenuSelectorTarget * selector = new CMenuSelectorTarget(&select);
		
		// intros
		menu->addItem(GenericMenuSeparator);

		for(unsigned int count = 0; count < number_of_options; count++) 
		{
			bool selected = false;
			const char * l_option;
			
			if (options[count].key == (*optionValue))
				selected = true;

			if(options[count].valname != 0)
				l_option = options[count].valname;
			else
				l_option = g_Locale->getText(options[count].value);
			
			sprintf(cnt, "%d", count);
			menu->addItem(new CMenuForwarderNonLocalized(l_option, true, NULL, selector, cnt), selected);
		}
		
		menu->exec(NULL, "");
		ret = menu_return::RETURN_REPAINT;
		
		if(select >= 0) 
		{
			*optionValue = options[select].key;
		}
		
		delete menu;
		delete selector;
	} 
	else 
	{
		for(unsigned int count = 0; count < number_of_options; count++) 
		{
			if (options[count].key == (*optionValue)) 
			{
				if( msg == CRCInput::RC_left ) 
				{
					if(count > 0)
						*optionValue = options[(count-1) % number_of_options].key;
					else
						*optionValue = options[number_of_options-1].key;
				} 
				else
					*optionValue = options[(count+1) % number_of_options].key;
				break;
			}
		}
	}
	
	paint(true);
	
	if(observ)
		wantsRepaint = observ->changeNotify(optionName, optionValue);

	if ( wantsRepaint )
		ret = menu_return::RETURN_REPAINT;

	return ret;
}

int CMenuOptionChooser::paint( bool selected )
{
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;

	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	if (!active)
	{
		color   = COL_MENUCONTENTINACTIVE;
		bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}
	
	// paint item
	frameBuffer->paintBoxRel(x + BORDER_LEFT, y, dx - (BORDER_LEFT + BORDER_RIGHT), height, bgcolor); //FIXME

	neutrino_locale_t option = NONEXISTANT_LOCALE;
	const char * l_option = NULL;

	for(unsigned int count = 0 ; count < number_of_options; count++) 
	{
		if (options[count].key == *optionValue) 
		{
			option = options[count].value;
			if(options[count].valname != 0)
				l_option = options[count].valname;
			else
				l_option = g_Locale->getText(option);
			break;
		}
	}

	if(l_option == NULL) 
	{
		*optionValue = options[0].key;
		option = options[0].value;
		if(options[0].valname != 0)
			l_option = options[0].valname;
		else
			l_option = g_Locale->getText(option);
	}

	if (!(iconName.empty()))
	{
		/* get icon size */
		int icon_w;
		int icon_h;
		
		frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
		
		frameBuffer->paintIcon(iconName, x + BORDER_LEFT + BORDER_LEFT/2, y+ ((height/2- icon_h/2)) );
	}
	else if (CRCInput::isNumeric(directKey))
	{
		/* define icon name depends of numeric value */
		char i_name[6]; /* X +'\0' */
		snprintf(i_name, 6, "%d", CRCInput::getNumericValue(directKey));
		i_name[5] = '\0'; /* even if snprintf truncated the string, ensure termination */
		iconName = i_name;
		
		if (!iconName.empty())
		{
			/* get icon size */
			int icon_w;
			int icon_h;
			
			frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
			
			frameBuffer->paintIcon(iconName, x + BORDER_LEFT + BORDER_LEFT/2, y+ ((height/2- icon_h/2)) );
		}
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + (BORDER_LEFT + BORDER_LEFT/2), y+ height, height, CRCInput::getKeyName(directKey), color, height);
        }

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_option, true); // UTF-8
	int stringstartposName = x + offx + BORDER_LEFT;
	int stringstartposOption = x + dx - stringwidth - BORDER_RIGHT  - BORDER_LEFT/2; //+ offx

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName,   y + height,dx - (stringstartposName - x), optionNameString.c_str(), color, 0, true); // UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + height,dx - (stringstartposOption - x), l_option, color, 0, true); // UTF-8

	// help bar
	if(g_settings.help_bar)
	{
		if (selected)
		{
			// refresh
			frameBuffer->paintBoxRel(x, HEIGHT - 25, dx, 25, COL_MENUHEAD_PLUS_0);
			
			// paint left/right icon
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_LEFT, x + BORDER_LEFT, HEIGHT - 25);
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RIGHT, x + BORDER_LEFT + 16 +5, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT + 16 + 5 + 16 + 5, HEIGHT, dx - (x + BORDER_LEFT + 16 + 5 + 16 + 5 -x), (const char *)"Navi", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			
			// ok icon
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x + dx/2 - 20, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + dx/2 -20 + 24 + 5, HEIGHT, dx - (x + dx/2 -20 + 24 + 5 - x), (const char *)"Enter", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			
			// exit icon
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HOME, x + dx - BORDER_RIGHT - 24 - 30, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + dx - BORDER_RIGHT - 30, HEIGHT, dx -(x + dx - BORDER_RIGHT - 30 - x), (const char *)"Exit", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
		}
	}
	
	// menutitle on VFD
	if(g_settings.menutitle_vfd)
	{
		if (selected)
		{
			char str[256];
			snprintf(str, 255, "%s %s", optionNameString.c_str(), l_option);

			CVFD::getInstance()->showMenuText(0, str, -1, true);
		}
	}

	return y + height;
}

// CMenuOptionStringChooser
CMenuOptionStringChooser::CMenuOptionStringChooser(const neutrino_locale_t OptionName, char* OptionValue, bool Active, CChangeObserver* Observ, const neutrino_msg_t DirectKey, const std::string & IconName, bool Pulldown)
{
	height      = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionName  = OptionName;
	active      = Active;
	optionValue = OptionValue;
	observ      = Observ;

	directKey   = DirectKey;
	iconName    = IconName;
	can_arrow   = true;
	pulldown    = Pulldown;
}

CMenuOptionStringChooser::~CMenuOptionStringChooser()
{
	options.clear();
}

void CMenuOptionStringChooser::addOption(const char * const value)
{
	options.push_back(std::string(value));
}

int CMenuOptionStringChooser::exec(CMenuTarget* parent)
{
	bool wantsRepaint = false;
	int ret = menu_return::RETURN_NONE;

	if (parent)
		parent->hide();

	if( (!parent || msg == CRCInput::RC_ok) && pulldown ) 
	{
		int select = -1;
		char cnt[5];

		CMenuWidget* menu = new CMenuWidget(optionName, NEUTRINO_ICON_SETTINGS);
		//if(parent) menu->move(20, 0);
		CMenuSelectorTarget * selector = new CMenuSelectorTarget(&select);
		
		// intros
		menu->addItem(GenericMenuSeparator);
		
		for(unsigned int count = 0; count < options.size(); count++) 
		{
			bool selected = false;
			if (strcmp(options[count].c_str(), optionValue) == 0)
				selected = true;
			sprintf(cnt, "%d", count);
			menu->addItem(new CMenuForwarderNonLocalized(options[count].c_str(), true, NULL, selector, cnt), selected);
		}
		menu->exec(NULL, "");
		ret = menu_return::RETURN_REPAINT;
		if(select >= 0)
			strcpy(optionValue, options[select].c_str());
		delete menu;
		delete selector;
	} 
	else 
	{
		//select next value
		for(unsigned int count = 0; count < options.size(); count++) 
		{
			if (strcmp(options[count].c_str(), optionValue) == 0) 
			{
				if( msg == CRCInput::RC_left ) 
				{
					if(count > 0)
						strcpy(optionValue, options[(count - 1) % options.size()].c_str());
					else
						strcpy(optionValue, options[options.size() - 1].c_str());
				} 
				else
					strcpy(optionValue, options[(count + 1) % options.size()].c_str());
				wantsRepaint = true;
				break;
			}
		}
	}

	if(parent)
		paint(true);
	
	if(observ) 
	{
		wantsRepaint = observ->changeNotify(optionName, optionValue);
	}
	
	if (wantsRepaint)
		ret = menu_return::RETURN_REPAINT;

	return ret;
}

int CMenuOptionStringChooser::paint( bool selected )
{
	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;
	
	if (selected) 
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	
	if (!active) 
	{
		color   = COL_MENUCONTENTINACTIVE;
		bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}
	
	// paint item
	CFrameBuffer::getInstance()->paintBoxRel(x + BORDER_LEFT, y, dx - (BORDER_LEFT + BORDER_RIGHT), height, bgcolor); //FIXME

	const char * l_optionName = g_Locale->getText(optionName);
	int optionwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_optionName, true);
	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(optionValue, true);
	
	// stringstartposName
	int stringstartposName = x + offx + BORDER_LEFT;

	// stringstartposOption
	int stringstartposOption = x + dx - stringwidth - BORDER_RIGHT  - BORDER_LEFT/2; //+ offx
	
	// recalculate stringstartposOption if stringWidth > dx
	if(stringwidth > dx)
		stringstartposOption = x + offx + BORDER_LEFT + BORDER_RIGHT + optionwidth;

	if (!(iconName.empty()))
	{
		/* get icon size */
		int icon_w;
		int icon_h;
		
		CFrameBuffer::getInstance()->getIconSize(iconName.c_str(), &icon_w, &icon_h);
		
		CFrameBuffer::getInstance()->paintIcon(iconName, x + BORDER_LEFT + BORDER_LEFT/2, y+ ((height/2- icon_h/2)) );	
	}
	else if (CRCInput::isNumeric(directKey))
	{
		/* define icon name depends of numeric value */
		char i_name[6]; /* X +'\0' */
		snprintf(i_name, 6, "%d", CRCInput::getNumericValue(directKey));
		i_name[5] = '\0'; /* even if snprintf truncated the string, ensure termination */
		iconName = i_name;
		
		if (!iconName.empty())
		{
			/* get icon size */
			int icon_w;
			int icon_h;
			
			CFrameBuffer::getInstance()->getIconSize(iconName.c_str(), &icon_w, &icon_h);
			
			CFrameBuffer::getInstance()->paintIcon(iconName, x + BORDER_LEFT + BORDER_LEFT/2, y+ ((height/2- icon_h/2)) );
		}
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + (BORDER_LEFT  + BORDER_LEFT/2), y + height, height, CRCInput::getKeyName(directKey), color, height);
        }

	// option name
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName,   y + height, dx- (stringstartposName - x),  l_optionName, color, 0, true); // UTF-8
	
	// option value
	if(stringwidth > dx)
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + height, dx- BORDER_RIGHT - (stringstartposOption - x), optionValue, color, 0, true);
	else
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + height, dx- (stringstartposOption - x), optionValue, color, 0, true);

	// help bar
	if(g_settings.help_bar)
	{
		if (selected)
		{
			// refresh
			CFrameBuffer::getInstance()->paintBoxRel(x, HEIGHT - 25, dx, 25, COL_MENUHEAD_PLUS_0);
			
			// paint left/right icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_LEFT, x + BORDER_LEFT, HEIGHT - 25);
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_RIGHT, x + BORDER_LEFT + 16 +5, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT + 16 + 5 + 16 + 5, HEIGHT, dx - (x + BORDER_LEFT + 16 + 5 + 16 + 5 -x), (const char *)"Navi", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			
			// ok icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x + dx/2 - 20, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + dx/2 -20 + 24 + 5, HEIGHT, dx - (x + dx/2 -20 + 24 + 5 - x), (const char *)"Enter", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			
			// exit icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_HOME, x + dx - BORDER_RIGHT - 24 - 30, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + dx - BORDER_RIGHT - 30, HEIGHT, dx -(x + dx - BORDER_RIGHT - 30 - x), (const char *)"Exit", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
		}
	}
	
	// menutitle on VFD
	if(g_settings.menutitle_vfd)
	{
		if (selected)
		{
			char str[256];
			snprintf(str, 255, "%s %s", l_optionName, optionValue);

			CVFD::getInstance()->showMenuText(0, str, -1, true);
		}
	}

	return y + height;
}

// CMenuOptionLanguageChooser
CMenuOptionLanguageChooser::CMenuOptionLanguageChooser(char* OptionValue, CChangeObserver* Observ, const char * const IconName)
{
	height      = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	optionValue = OptionValue;
	observ      = Observ;

	directKey   = CRCInput::RC_nokey;
	iconName = IconName ? IconName : "";
}

CMenuOptionLanguageChooser::~CMenuOptionLanguageChooser()
{
	options.clear();
}

void CMenuOptionLanguageChooser::addOption(const char * const value)
{
	options.push_back(std::string(value));
}

int CMenuOptionLanguageChooser::exec(CMenuTarget*)
{
	bool wantsRepaint = false;

	//select value
	for(unsigned int count = 0; count < options.size(); count++)
	{
		if (strcmp(options[count].c_str(), optionValue) == 0)
		{
			strcpy(g_settings.language, options[(count + 1) % options.size()].c_str());
			break;
		}
	}

	paint(true);
	
	if(observ)
	{
		wantsRepaint = observ->changeNotify(LOCALE_LANGUAGESETUP_SELECT, optionValue);
	}
	
	return menu_return::RETURN_EXIT;
	
	if ( wantsRepaint )
		return menu_return::RETURN_REPAINT;
	else
		return menu_return::RETURN_NONE;
}

int CMenuOptionLanguageChooser::paint( bool selected )
{
	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;
	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	
	// paint item
	CFrameBuffer::getInstance()->paintBoxRel(x + BORDER_LEFT, y, dx - (BORDER_LEFT + BORDER_RIGHT), height, bgcolor); //FIXME

	if (!(iconName.empty()))
	{
		/* get icon size */
		int icon_w;
		int icon_h;
		
		CFrameBuffer::getInstance()->getIconSize(iconName.c_str(), &icon_w, &icon_h);
		
		CFrameBuffer::getInstance()->paintIcon(iconName, x + BORDER_LEFT + BORDER_LEFT/2, y+ ((height/2- icon_h/2)) );
	}

	//int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(optionValue);
	int stringstartposOption = x + offx + BORDER_LEFT;
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y+height,dx- (stringstartposOption - x), optionValue, color);

	// help bar
	if(g_settings.help_bar)
	{
		if (selected)
		{
			// refresh
			CFrameBuffer::getInstance()->paintBoxRel(x, HEIGHT - 25, dx, 25, COL_MENUHEAD_PLUS_0);
			
			// paint left/right icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_LEFT, x + BORDER_LEFT, HEIGHT - 25);
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_RIGHT, x + BORDER_LEFT + 16 +5, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT + 16 + 5 + 16 + 5, HEIGHT, dx - (x + BORDER_LEFT + 16 + 5 + 16 + 5 -x), (const char *)"Navi", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			
			// ok icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x + dx/2 - 20, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + dx/2 -20 + 24 + 5, HEIGHT, dx - (x + dx/2 -20 + 24 + 5 - x), (const char *)"Enter", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			
			// exit icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_HOME, x + dx - BORDER_RIGHT - 24 - 30, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + dx - BORDER_RIGHT - 30, HEIGHT, dx -(x + dx - BORDER_RIGHT - 30 - x), (const char *)"Exit", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
		}
	}
	
	// menutitle on VFD
	if(g_settings.menutitle_vfd)
	{
		if (selected)
		{
			CVFD::getInstance()->showMenuText(1, optionValue);
		}
	}

	return y + height;
}

//CMenuForwarder
CMenuForwarder::CMenuForwarder(const neutrino_locale_t Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName)
{
	option = Option;
	option_string = NULL;
	text=Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
}

CMenuForwarder::CMenuForwarder(const neutrino_locale_t Text, const bool Active, const std::string &Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName)
{
	option = NULL;
	option_string = &Option;
	text=Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
}

int CMenuForwarder::getHeight(void) const
{
	return g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

int CMenuForwarder::getWidth(void) const
{
	int tw = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(g_Locale->getText(text), true);
	const char * option_text = NULL;

	if (option)
		option_text = option;
	else if (option_string)
		option_text = option_string->c_str();
	

        if (option_text != NULL)
                tw += BORDER_LEFT + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(option_text, true);

	return tw;
}

int CMenuForwarder::exec(CMenuTarget* parent)
{
	if(jumpTarget)
	{
		return jumpTarget->exec(parent, actionKey);
	}
	else
	{
		return menu_return::RETURN_EXIT;
	}
}

const char * CMenuForwarder::getOption(void)
{
	if (option)
		return option;
	else
		if (option_string)
			return option_string->c_str();
		else
			return NULL;
}

const char * CMenuForwarder::getName(void)
{
	return g_Locale->getText(text);
}

int CMenuForwarder::paint(bool selected)
{
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	int height = getHeight();
	const char * l_text = getName();
	
	int stringstartposX = x + (offx == 0 ? BORDER_LEFT : offx) + BORDER_LEFT;

	const char * option_text = getOption();

	// help bar
	if(g_settings.help_bar)
	{
		if (selected)
		{
			// refresh
			CFrameBuffer::getInstance()->paintBoxRel(x, HEIGHT - 25, dx, 25, COL_MENUHEAD_PLUS_0);
			
			// paint left/right icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_LEFT, x + BORDER_LEFT, HEIGHT - 25);
			//CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_RIGHT, x + BORDER_LEFT + 16 +5, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT + 16 + 5, HEIGHT, dx - (x + BORDER_LEFT + 16 + 5 -x), (const char *)"Back", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			
			// ok icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x + dx/2 - 20, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + dx/2 -20 + 24 + 5, HEIGHT, dx - (x + dx/2 -20 + 24 + 5 - x), (const char *)"Enter", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			
			// exit icon
			CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_HOME, x + dx - BORDER_RIGHT - 24 - 30, HEIGHT - 25);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + dx - BORDER_RIGHT - 30, HEIGHT, dx -(x + dx - BORDER_RIGHT - 30 - x), (const char *)"Exit", COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
		}
	}
	
	// menutitle on VFD
	if(g_settings.menutitle_vfd)
	{
		if (selected)
		{
			char str[256];

			if (option_text != NULL) 
			{
				snprintf(str, 255, "%s %s", l_text, option_text);

				CVFD::getInstance()->showMenuText(0, str, -1, true);
			} 
			else
			{
				CVFD::getInstance()->showMenuText(0, l_text, -1, true);
			}
		}
	}

	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;

	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	else if (!active)
	{
		color   = COL_MENUCONTENTINACTIVE;
		bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}
	
	// paint item
	frameBuffer->paintBoxRel(x + BORDER_LEFT, y, dx -(BORDER_LEFT + BORDER_RIGHT), height, bgcolor); //FIXME

	//local-text
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y + height, dx - BORDER_LEFT - - BORDER_LEFT/2 - (stringstartposX - x), l_text, color, 0, true); // UTF-8
	
	//icons/keys
	if (!iconName.empty())
	{
		//get icon size
		int icon_w = 0;
		int icon_h = 0;
		
		frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
		
		frameBuffer->paintIcon(iconName, x + BORDER_LEFT + BORDER_LEFT/2, y+ ((height/2- icon_h/2)) );
	}
	else if (CRCInput::isNumeric(directKey))
	{
		//define icon name depends of numeric value
		char i_name[6]; /* X +'\0' */
		snprintf(i_name, 6, "%d", CRCInput::getNumericValue(directKey));
		i_name[5] = '\0'; /* even if snprintf truncated the string, ensure termination */
		iconName = i_name;
		
		if (!iconName.empty())
		{
			//get icon size
			int icon_w = 0;
			int icon_h = 0;
			
			frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
			
			frameBuffer->paintIcon(iconName, x + BORDER_LEFT + BORDER_LEFT/2, y+ ((height/2- icon_h/2)) );
		}
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + (BORDER_LEFT + BORDER_LEFT/2), y+ height, height, CRCInput::getKeyName(directKey), color, height);
	}

	//option-text
	if (option_text != NULL)
	{
		int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(option_text, true);
		int stringstartposOption = std::max(stringstartposX + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text, true) + BORDER_LEFT + BORDER_LEFT/2, x + dx - stringwidth - BORDER_RIGHT - BORDER_RIGHT/2); //+ offx
		
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + height, dx - BORDER_LEFT - (stringstartposOption- x),  option_text, color, 0, true);
	}
	
	return y + height;
}

// CMenuForwarderItemMenuIcon
CMenuForwarderItemMenuIcon::CMenuForwarderItemMenuIcon(const neutrino_locale_t Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName, const char * const ItemIcon, const neutrino_locale_t HelpText )
{
	option = Option;
	option_string = NULL;
	text = Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
	itemIcon = ItemIcon ? ItemIcon : "";
	helptext = HelpText;
}

CMenuForwarderItemMenuIcon::CMenuForwarderItemMenuIcon(const neutrino_locale_t Text, const bool Active, const std::string &Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName, const char * const ItemIcon, const neutrino_locale_t HelpText)
{
	option = NULL;
	option_string = &Option;
	text = Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
	itemIcon = ItemIcon ? ItemIcon : "";
	helptext = HelpText;
}

int CMenuForwarderItemMenuIcon::getHeight(void) const
{
	return g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

int CMenuForwarderItemMenuIcon::getWidth(void) const
{
	int tw = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(g_Locale->getText(text), true);
	const char * option_text = NULL;

	if (option)
		option_text = option;
	else if (option_string)
		option_text = option_string->c_str();
	

        if (option_text != NULL)
                tw += BORDER_LEFT + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(option_text, true);

	return tw;
}

int CMenuForwarderItemMenuIcon::exec(CMenuTarget* parent)
{
	if(jumpTarget)
	{
		return jumpTarget->exec(parent, actionKey);
	}
	else
	{
		return menu_return::RETURN_EXIT;
	}
}

const char * CMenuForwarderItemMenuIcon::getOption(void)
{
	if (option)
		return option;
	else
		if (option_string)
			return option_string->c_str();
		else
			return NULL;
}

const char * CMenuForwarderItemMenuIcon::getName(void)
{
	return g_Locale->getText(text);
}

const char * CMenuForwarderItemMenuIcon::getHelpText(void)
{
	return g_Locale->getText(helptext);
}

int CMenuForwarderItemMenuIcon::paint(bool selected)
{
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	int height = getHeight();
	const char * l_text = getName();
	
	int stringstartposX = x + (offx == 0? BORDER_LEFT : offx) + BORDER_LEFT;

	const char * option_text = getOption();	

	unsigned char color   = COL_MENUCONTENT;
	fb_pixel_t    bgcolor = COL_MENUCONTENT_PLUS_0;

	if (selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	else if (!active)
	{
		color   = COL_MENUCONTENTINACTIVE;
		bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}
	
	//show ItemMenuIcon
	if (selected)
	{
		if (!itemIcon.empty())
		{
			/*
			* CMenuWidget menues are always centered
			*/
			/* get icon size */
			int icon_w = 128;
			int icon_h = 128;
			
			int hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
			
			//refresh pic box
			frameBuffer->paintBoxRel(x + BORDER_LEFT + (dx/3)*2 + (((dx - (dx/3)*2 - 10)/2) - icon_w/2), ( frameBuffer->getScreenHeight(true) - icon_h + hheight + 25 )/2, icon_w, icon_h, COL_MENUCONTENTDARK_PLUS_0 ); // 25 foot height
		
			// paint item icon
			frameBuffer->paintIcon(itemIcon, x + BORDER_LEFT + (dx/3)*2 + ((( dx - (dx/3)*2 - BORDER_RIGHT )/2) - icon_w/2), ( frameBuffer->getScreenHeight(true) - icon_h + hheight + 25 )/2);  //25:foot height
		}
		
		// help bar
		if(g_settings.help_txt)
		{
			//help text
			// refresh
			frameBuffer->paintBoxRel(x, HEIGHT - 25, dx, 25, COL_MENUHEAD_PLUS_0);
			
			// paint help icon
			frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + BORDER_LEFT - 2, HEIGHT - 25);
			
			// help text
			const char * help_text = getHelpText();
			
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(stringstartposX, HEIGHT, dx - (stringstartposX - x), help_text, COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
		}
	}
	
	// menutitle on VFD
	if(g_settings.menutitle_vfd)
	{
		if (selected)
		{
			char str[256];

			if (option_text != NULL) 
			{
				snprintf(str, 255, "%s %s", l_text, option_text);

				CVFD::getInstance()->showMenuText(0, str, -1, true);
			} 
			else
			{
				CVFD::getInstance()->showMenuText(0, l_text, -1, true);
			}
		}
	}
	
	// paint item
	frameBuffer->paintBoxRel(x + BORDER_LEFT, y, (dx/3)*2, height, bgcolor);

	//local-text
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y + height, (dx/3)*2 - (stringstartposX - x), l_text, color, 0, true); // UTF-8

	// icon/direkt-key	
	if (!iconName.empty())
	{
		// get icon size
		int icon_w = 0;
		int icon_h = 0;
		
		frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
		
		frameBuffer->paintIcon(iconName, x + BORDER_LEFT + BORDER_LEFT/2, y+ ((height/2- icon_h/2)) );
	}
	else if (CRCInput::isNumeric(directKey))
	{
		// define icon name depends of numeric value
		char i_name[6]; 							// X +'\0'
		snprintf(i_name, 6, "%d", CRCInput::getNumericValue(directKey));
		i_name[5] = '\0'; 							// even if snprintf truncated the string, ensure termination
		iconName = i_name;
		
		if (!iconName.empty())
		{
			// get icon size
			int icon_w = 0;
			int icon_h = 0;
			
			frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
			
			frameBuffer->paintIcon(iconName, x + BORDER_LEFT + BORDER_LEFT/2, y+ ((height/2- icon_h/2)) );
		}
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + (BORDER_LEFT + BORDER_LEFT/2), y + height, height, CRCInput::getKeyName(directKey), color, height);
	}
	
	//option-text
	if (option_text != NULL)
	{
		int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(option_text, true);
		int stringstartposOption = std::max(stringstartposX + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text, true) + BORDER_LEFT + BORDER_LEFT/2, x + (dx/3)*2 - stringwidth - BORDER_RIGHT ); //+ offx
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + height, (dx/3)*2 - (stringstartposOption- x),  option_text, color, 0, true);
	}

	return y + height;
}

// CMenuForwarderNonLocalized 
const char * CMenuForwarderNonLocalized::getName(void)
{
	return the_text.c_str();
}

CMenuForwarderNonLocalized::CMenuForwarderNonLocalized(const char * const Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName) : CMenuForwarder(NONEXISTANT_LOCALE, Active, Option, Target, ActionKey, DirectKey, IconName)
{
	the_text = Text;
}

CMenuForwarderNonLocalized::CMenuForwarderNonLocalized(const char * const Text, const bool Active, const std::string &Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName) : CMenuForwarder(NONEXISTANT_LOCALE, Active, Option, Target, ActionKey, DirectKey, IconName)
{
	the_text = Text;
}

// CMenuForwarderNonLocalizedItemMenuIcon
const char * CMenuForwarderNonLocalizedItemMenuIcon::getName(void)
{
	return the_text.c_str();
}

CMenuForwarderNonLocalizedItemMenuIcon::CMenuForwarderNonLocalizedItemMenuIcon(const char * const Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName, const char * const ItemIcon) : CMenuForwarderItemMenuIcon(NONEXISTANT_LOCALE, Active, Option, Target, ActionKey, DirectKey, IconName, ItemIcon)
{
	the_text = Text;
}

CMenuForwarderNonLocalizedItemMenuIcon::CMenuForwarderNonLocalizedItemMenuIcon(const char * const Text, const bool Active, const std::string &Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName, const char * const ItemIcon) : CMenuForwarderItemMenuIcon(NONEXISTANT_LOCALE, Active, Option, Target, ActionKey, DirectKey, IconName, ItemIcon)
{
	the_text = Text;
}

/* CMenuSeparator */
CMenuSeparator::CMenuSeparator(const int Type, const neutrino_locale_t Text)
{
	directKey = CRCInput::RC_nokey;
	iconName = "";
	type     = Type;
	text     = Text;
}


int CMenuSeparator::getHeight(void) const
{
	return (text == NONEXISTANT_LOCALE) ? BORDER_LEFT : g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

int CMenuSeparator::getWidth(void) const
{
	return 0;
}

const char * CMenuSeparator::getString(void)
{
	return g_Locale->getText(text);
}

int CMenuSeparator::paint(bool selected)
{
	int height;
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	height = getHeight();

	frameBuffer->paintBoxRel(x + BORDER_LEFT, y, dx - BORDER_LEFT - BORDER_RIGHT, height, COL_MENUCONTENTDARK_PLUS_0 );

	if ((type & LINE))
	{
		frameBuffer->paintBoxRel(x + BORDER_LEFT, y, dx - BORDER_LEFT - BORDER_RIGHT, height, COL_MENUCONTENT_PLUS_0);
		
		frameBuffer->paintHLineRel(x + BORDER_LEFT, dx - BORDER_LEFT - BORDER_RIGHT, y + (height >> 1), COL_MENUCONTENTDARK_PLUS_0 );
		frameBuffer->paintHLineRel(x + BORDER_LEFT, dx - BORDER_LEFT - BORDER_RIGHT, y + (height >> 1) + 1, COL_MENUCONTENTDARK_PLUS_0 );
	}

	if ((type & STRING))
	{

		if (text != NONEXISTANT_LOCALE)
		{
			int stringstartposX;

			const char * l_text = getString();
			int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text, true); // UTF-8

			/* if no alignment is specified, align centered */
			if (type & ALIGN_LEFT)
				stringstartposX = x + (BORDER_LEFT + 10);
			else if (type & ALIGN_RIGHT)
				stringstartposX = x + dx - stringwidth - (BORDER_RIGHT + 10 );
			else /* ALIGN_CENTER */
				stringstartposX = x + (dx >> 1) - (stringwidth >> 1);

			frameBuffer->paintBoxRel(stringstartposX - 5, y, stringwidth + 10, height, COL_MENUCONTENT_PLUS_0);

			g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y+height,dx- (stringstartposX- x) , l_text, COL_MENUCONTENTINACTIVE, 0, true); // UTF-8

			// help bar
			if(g_settings.help_bar)
			{
				if (selected)
				{
					// refresh
					frameBuffer->paintBoxRel(x, HEIGHT - 25, dx, 25, COL_MENUHEAD_PLUS_0);
				}
			}
		}
	}

	return y+ height;
}

// CMenuSeparatorMainMenu
CMenuSeparatorItemMenuIcon::CMenuSeparatorItemMenuIcon(const int Type, const neutrino_locale_t Text)
{
	directKey = CRCInput::RC_nokey;
	iconName = "";
	type     = Type;
	text     = Text;
}


int CMenuSeparatorItemMenuIcon::getHeight(void) const
{
	return (text == NONEXISTANT_LOCALE) ? BORDER_LEFT : g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

int CMenuSeparatorItemMenuIcon::getWidth(void) const
{
	return 0;
}

const char * CMenuSeparatorItemMenuIcon::getString(void)
{
	return g_Locale->getText(text);
}

int CMenuSeparatorItemMenuIcon::paint(bool selected)
{
	int height;
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	height = getHeight();

	frameBuffer->paintBoxRel(x + BORDER_LEFT, y, (dx/3)*2, height, COL_MENUCONTENTDARK_PLUS_0 );

	if ((type & LINE))
	{
		//repaint thr dark box 
		frameBuffer->paintBoxRel(x + BORDER_LEFT, y, (dx/3)*2, height, COL_MENUCONTENT_PLUS_0 );
		
		frameBuffer->paintHLineRel(x + BORDER_LEFT, (dx/3)*2, y + (height >> 1), COL_MENUCONTENTDARK_PLUS_0);
		frameBuffer->paintHLineRel(x + BORDER_LEFT, (dx/3)*2, y + (height >> 1) + 1, COL_MENUCONTENTDARK_PLUS_0);
	}

	if ((type & STRING))
	{

		if (text != NONEXISTANT_LOCALE)
		{
			int stringstartposX;

			const char * l_text = getString();
			int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text, true); // UTF-8

			/* if no alignment is specified, align centered */
			if (type & ALIGN_LEFT)
				stringstartposX = x + (BORDER_LEFT + 10) ;
			else if (type & ALIGN_RIGHT)
				stringstartposX = x + (dx/3)*2 - stringwidth - (BORDER_RIGHT + 10);
			else /* ALIGN_CENTER */
				stringstartposX = x + ( ((dx/3)*2) >> 1) - (stringwidth >> 1);

			frameBuffer->paintBoxRel(stringstartposX - 5, y, stringwidth + 10, height, COL_MENUCONTENT_PLUS_0);

			g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y + height, (dx/3)*2 - (stringstartposX- x) , l_text, COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
		}
	}

	return y + height;
}

bool CPINProtection::check()
{
	char cPIN[4];
	neutrino_locale_t hint = NONEXISTANT_LOCALE;
	do
	{
		cPIN[0] = 0;
		CPINInput* PINInput = new CPINInput(LOCALE_PINPROTECTION_HEAD, cPIN, 4, hint);
		PINInput->exec( getParent(), "");
		delete PINInput;
		hint = LOCALE_PINPROTECTION_WRONGCODE;
	} while ((strncmp(cPIN,validPIN,4) != 0) && (cPIN[0] != 0));
	
	return ( strncmp(cPIN,validPIN,4) == 0);
}

bool CZapProtection::check()
{
	int res;
	char cPIN[5];
	neutrino_locale_t hint2 = NONEXISTANT_LOCALE;
	do
	{
		cPIN[0] = 0;

		CPLPINInput* PINInput = new CPLPINInput(LOCALE_PARENTALLOCK_HEAD, cPIN, 4, hint2, fsk);

		res = PINInput->exec(getParent(), "");
		delete PINInput;

		hint2 = LOCALE_PINPROTECTION_WRONGCODE;
	} while ( (strncmp(cPIN,validPIN,4) != 0) &&
		  (cPIN[0] != 0) &&
		  ( res == menu_return::RETURN_REPAINT ) &&
		  ( fsk >= g_settings.parentallock_lockage ) );
	return ( ( strncmp(cPIN,validPIN,4) == 0 ) ||
			 ( fsk < g_settings.parentallock_lockage ) );
}

int CLockedMenuForwarder::exec(CMenuTarget* parent)
{
	Parent = parent;
	
	if( (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_NEVER) || AlwaysAsk )
		if (!check())
		{
			Parent = NULL;
			return menu_return::RETURN_REPAINT;
		}

	Parent = NULL;
	return CMenuForwarder::exec(parent);
}

int CLockedMenuForwarderItemMenuIcon::exec(CMenuTarget* parent)
{
	Parent = parent;
	if( (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_NEVER) || AlwaysAsk )
	{
		if (!check())
		{
			Parent = NULL;
			return menu_return::RETURN_REPAINT;
		}
	}

	Parent = NULL;
	return CMenuForwarderItemMenuIcon::exec(parent);
}

int CMenuSelectorTarget::exec(CMenuTarget* parent, const std::string & actionKey)
{
        if (actionKey != "")
                *m_select = atoi(actionKey.c_str());
        else
                *m_select = -1;
	
        return menu_return::RETURN_EXIT;
}

