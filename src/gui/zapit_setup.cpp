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

#include <zapit/bouquets.h>
#include "gui/zapit_setup.h"

#include <global.h>
#include <neutrino.h>

#include <driver/screen_max.h>


extern Zapit_config zapitCfg;	//defined in neutrino.cpp
void setZapitConfig(Zapit_config * Cfg);
void getZapitConfig(Zapit_config *Cfg);

//option off0_on1
#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, LOCALE_OPTIONS_OFF },
        { 1, LOCALE_OPTIONS_ON  }
};

/* option off1 on0*/
#define OPTIONS_OFF1_ON0_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_OFF1_ON0_OPTIONS[OPTIONS_OFF1_ON0_OPTION_COUNT] =
{
        { 1, LOCALE_OPTIONS_OFF },
        { 0, LOCALE_OPTIONS_ON  }
};

//option off0_on1
#define OPTIONS_LASTMODE_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_LASTMODE_OPTIONS[OPTIONS_LASTMODE_OPTION_COUNT] =
{
        { 0, NONEXISTANT_LOCALE, "Radio" },
        { 1, NONEXISTANT_LOCALE, "TV"  }
};

#define SECTIONSD_SCAN_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval SECTIONSD_SCAN_OPTIONS[SECTIONSD_SCAN_OPTIONS_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF },
	{ 1, LOCALE_OPTIONS_ON  },
	{ 2, LOCALE_OPTIONS_ON_WITHOUT_MESSAGES  }
};

CZapitSetup::CZapitSetup()
{
	//width = w_max (500, 100);
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
		setZapitConfig(&zapitCfg);
		
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
	CMenuWidget *zapit = new CMenuWidget(LOCALE_MISCSETTINGS_ZAPIT, NEUTRINO_ICON_SETTINGS);
	zapit->setSelected(selected);
	
	// intros
	zapit->addItem(GenericMenuBack);
	zapit->addItem(GenericMenuSeparatorLine);
	
	// save settings
	zapit->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "save_action", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	zapit->addItem(GenericMenuSeparatorLine);
	
	int shortcut = 1;

	//zapit
	zapit->addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_ZAPIT, &g_settings.uselastchannel, OPTIONS_OFF1_ON0_OPTIONS, OPTIONS_OFF1_ON0_OPTION_COUNT, true, this, CRCInput::convertDigitToKey(shortcut++) ));
	zapit->addItem(zapit1 = new CMenuOptionChooser(LOCALE_ZAPITSETUP_LAST_MODE, &g_settings.lastChannelMode, OPTIONS_LASTMODE_OPTIONS, OPTIONS_LASTMODE_OPTION_COUNT, !g_settings.uselastchannel, this, CRCInput::convertDigitToKey(shortcut++) ));
	zapit->addItem(zapit2 = new CMenuForwarder(LOCALE_ZAPITSETUP_LAST_TV, !g_settings.uselastchannel, g_settings.StartChannelTV, new CSelectChannelWidget(), "tv", CRCInput::convertDigitToKey(shortcut++) ));
	zapit->addItem(zapit3 = new CMenuForwarder(LOCALE_ZAPITSETUP_LAST_RADIO, !g_settings.uselastchannel, g_settings.StartChannelRadio, new CSelectChannelWidget(), "radio", CRCInput::convertDigitToKey(shortcut++) ));
	
	getZapitConfig(&zapitCfg);
	
	zapit->addItem(GenericMenuSeparatorLine);
	zapit->addItem(new CMenuOptionChooser(LOCALE_EXTRA_ZAPIT_MAKE_BOUQUET, (int *)&zapitCfg.makeRemainingChannelsBouquet, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	//zapit->addItem(new CMenuOptionChooser(LOCALE_EXTRA_ZAPIT_SAVE_LAST, (int *)&zapitCfg.saveLastChannel, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	zapit->addItem(new CMenuOptionChooser(LOCALE_EXTRA_ZAPIT_WRITE_NAMES, (int *)&zapitCfg.writeChannelsNames, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	zapit->addItem(new CMenuOptionChooser(LOCALE_EXTRA_ZAPIT_SORTNAMES,  (int *)&zapitCfg.sortNames, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	zapit->addItem( new CMenuOptionChooser(LOCALE_ZAPIT_SCANSDT, (int *)&zapitCfg.scanSDT, SECTIONSD_SCAN_OPTIONS, SECTIONSD_SCAN_OPTIONS_COUNT, true));

	zapit->addItem(new CMenuOptionChooser(LOCALE_EXTRA_ZAPIT_SCANPIDS,  (int *)&zapitCfg.scanPids, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

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
		zapit2->setActive(!g_settings.uselastchannel);
		zapit3->setActive(!g_settings.uselastchannel);
	}

	return true;
}

//select menu
CSelectChannelWidget::CSelectChannelWidget()
{
	//width = w_max (500, 100);
}

CSelectChannelWidget::~CSelectChannelWidget()
{

}

int CSelectChannelWidget::exec(CMenuTarget* parent, const std::string& actionKey)
{
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

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
	else if (strncmp(actionKey.c_str(), "ZCT:", 4) == 0 || strncmp(actionKey.c_str(), "ZCR:", 4) == 0)
	{
		unsigned int cnr = 0;
		t_channel_id channel_id = 0;
		sscanf(&(actionKey[4]),"%u|%llx", &cnr, &channel_id);

		if (strncmp(actionKey.c_str(), "ZCT:", 4) == 0)//...tv
		{
			g_settings.StartChannelTV = actionKey.substr(actionKey.find_first_of("#")+1);
			g_settings.startchanneltv_id = channel_id;
			g_settings.startchanneltv_nr = cnr-1;
		}
		else if (strncmp(actionKey.c_str(), "ZCR:", 4) == 0)//...radio
		{
			g_settings.StartChannelRadio = actionKey.substr(actionKey.find_first_of("#")+1);
			g_settings.startchannelradio_id= channel_id;
			g_settings.startchannelradio_nr = cnr-1;
		}

		// ...leave bouquet/channel menu and show a refreshed zapit menu with current start channel(s)
		g_RCInput->postMsg(CRCInput::RC_timeout, 0);
		return menu_return::RETURN_EXIT;
	}

	return res;
}

extern CBouquetManager *g_bouquetManager;
void CSelectChannelWidget::InitZapitChannelHelper(CZapitClient::channelsMode mode)
{
	std::vector<CMenuWidget *> toDelete;
	CMenuWidget mctv(LOCALE_TIMERLIST_BOUQUETSELECT, NEUTRINO_ICON_SETTINGS);
	//mctv.addIntroItems();

	for (int i = 0; i < (int) g_bouquetManager->Bouquets.size(); i++) 
	{
		CMenuWidget* mwtv = new CMenuWidget(LOCALE_TIMERLIST_CHANNELSELECT, NEUTRINO_ICON_SETTINGS);
		toDelete.push_back(mwtv);
		
		ZapitChannelList channels = (mode == CZapitClient::MODE_RADIO) ? g_bouquetManager->Bouquets[i]->radioChannels : g_bouquetManager->Bouquets[i]->tvChannels;
		
		for(int j = 0; j < (int) channels.size(); j++) 
		{
			CZapitChannel * channel = channels[j];
			char cChannelId[60] = {0};
			snprintf(cChannelId,sizeof(cChannelId),"ZC%c:%d|%llx#",(mode==CZapitClient::MODE_TV)?'T':'R',channel->number, channel->channel_id);

			CMenuForwarderNonLocalized * chan_item = new CMenuForwarderNonLocalized(channel->getName().c_str(), true, NULL, this, (std::string(cChannelId) + channel->getName()).c_str(), CRCInput::RC_nokey, channel->scrambled ?NEUTRINO_ICON_SCRAMBLED:NULL );
			mwtv->addItem(chan_item);
		}
		
		if(!channels.empty() && (!g_bouquetManager->Bouquets[i]->bHidden ))
		{
			mctv.addItem(new CMenuForwarderNonLocalized(g_bouquetManager->Bouquets[i]->Name.c_str(), true, NULL, mwtv));
		}
	}
	mctv.exec (NULL, "");
	mctv.hide ();

	// delete dynamic created objects
	for(unsigned int count = 0; count < toDelete.size(); count++)
	{
		delete toDelete[count];
	}
	toDelete.clear();
}
