/*
	zapit_setup settings menu - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
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

/*zapit includes*/
#include <bouquets.h>

#include "gui/zapit_setup.h"

#include <global.h>
#include <neutrino.h>

#include <driver/screen_max.h>
#include "gui/bouquetlist.h"


extern CBouquetList * bouquetList;
//extern Zapit_config zapitCfg;	//defined in neutrino.cpp
//void setZapitConfig(Zapit_config * Cfg);
//void getZapitConfig(Zapit_config *Cfg);

//option off0_on1
#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, LOCALE_OPTIONS_OFF, NULL },
        { 1, LOCALE_OPTIONS_ON, NULL  }
};

/* option off1 on0*/
#define OPTIONS_OFF1_ON0_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_OFF1_ON0_OPTIONS[OPTIONS_OFF1_ON0_OPTION_COUNT] =
{
        { 1, LOCALE_OPTIONS_OFF, NULL },
        { 0, LOCALE_OPTIONS_ON, NULL  }
};

//option off0_on1
#define OPTIONS_LASTMODE_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_LASTMODE_OPTIONS[OPTIONS_LASTMODE_OPTION_COUNT] =
{
        { 0, NONEXISTANT_LOCALE, "Radio" },
        { 1, NONEXISTANT_LOCALE, "TV"  }
};

CZapitSetup::CZapitSetup()
{
	selected = -1;
}

CZapitSetup::~CZapitSetup()
{

}

int CZapitSetup::exec(CMenuTarget * parent, const std::string &actionKey)
{
	printf("[neutrino] init zapit menu setup...\n");
	int   res = menu_return::RETURN_REPAINT;
	
	if(actionKey == "save_action") 
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		showMenu();
		return menu_return::RETURN_EXIT;
	}
	
	if (parent)
		parent->hide();

	showMenu();

	return res;
}

void CZapitSetup::showMenu()
{
	//menue init
	CMenuWidget * zapit = new CMenuWidget(LOCALE_MISCSETTINGS_ZAPIT, NEUTRINO_ICON_SETTINGS);
	zapit->setSelected(selected);
	
	// intros
	zapit->addItem(GenericMenuBack);
	zapit->addItem(GenericMenuSeparatorLine);
	
	// save settings
	zapit->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "save_action", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	zapit->addItem(GenericMenuSeparatorLine);
	
	int shortcut = 1;
	
	int mode = CNeutrinoApp::getInstance()->getMode();

	//zapit
	zapit->addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_ZAPIT, &g_settings.uselastchannel, OPTIONS_OFF1_ON0_OPTIONS, OPTIONS_OFF1_ON0_OPTION_COUNT, true, this, CRCInput::convertDigitToKey(shortcut++) ));
	zapit->addItem(zapit1 = new CMenuOptionChooser(LOCALE_ZAPITSETUP_LAST_MODE, &g_settings.lastChannelMode, OPTIONS_LASTMODE_OPTIONS, OPTIONS_LASTMODE_OPTION_COUNT, !g_settings.uselastchannel, this, CRCInput::convertDigitToKey(shortcut++) ));
	zapit->addItem(zapit2 = new CMenuForwarder(LOCALE_ZAPITSETUP_LAST_TV, !g_settings.uselastchannel && (mode == NeutrinoMessages::mode_tv), g_settings.StartChannelTV, new CSelectChannelWidget(), "tv", CRCInput::convertDigitToKey(shortcut++) ));
	zapit->addItem(zapit3 = new CMenuForwarder(LOCALE_ZAPITSETUP_LAST_RADIO, !g_settings.uselastchannel && (mode == NeutrinoMessages::mode_radio), g_settings.StartChannelRadio, new CSelectChannelWidget(), "radio", CRCInput::convertDigitToKey(shortcut++) ));

	zapit->exec(NULL, "");
	zapit->hide();
	selected = zapit->getSelected();
	
	delete zapit;
}

bool CZapitSetup::changeNotify(const neutrino_locale_t OptionName, void *)
{
	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_MISCSETTINGS_ZAPIT))
	{
		zapit1->setActive(!g_settings.uselastchannel);
		zapit2->setActive(!g_settings.uselastchannel && !(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio) );
		zapit3->setActive(!g_settings.uselastchannel && !(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_tv) );
	}

	return true;
}

//select menu
CSelectChannelWidget::CSelectChannelWidget()
{

}

CSelectChannelWidget::~CSelectChannelWidget()
{

}

int CSelectChannelWidget::exec(CMenuTarget * parent, const std::string& actionKey)
{
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
		parent->hide();

	if(actionKey == "tv")
	{
		InitZapitChannelHelper(CZapitClient::MODE_TV);
		return res;
	}
	else if(actionKey == "radio")
	{
		InitZapitChannelHelper(CZapitClient::MODE_RADIO);
		return res;
	}

	return res;
}

void CSelectChannelWidget::InitZapitChannelHelper(CZapitClient::channelsMode mode)
{
	int nNewChannel;
	int nNewBouquet;
	
	nNewBouquet = bouquetList->show();
			
	if (nNewBouquet > -1)
	{
		nNewChannel = bouquetList->Bouquets[nNewBouquet]->channelList->show();
		
		//printf("nNewChannel %d\n",nNewChannel);
		
		if (nNewChannel > -1)
		{
			if(mode == CZapitClient::MODE_TV)
			{
				g_settings.startchanneltv_id = bouquetList->Bouquets[nNewBouquet]->channelList->getActiveChannel_ChannelID();
				g_settings.StartChannelTV = g_Zapit->getChannelName(g_settings.startchanneltv_id);
			}
			else if (mode == CZapitClient::MODE_RADIO)
			{
				g_settings.startchannelradio_id = bouquetList->Bouquets[nNewBouquet]->channelList->getActiveChannel_ChannelID();
				g_settings.StartChannelRadio = g_Zapit->getChannelName(g_settings.startchannelradio_id);
			}
		}
	}
}
