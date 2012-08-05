/*
	$Id: scan_setup.cpp,v 1.8 2011/10/11 15:26:38 mohousch Exp $

	Neutrino-GUI  -   DBoxII-Project

	scan setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

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

#include "gui/scan_setup.h"

#include <global.h>
#include <neutrino.h>

#include "gui/scan.h"
#include "gui/motorcontrol.h"

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/hintbox.h>

#include "gui/widget/stringinput.h"
#include "gui/widget/stringinput_ext.h"

#include <driver/screen_max.h>

#include <system/debug.h>

#include <global.h>

#include <zapit/frontend_c.h>
#include <zapit/getservices.h>
#include <zapit/satconfig.h>

#define scanSettings CNeutrinoApp::getInstance()->getScanSettings() 	//(CNeutrinoApp::getInstance()->ScanSettings())


extern CZapitClient::SatelliteList satList;	//defined neutrino.cpp
extern Zapit_config zapitCfg;			//defined neutrino.cpp
extern char zapit_lat[20];			//defined neutrino.cpp
extern char zapit_long[20];			//defined neutrino.cpp
extern int scan_pids;

extern int FrontendCount;			// defined in zapit.cpp
int tuner_to_scan = 0;

CScanSetup::CScanSetup(int num)
{
	frameBuffer = CFrameBuffer::getInstance();

	width = w_max (500, 100);
	
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	
	height = hheight+13*mheight+ 10;
	
	x = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth()-width) >> 1);
	y = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight()-height) >> 1);
	
	feindex = num;
	
	//FIXME
	tuner_to_scan = num;
}

CScanSetup::~CScanSetup()
{
  
}

void setZapitConfig(Zapit_config * Cfg);
int CScanSetup::exec(CMenuTarget * parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "CScanSetup::exec: init scan service\n");
	int   res = menu_return::RETURN_REPAINT;

	if(actionKey == "save_scansettings") 
	{
		//CNeutrinoApp::getInstance()->exec(NULL, "savescansettings");
		//return res;
		
		// hint box
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MAINSETTINGS_SAVESETTINGSNOW_HINT)); // UTF-8
		hintBox->paint();
		
		// save scan.conf
		if(!scanSettings.saveSettings(NEUTRINO_SCAN_SETTINGS_FILE, feindex)) 
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: error while saving scan-settings!\n");
		
		if( CFrontend::getInstance(feindex)->getInfo()->type == FE_QPSK )
		{
			SaveMotorPositions();
			
			//diseqc type
			g_Zapit->setDiseqcType((diseqc_t)scanSettings.diseqcMode, feindex);
			
			// diseqc repeat
			g_Zapit->setDiseqcRepeat(scanSettings.diseqcRepeat, feindex);
		
			//gotoxx
			zapitCfg.gotoXXLatitude = strtod(zapit_lat, NULL);
			zapitCfg.gotoXXLongitude = strtod(zapit_long, NULL);
			
			setZapitConfig(&zapitCfg);
		}
		
		hintBox->hide();
		delete hintBox;
		
		return res;
	}
	
	if (parent)
	{
		parent->hide();
	}

	showScanService();
	
	return res;
}

void CScanSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height);
#ifdef FB_BLIT
	frameBuffer->blit();
#endif
}

// option off0_on1
#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, LOCALE_OPTIONS_OFF },
        { 1, LOCALE_OPTIONS_ON  }
};

// option off1 on0
#define OPTIONS_OFF1_ON0_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_OFF1_ON0_OPTIONS[OPTIONS_OFF1_ON0_OPTION_COUNT] =
{
        { 1, LOCALE_OPTIONS_OFF },
        { 0, LOCALE_OPTIONS_ON  }
};

#define SCANTS_BOUQUET_OPTION_COUNT 2
const CMenuOptionChooser::keyval SCANTS_BOUQUET_OPTIONS[SCANTS_BOUQUET_OPTION_COUNT] =
{
	//{ CZapitClient::BM_DELETEBOUQUETS        , LOCALE_SCANTS_BOUQUET_ERASE     },
	{ CZapitClient::BM_DONTTOUCHBOUQUETS     , LOCALE_SCANTS_BOUQUET_LEAVE     },
	{ CZapitClient::BM_UPDATEBOUQUETS        , LOCALE_SCANTS_BOUQUET_UPDATE    }
};

#define SCANTS_ZAPIT_SCANTYPE_COUNT 4
const CMenuOptionChooser::keyval SCANTS_ZAPIT_SCANTYPE[SCANTS_ZAPIT_SCANTYPE_COUNT] =
{
	{  CZapitClient::ST_TVRADIO	, LOCALE_ZAPIT_SCANTYPE_TVRADIO },
	{  CZapitClient::ST_TV		, LOCALE_ZAPIT_SCANTYPE_TV },
	{  CZapitClient::ST_RADIO	, LOCALE_ZAPIT_SCANTYPE_RADIO },
	{  CZapitClient::ST_ALL		, LOCALE_ZAPIT_SCANTYPE_ALL }
};

#define SATSETUP_DISEQC_OPTION_COUNT 6
const CMenuOptionChooser::keyval SATSETUP_DISEQC_OPTIONS[SATSETUP_DISEQC_OPTION_COUNT] =
{
	{ NO_DISEQC          , LOCALE_SATSETUP_NODISEQC,	NULL },
	{ MINI_DISEQC        , LOCALE_SATSETUP_MINIDISEQC,	NULL },
	{ DISEQC_1_0         , LOCALE_SATSETUP_DISEQC10,	NULL },
	{ DISEQC_1_1         , LOCALE_SATSETUP_DISEQC11,	NULL },
	{ DISEQC_ADVANCED    , LOCALE_SATSETUP_DISEQ_ADVANCED,	NULL },
	{ SMATV_REMOTE_TUNING, LOCALE_SATSETUP_SMATVREMOTE,	NULL }
};

#define SATSETUP_SCANTP_FEC_COUNT 23
#define CABLESETUP_SCANTP_FEC_COUNT 5
const CMenuOptionChooser::keyval SATSETUP_SCANTP_FEC[SATSETUP_SCANTP_FEC_COUNT] =
{
        { FEC_1_2, LOCALE_SCANTP_FEC_1_2 },
        { FEC_2_3, LOCALE_SCANTP_FEC_2_3 },
        { FEC_3_4, LOCALE_SCANTP_FEC_3_4 },
        { FEC_5_6, LOCALE_SCANTP_FEC_5_6 },
        { FEC_7_8, LOCALE_SCANTP_FEC_7_8 },

        { FEC_S2_QPSK_1_2, LOCALE_FEC_S2_QPSK_1_2 },
        { FEC_S2_QPSK_2_3, LOCALE_FEC_S2_QPSK_2_3 },
        { FEC_S2_QPSK_3_4, LOCALE_FEC_S2_QPSK_3_4 },
        { FEC_S2_QPSK_5_6, LOCALE_FEC_S2_QPSK_5_6 },
        { FEC_S2_QPSK_7_8, LOCALE_FEC_S2_QPSK_7_8 },
        { FEC_S2_QPSK_8_9, LOCALE_FEC_S2_QPSK_8_9 },
        { FEC_S2_QPSK_3_5, LOCALE_FEC_S2_QPSK_3_5 },
        { FEC_S2_QPSK_4_5, LOCALE_FEC_S2_QPSK_4_5 },
        { FEC_S2_QPSK_9_10, LOCALE_FEC_S2_QPSK_9_10 },

        { FEC_S2_8PSK_1_2, LOCALE_FEC_S2_8PSK_1_2 },
        { FEC_S2_8PSK_2_3, LOCALE_FEC_S2_8PSK_2_3 },
        { FEC_S2_8PSK_3_4, LOCALE_FEC_S2_8PSK_3_4 },
        { FEC_S2_8PSK_5_6, LOCALE_FEC_S2_8PSK_5_6 },
        { FEC_S2_8PSK_7_8, LOCALE_FEC_S2_8PSK_7_8 },
        { FEC_S2_8PSK_8_9, LOCALE_FEC_S2_8PSK_8_9 },
        { FEC_S2_8PSK_3_5, LOCALE_FEC_S2_8PSK_3_5 },
        { FEC_S2_8PSK_4_5, LOCALE_FEC_S2_8PSK_4_5 },
        { FEC_S2_8PSK_9_10, LOCALE_FEC_S2_8PSK_9_10 }
};

#define CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT 6
const CMenuOptionChooser::keyval CABLETERRESTRIALSETUP_SCANTP_MOD[CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT] =
{
	// cable
	{ QAM_16, LOCALE_SCANTP_MOD_16 },
	{ QAM_32, LOCALE_SCANTP_MOD_32 },
	{ QAM_64, LOCALE_SCANTP_MOD_64 },
	{ QAM_128, LOCALE_SCANTP_MOD_128 },
	{ QAM_256, LOCALE_SCANTP_MOD_256 },
	
	{ QAM_AUTO, NONEXISTANT_LOCALE, "QAM_AUTO" }
};

#define SATSETUP_SCANTP_MOD_COUNT 2
const CMenuOptionChooser::keyval SATSETUP_SCANTP_MOD[SATSETUP_SCANTP_MOD_COUNT] =
{
	// sat
	{ QPSK, NONEXISTANT_LOCALE, "QPSK" },
	{ PSK_8, NONEXISTANT_LOCALE, "PSK_8" }
};

#define SATSETUP_SCANTP_BAND_COUNT 4
const CMenuOptionChooser::keyval SATSETUP_SCANTP_BAND[SATSETUP_SCANTP_BAND_COUNT] =
{
	{ 0, NONEXISTANT_LOCALE, "BAND_8" },
	{ 1, NONEXISTANT_LOCALE, "BAND_7" },
	{ 2, NONEXISTANT_LOCALE, "BAND_6" },
	{ 3, NONEXISTANT_LOCALE, "BAND_AUTO"}
};

#define SATSETUP_SCANTP_POL_COUNT 2
const CMenuOptionChooser::keyval SATSETUP_SCANTP_POL[SATSETUP_SCANTP_POL_COUNT] =
{
	{ 0, LOCALE_EXTRA_POL_H },
	{ 1, LOCALE_EXTRA_POL_V }
};

#define DISEQC_ORDER_OPTION_COUNT 2
const CMenuOptionChooser::keyval DISEQC_ORDER_OPTIONS[DISEQC_ORDER_OPTION_COUNT] =
{
	{ COMMITED_FIRST, LOCALE_SATSETUP_DISEQC_COM_UNCOM },
	{ UNCOMMITED_FIRST, LOCALE_SATSETUP_DISEQC_UNCOM_COM  }
};

#define OPTIONS_SOUTH0_NORTH1_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_SOUTH0_NORTH1_OPTIONS[OPTIONS_SOUTH0_NORTH1_OPTION_COUNT] =
{
	{0, LOCALE_EXTRA_SOUTH},
	{1, LOCALE_EXTRA_NORTH}
};

#define OPTIONS_EAST0_WEST1_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_EAST0_WEST1_OPTIONS[OPTIONS_EAST0_WEST1_OPTION_COUNT] =
{
	{0, LOCALE_EXTRA_EAST},
	{1, LOCALE_EXTRA_WEST}
};

// 
#define FRONTEND_MODE_OPTION_COUNT 3
const CMenuOptionChooser::keyval FRONTEND_MODE_OPTIONS[FRONTEND_MODE_OPTION_COUNT] =
{
	{ 0, NONEXISTANT_LOCALE, "single"  },
	{ 1, NONEXISTANT_LOCALE, "loop" },
	{ 2, NONEXISTANT_LOCALE, "not connected" },
};

void CScanSetup::showScanService()
{
	dprintf(DEBUG_DEBUG, "init scansettings\n");
	
	printf("CScanSetup::showScanService: Tuner: %d\n", feindex);
	
	//load scan settings 
	if( !scanSettings.loadSettings(NEUTRINO_SCAN_SETTINGS_FILE, feindex) ) 
		dprintf(DEBUG_NORMAL, "CScanSetup::CScanSetup: Loading of scan settings failed. Using defaults.\n");
	
	//menue init
	CMenuWidget * scansetup = new CMenuWidget(LOCALE_SERVICEMENU_SCANTS, NEUTRINO_ICON_SETTINGS, width);
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::InitScanSettings\n");
	
	// 
	int dmode = scanSettings.diseqcMode;
	int shortcut = 1;
	
	sat_iterator_t sit; //sat list iterator
	
	// intros
	scansetup->addItem(GenericMenuBack);
	scansetup->addItem(GenericMenuSeparatorLine);
	
	//save settings
	scansetup->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	scansetup->addItem(GenericMenuSeparatorLine);
			
	// init satNotify
	CSatelliteSetupNotifier * satNotify = new CSatelliteSetupNotifier();
	
	// Sat Setup
	CMenuWidget * satSetup = new CMenuWidget(LOCALE_SATSETUP_SAT_SETUP, NEUTRINO_ICON_SETTINGS);
	
	satSetup->addItem(GenericMenuBack);
	satSetup->addItem(GenericMenuSeparatorLine);

	// satfind menu
	CMenuWidget * satfindMenu = new CMenuWidget(LOCALE_MOTORCONTROL_HEAD, NEUTRINO_ICON_SETTINGS);

	satfindMenu->addItem(GenericMenuBack);
	satfindMenu->addItem(GenericMenuSeparatorLine);
		
	// satname (list)
	CMenuOptionStringChooser * satSelect = NULL;
	CMenuWidget * satOnOff = NULL;
	
	// scan setup SAT
	if(CFrontend::getInstance(feindex)->getInfo()->type == FE_QPSK) 
	{
		satSelect = new CMenuOptionStringChooser(LOCALE_SATSETUP_SATELLITE, scanSettings.satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);
			
		satOnOff = new CMenuWidget(LOCALE_SATSETUP_SATELLITE, NEUTRINO_ICON_SETTINGS);
	
		// intros
		satOnOff->addItem(GenericMenuBack);
		satOnOff->addItem(GenericMenuSeparatorLine);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
		{
			// satname
			if(sit->second.type == DVB_S)
			{
				satSelect->addOption(sit->second.name.c_str());
				dprintf(DEBUG_DEBUG, "[neutrino] fe(%d) Adding sat menu for %s position %d\n", sit->second.feindex, sit->second.name.c_str(), sit->first);

				CMenuWidget * tempsat = new CMenuWidget(sit->second.name.c_str(), NEUTRINO_ICON_SETTINGS);
				
				tempsat->addItem(GenericMenuBack);
				tempsat->addItem(GenericMenuSeparatorLine);
				
				// save settings
				tempsat->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
				tempsat->addItem(GenericMenuSeparatorLine);

				// satname
				CMenuOptionChooser * inuse = new CMenuOptionChooser(sit->second.name.c_str(),  &sit->second.use_in_scan, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

				// diseqc
				CMenuOptionNumberChooser * diseqc = new CMenuOptionNumberChooser(LOCALE_SATSETUP_DISEQC_INPUT, &sit->second.diseqc, ((dmode != NO_DISEQC) && (dmode != DISEQC_ADVANCED)), -1, 15, NULL, 1, -1, LOCALE_OPTIONS_OFF);

				// commited input
				CMenuOptionNumberChooser * comm = new CMenuOptionNumberChooser(LOCALE_SATSETUP_COMM_INPUT, &sit->second.commited, dmode == DISEQC_ADVANCED, -1, 15, NULL, 1, -1, LOCALE_OPTIONS_OFF);

				// uncommited input
				CMenuOptionNumberChooser * uncomm = new CMenuOptionNumberChooser(LOCALE_SATSETUP_UNCOMM_INPUT, &sit->second.uncommited, dmode == DISEQC_ADVANCED, -1, 15, NULL, 1, -1, LOCALE_OPTIONS_OFF);

				// motor
				CMenuOptionNumberChooser * motor = new CMenuOptionNumberChooser(LOCALE_SATSETUP_MOTOR_POS, &sit->second.motor_position, true, 0, 64, NULL, 0, 0, LOCALE_OPTIONS_OFF);

				// usals
				CMenuOptionChooser * usals = new CMenuOptionChooser(LOCALE_EXTRA_USE_GOTOXX,  &sit->second.use_usals, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

				satNotify->addItem(1, diseqc);
				satNotify->addItem(0, comm);
				satNotify->addItem(0, uncomm);
				
				//satNotify->addItem(0, motor); //FIXME testing motor with not DISEQC_ADVANCED
				//satNotify->addItem(0, usals);

				CIntInput* lofL = new CIntInput(LOCALE_SATSETUP_LOFL, (int&) sit->second.lnbOffsetLow, 5, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE);
				CIntInput* lofH = new CIntInput(LOCALE_SATSETUP_LOFH, (int&) sit->second.lnbOffsetHigh, 5, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE);
				CIntInput* lofS = new CIntInput(LOCALE_SATSETUP_LOFS, (int&) sit->second.lnbSwitch, 5, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE);

				satOnOff->addItem(inuse);
					
				tempsat->addItem(diseqc);
				tempsat->addItem(comm);
				tempsat->addItem(uncomm);
				tempsat->addItem(motor);
				tempsat->addItem(usals);
				tempsat->addItem(new CMenuForwarder(LOCALE_SATSETUP_LOFL, true, lofL->getValue(), lofL ));
				tempsat->addItem(new CMenuForwarder(LOCALE_SATSETUP_LOFH, true, lofH->getValue(), lofH ));
				tempsat->addItem(new CMenuForwarder(LOCALE_SATSETUP_LOFS, true, lofS->getValue(), lofS));
					
				// sat setup
				satSetup->addItem(new CMenuForwarderNonLocalized(sit->second.name.c_str(), true, NULL, tempsat));
			}
		}
	} 
	else if (CFrontend::getInstance(feindex)->getInfo()->type == FE_QAM) 
	{
		satSelect = new CMenuOptionStringChooser(LOCALE_CABLESETUP_PROVIDER, (char*)scanSettings.satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
		{
			if(sit->second.type == DVB_C)
			{
				satSelect->addOption(sit->second.name.c_str());
				dprintf(DEBUG_DEBUG, "[neutrino] fe(%d) Adding cable menu for %s position %d\n", sit->second.feindex, sit->second.name.c_str(), sit->first);
			}
		}
	}
	else if (CFrontend::getInstance(feindex)->getInfo()->type == FE_OFDM) 
	{
		satSelect = new CMenuOptionStringChooser(LOCALE_TERRESTRIALSETUP_PROVIDER, (char*)scanSettings.satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++)
		{
			if(sit->second.type == DVB_T)
			{
				satSelect->addOption(sit->second.name.c_str());
				dprintf(DEBUG_DEBUG, "CNeutrinoApp::InitScanSettings fe(%d) Adding terrestrial menu for %s position %d\n", sit->second.feindex, sit->second.name.c_str(), sit->first);
			}
		}
	}

	// sat select menu
	satfindMenu->addItem(satSelect);

	// motor menu
	CMenuWidget * motorMenu = NULL;

	if ( CFrontend::getInstance(feindex)->getInfo()->type == FE_QPSK) 
	{
		satfindMenu->addItem(new CMenuForwarder(LOCALE_MOTORCONTROL_HEAD, true, NULL, new CMotorControl(feindex), "", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));

		motorMenu = new CMenuWidget(LOCALE_SATSETUP_EXTENDED_MOTOR, NEUTRINO_ICON_SETTINGS);
		
		// intros
		motorMenu->addItem(GenericMenuSeparator);
		motorMenu->addItem(GenericMenuBack);

		// save settings
		motorMenu->addItem(new CMenuForwarder(LOCALE_SATSETUP_SAVESETTINGSNOW, true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));

		motorMenu->addItem(new CMenuForwarder(LOCALE_MOTORCONTROL_HEAD, true, NULL, satfindMenu, "", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));

		motorMenu->addItem(GenericMenuSeparatorLine);

		motorMenu->addItem(new CMenuOptionNumberChooser(LOCALE_EXTRA_ZAPIT_ROTATION_SPEED, (int *)&zapitCfg.motorRotationSpeed, true, 0, 64, NULL) );

		//motorMenu->addItem(new CMenuOptionChooser(LOCALE_EXTRA_USE_GOTOXX,  (int *)&zapitCfg.useGotoXX, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

		CStringInput * toff;
		sprintf(zapit_lat, "%3.6f", zapitCfg.gotoXXLatitude);
		sprintf(zapit_long, "%3.6f", zapitCfg.gotoXXLongitude);

		motorMenu->addItem(new CMenuOptionChooser(LOCALE_EXTRA_LADIR,  (int *)&zapitCfg.gotoXXLaDirection, OPTIONS_SOUTH0_NORTH1_OPTIONS, OPTIONS_SOUTH0_NORTH1_OPTION_COUNT, true));

		toff = new CStringInput(LOCALE_EXTRA_LAT, (char *) zapit_lat, 10, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789.");
		motorMenu->addItem(new CMenuForwarder(LOCALE_EXTRA_LAT, true, zapit_lat, toff));

		motorMenu->addItem(new CMenuOptionChooser(LOCALE_EXTRA_LODIR,  (int *)&zapitCfg.gotoXXLoDirection, OPTIONS_EAST0_WEST1_OPTIONS, OPTIONS_EAST0_WEST1_OPTION_COUNT, true));

		toff = new CStringInput(LOCALE_EXTRA_LONG, (char *) zapit_long, 10, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789.");
		motorMenu->addItem(new CMenuForwarder(LOCALE_EXTRA_LONG, true, zapit_long, toff));
		motorMenu->addItem(new CMenuOptionNumberChooser(LOCALE_SATSETUP_USALS_REPEAT, (int *)&zapitCfg.repeatUsals, true, 0, 10, NULL, 0, 0, LOCALE_OPTIONS_OFF) );
		
		// rotor swap east/west
		motorMenu->addItem( new CMenuOptionChooser(LOCALE_EXTRA_ROTORSWAP, &g_settings.rotor_swap, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true ));
	}
	
	//FIXME:
	// fe mode: connected, independant, loop
	scansetup->addItem(new CMenuForwarderNonLocalized("Tuner Mode", false, "independant", NULL ));
	scansetup->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	// scan type
	CMenuOptionChooser* ojScantype = new CMenuOptionChooser(LOCALE_ZAPIT_SCANTYPE, (int *)&scanSettings.scanType, SCANTS_ZAPIT_SCANTYPE, SCANTS_ZAPIT_SCANTYPE_COUNT, true, NULL, CRCInput::convertDigitToKey(shortcut++), "", true);
	scansetup->addItem(ojScantype);
		
	// bqts
	CMenuOptionChooser* ojBouquets = new CMenuOptionChooser(LOCALE_SCANTS_BOUQUET, (int *)&scanSettings.bouquetMode, SCANTS_BOUQUET_OPTIONS, SCANTS_BOUQUET_OPTION_COUNT, true, NULL, CRCInput::convertDigitToKey(shortcut++), "", true);
	scansetup->addItem(ojBouquets);
		
	scansetup->addItem(GenericMenuSeparatorLine);
		
	// diseqc/diseqcrepeat/lnb/motor
	CMenuOptionChooser * ojDiseqc = NULL;
	CMenuOptionNumberChooser * ojDiseqcRepeats = NULL;
	CMenuForwarder * fsatSetup = NULL;
	CMenuForwarder * fmotorMenu = NULL;
	//CMenuForwarder *fautoScanAll = NULL;

	if( CFrontend::getInstance(feindex)->getInfo()->type == FE_QPSK )
	{
		// diseqc
		ojDiseqc = new CMenuOptionChooser(LOCALE_SATSETUP_DISEQC, (int *)&scanSettings.diseqcMode, SATSETUP_DISEQC_OPTIONS, SATSETUP_DISEQC_OPTION_COUNT, true, satNotify, CRCInput::convertDigitToKey(shortcut++), "", true);
		
		// diseqc repeat
		ojDiseqcRepeats = new CMenuOptionNumberChooser(LOCALE_SATSETUP_DISEQCREPEAT, (int *)&scanSettings.diseqcRepeat, (dmode != NO_DISEQC) && (dmode != DISEQC_ADVANCED), 0, 2, NULL);

		satNotify->addItem(1, ojDiseqcRepeats);

		fsatSetup = new CMenuForwarder(LOCALE_SATSETUP_SAT_SETUP, true, NULL, satSetup, "", CRCInput::convertDigitToKey(shortcut++));
		//fmotorMenu = new CMenuForwarder(LOCALE_SATSETUP_EXTENDED_MOTOR, (dmode == DISEQC_ADVANCED), NULL, motorMenu, "", CRCInput::convertDigitToKey(shortcut++));
		//satNotify->addItem(0, fmotorMenu); //FIXME testing motor with not DISEQC_ADVANCED
		fmotorMenu = new CMenuForwarder(LOCALE_SATSETUP_EXTENDED_MOTOR, true, NULL, motorMenu, "", CRCInput::convertDigitToKey(shortcut++));
		
		scansetup->addItem(ojDiseqc);
		scansetup->addItem(ojDiseqcRepeats);
		scansetup->addItem(fsatSetup);
		scansetup->addItem(fmotorMenu);
	}

	// manuel scan menu
	CMenuWidget* manualScan = new CMenuWidget(LOCALE_SATSETUP_MANUAL_SCAN, NEUTRINO_ICON_SETTINGS);

	CScanTs * scanTs = new CScanTs(feindex);

	// intros
	manualScan->addItem(GenericMenuBack);
	manualScan->addItem(GenericMenuSeparatorLine);
	
	// save settings
	manualScan->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	manualScan->addItem(GenericMenuSeparatorLine);

	// sat select
	manualScan->addItem(satSelect);
		
	// TP select
	CTPSelectHandler * tpSelect = new CTPSelectHandler(feindex);
		
	manualScan->addItem(new CMenuForwarder(LOCALE_SCANTS_SELECT_TP, true, NULL, tpSelect, "test"));
	
	int man_shortcut = 1;
		
	// frequency
	int freq_length = ( CFrontend::getInstance( feindex )->getInfo()->type == FE_QPSK) ? 8 : 6;
	CStringInput * freq = new CStringInput(LOCALE_EXTRA_FREQ, (char *) scanSettings.TP_freq, freq_length, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789");
	CMenuForwarder * Freq = new CMenuForwarder(LOCALE_EXTRA_FREQ, true, scanSettings.TP_freq, freq, "", CRCInput::convertDigitToKey(man_shortcut++));
		
	manualScan->addItem(Freq);
		
	// modulation(t/c)/polarisation(sat)
	CMenuOptionChooser * mod_pol = NULL;

	if( CFrontend::getInstance( feindex )->getInfo()->type == FE_QPSK )
	{
		mod_pol = new CMenuOptionChooser(LOCALE_EXTRA_POL, (int *)&scanSettings.TP_pol, SATSETUP_SCANTP_POL, SATSETUP_SCANTP_POL_COUNT, true, NULL, CRCInput::convertDigitToKey(man_shortcut++), "", true);
	}
	else if( CFrontend::getInstance( feindex )->getInfo()->type == FE_QAM)
	{
		mod_pol = new CMenuOptionChooser(LOCALE_EXTRA_MOD, (int *)&scanSettings.TP_mod, CABLETERRESTRIALSETUP_SCANTP_MOD, CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT, true, NULL, CRCInput::convertDigitToKey(man_shortcut++), "", true);
	}
	else if( CFrontend::getInstance( feindex )->getInfo()->type == FE_OFDM)
	{
		mod_pol = new CMenuOptionChooser(LOCALE_EXTRA_MOD, (int *)&scanSettings.TP_const, CABLETERRESTRIALSETUP_SCANTP_MOD, CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT, true, NULL, CRCInput::convertDigitToKey(man_shortcut++), "", true);
	}

	manualScan->addItem(mod_pol);

	// symbol rate
	CStringInput * rate = new CStringInput(LOCALE_EXTRA_RATE, (char *) scanSettings.TP_rate, 8, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789");
	CMenuForwarder * Rate = new CMenuForwarder(LOCALE_EXTRA_RATE, true, scanSettings.TP_rate, rate, "", CRCInput::convertDigitToKey(man_shortcut++));

	// fec
	int fec_count = ( CFrontend::getInstance( feindex )->getInfo()->type == FE_QPSK) ? SATSETUP_SCANTP_FEC_COUNT : CABLESETUP_SCANTP_FEC_COUNT;
	CMenuOptionChooser * fec = new CMenuOptionChooser(LOCALE_EXTRA_FEC, (int *)&scanSettings.TP_fec, SATSETUP_SCANTP_FEC, fec_count, true, NULL, CRCInput::convertDigitToKey(man_shortcut++), "", true);
		
	if( CFrontend::getInstance( feindex )->getInfo()->type != FE_OFDM)
	{
		// Rate
		manualScan->addItem(Rate);
			
		// fec
		manualScan->addItem(fec);
	}

	// band/hp/lp/
	if( CFrontend::getInstance( feindex )->getInfo()->type == FE_OFDM)
	{
		// Band
		CMenuOptionChooser * Band = new CMenuOptionChooser(LOCALE_EXTRA_BAND, (int *)&scanSettings.TP_band, SATSETUP_SCANTP_BAND, SATSETUP_SCANTP_BAND_COUNT, true, NULL, CRCInput::convertDigitToKey(man_shortcut++), "", true);
		manualScan->addItem(Band);

		// HP
		CMenuOptionChooser * HP = new CMenuOptionChooser(LOCALE_EXTRA_HP, (int *)&scanSettings.TP_HP, /*SATSETUP_SCANTP_HP_LP*/ SATSETUP_SCANTP_FEC, /*SATSETUP_SCANTP_HP_LP_COUNT*/ fec_count, true, NULL, CRCInput::convertDigitToKey(man_shortcut++), "", true);
		manualScan->addItem(HP);

		// LP
		CMenuOptionChooser * LP = new CMenuOptionChooser(LOCALE_EXTRA_LP, (int *)&scanSettings.TP_LP, /*SATSETUP_SCANTP_HP_LP*/ SATSETUP_SCANTP_FEC, /*SATSETUP_SCANTP_HP_LP_COUNT*/fec_count, true, NULL, CRCInput::convertDigitToKey(man_shortcut++), "", true);
		manualScan->addItem(LP);
	}	

	// NIT
	CMenuOptionChooser* useNit = new CMenuOptionChooser(LOCALE_SATSETUP_USE_NIT, (int *)&scanSettings.scan_mode, OPTIONS_OFF1_ON0_OPTIONS, OPTIONS_OFF1_ON0_OPTION_COUNT, true, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN);
	manualScan->addItem(useNit);

	manualScan->addItem(GenericMenuSeparatorLine);
		
	// test signal
	manualScan->addItem(new CMenuForwarder(LOCALE_SCANTS_TEST, true, NULL, scanTs, "test", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
		
	// scan
	manualScan->addItem(new CMenuForwarder(LOCALE_SCANTS_STARTNOW, true, NULL, scanTs, "manual", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
		
	scansetup->addItem(new CMenuForwarder(LOCALE_SATSETUP_MANUAL_SCAN, true, NULL, manualScan, "", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
		
	// auto scan menu
	CMenuWidget * autoScan = new CMenuWidget(LOCALE_SATSETUP_AUTO_SCAN, NEUTRINO_ICON_SETTINGS);
	
	// intros
	autoScan->addItem(GenericMenuBack);
	autoScan->addItem(GenericMenuSeparatorLine);
	
	// save settings
	autoScan->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	autoScan->addItem(GenericMenuSeparatorLine);
		
	// sat select
	autoScan->addItem(satSelect);
		
	// NIT
	autoScan->addItem(useNit);
		
	// scan pids
	CMenuOptionChooser* scanPids = new CMenuOptionChooser(LOCALE_EXTRA_ZAPIT_SCANPIDS,  &scan_pids, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);
	autoScan->addItem(scanPids);
		
	// auto scan
	autoScan->addItem(new CMenuForwarder(LOCALE_SCANTS_STARTNOW, true, NULL, scanTs, "auto", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
		
	// auto scan menu item
	scansetup->addItem(new CMenuForwarder(LOCALE_SATSETUP_AUTO_SCAN, true, NULL, autoScan, "", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));

	// scan all sats
	CMenuForwarder * fautoScanAll = NULL;
		
	if( CFrontend::getInstance( feindex )->getInfo()->type == FE_QPSK )
	{
		ojDiseqc = new CMenuOptionChooser(LOCALE_SATSETUP_DISEQC, (int *)&scanSettings.diseqcMode, SATSETUP_DISEQC_OPTIONS, SATSETUP_DISEQC_OPTION_COUNT, true, satNotify, CRCInput::convertDigitToKey(shortcut++), "", true);
		ojDiseqcRepeats = new CMenuOptionNumberChooser(LOCALE_SATSETUP_DISEQCREPEAT, (int *)&scanSettings.diseqcRepeat, (dmode != NO_DISEQC) && (dmode != DISEQC_ADVANCED), 0, 2, NULL);

		satNotify->addItem(1, ojDiseqcRepeats);

		fsatSetup = new CMenuForwarder(LOCALE_SATSETUP_SAT_SETUP, true, NULL, satSetup, "", CRCInput::convertDigitToKey(shortcut++));
		//fmotorMenu	= new CMenuForwarder(LOCALE_SATSETUP_EXTENDED_MOTOR, (dmode == DISEQC_ADVANCED), NULL, motorMenu, "", CRCInput::convertDigitToKey(shortcut++));
		//satNotify->addItem(0, fmotorMenu); //FIXME testing motor with not DISEQC_ADVANCED
		fmotorMenu = new CMenuForwarder(LOCALE_SATSETUP_EXTENDED_MOTOR, true, NULL, motorMenu, "", CRCInput::convertDigitToKey(shortcut++));

		CMenuWidget * autoScanAll = new CMenuWidget(LOCALE_SATSETUP_AUTO_SCAN_ALL, NEUTRINO_ICON_SETTINGS);
			
		fautoScanAll = new CMenuForwarder(LOCALE_SATSETUP_AUTO_SCAN_ALL, (dmode != NO_DISEQC), NULL, autoScanAll, "", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE);
		satNotify->addItem(2, fautoScanAll);

		// intros
		autoScanAll->addItem(GenericMenuBack);
		autoScanAll->addItem(GenericMenuSeparatorLine);
		
		// save settings
		autoScanAll->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
		autoScanAll->addItem(GenericMenuSeparatorLine);
		
		// sat
		autoScanAll->addItem(new CMenuForwarder(LOCALE_SATSETUP_SATELLITE, true, NULL, satOnOff ));
			
		// NIT
		autoScanAll->addItem(useNit);
			
		// scan pids
		autoScanAll->addItem(scanPids);
			
		// scan
		autoScanAll->addItem(new CMenuForwarder(LOCALE_SCANTS_STARTNOW, true, NULL, scanTs, "all", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));

		// add item 
		scansetup->addItem(fautoScanAll);
	}

	scansetup->exec(NULL, "");
	scansetup->hide();
	delete scansetup;
}

/* TPSelectHandler */
CTPSelectHandler::CTPSelectHandler(int num)
{
	feindex = num;
}

extern std::map<transponder_id_t, transponder> select_transponders;
int CTPSelectHandler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	transponder_list_t::iterator tI;
	sat_iterator_t sit;
	t_satellite_position position = 0;
	std::map<int, transponder> tmplist;
	std::map<int, transponder>::iterator tmpI;
	int i;
	char cnt[5];
	int select = -1;
	static int old_selected = 0;
	static t_satellite_position old_position = 0;

	if (parent)
		parent->hide();

	//loop throught satpos
	for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		if(!strcmp(sit->second.name.c_str(), CNeutrinoApp::getInstance()->getScanSettings().satNameNoDiseqc)) 
		{
			position = sit->first;
			break;
		}
	}

	if(old_position != position) 
	{
		old_selected = 0;
		old_position = position;
	}
	
	printf("CTPSelectHandler::exec: fe(%d) %s position(%d)\n", feindex, CNeutrinoApp::getInstance()->getScanSettings().satNameNoDiseqc, position);

        CMenuWidget * menu = new CMenuWidget(LOCALE_SCANTS_SELECT_TP, NEUTRINO_ICON_SETTINGS);
        CMenuSelectorTarget * selector = new CMenuSelectorTarget(&select);
	
	// intros
	//menu->addItem(GenericMenuSeparator);
	
	i = 0;

	for(tI = select_transponders.begin(); tI != select_transponders.end(); tI++) 
	{
		t_satellite_position satpos = GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xFFF;
		if(GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xF000)
			satpos = -satpos;
		
		if(satpos != position)
			continue;

		char buf[128];
		sprintf(cnt, "%d", i);
		char * f, *s, *m;
		
		switch(CFrontend::getInstance(feindex)->getInfo()->type) 
		{
			case FE_QPSK:
			{
				CFrontend::getInstance(feindex)->getDelSys(tI->second.feparams.u.qpsk.fec_inner, dvbs_get_modulation(tI->second.feparams.u.qpsk.fec_inner),  f, s, m);

				snprintf(buf, sizeof(buf), "%d %c %d %s %s %s ", tI->second.feparams.frequency/1000, tI->second.polarization ? 'V' : 'H', tI->second.feparams.u.qpsk.symbol_rate/1000, f, s, m);
			}
			break;

			case FE_QAM:
			{
				CFrontend::getInstance(feindex)->getDelSys(tI->second.feparams.u.qam.fec_inner, tI->second.feparams.u.qam.modulation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %d %s %s %s ", tI->second.feparams.frequency/1000, tI->second.feparams.u.qam.symbol_rate/1000, f, s, m);
			}
			break;

			case FE_OFDM:
			{
				CFrontend::getInstance(feindex)->getDelSys(tI->second.feparams.u.ofdm.code_rate_HP, tI->second.feparams.u.ofdm.constellation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %s %s %s ", tI->second.feparams.frequency/1000, f, s, m);
			}
			break;
				
			case FE_ATSC:
				break;
		}
		
		menu->addItem(new CMenuForwarderNonLocalized(buf, true, NULL, selector, cnt), old_selected == i);
		tmplist.insert(std::pair <int, transponder>(i, tI->second));
		i++;
	}

	int retval = menu->exec(NULL, "");
	delete menu;
	delete selector;

	if(select >= 0) 
	{
		old_selected = select;

		tmpI = tmplist.find(select);
		//printf("CTPSelectHandler::exec: selected TP: freq %d pol %d SR %d\n", tmpI->second.feparams.frequency, tmpI->second.polarization, tmpI->second.feparams.u.qpsk.symbol_rate);

		sprintf(get_set.TP_freq, "%d", tmpI->second.feparams.frequency);
		
		switch(CFrontend::getInstance(feindex)->getInfo()->type) 
		{
			case FE_QPSK:
				printf("CTPSelectHandler::exec: fe(%d) selected TP: freq %d pol %d SR %d fec %d\n", feindex, tmpI->second.feparams.frequency, tmpI->second.polarization, tmpI->second.feparams.u.qpsk.symbol_rate, tmpI->second.feparams.u.qpsk.fec_inner);
					
				sprintf(get_set.TP_rate, "%d", tmpI->second.feparams.u.qpsk.symbol_rate);
				get_set.TP_fec = tmpI->second.feparams.u.qpsk.fec_inner;
				get_set.TP_pol = tmpI->second.polarization;
				break;

			case FE_QAM:
				printf("CTPSelectHandler::exec: fe(%d) selected TP: freq %d SR %d fec %d mod %d\n", feindex, tmpI->second.feparams.frequency, tmpI->second.feparams.u.qpsk.symbol_rate, tmpI->second.feparams.u.qam.fec_inner, tmpI->second.feparams.u.qam.modulation);
					
				sprintf(get_set.TP_rate, "%d", tmpI->second.feparams.u.qam.symbol_rate);
				get_set.TP_fec = tmpI->second.feparams.u.qam.fec_inner;
				get_set.TP_mod = tmpI->second.feparams.u.qam.modulation;
				break;

			case FE_OFDM:
			{
				printf("CTPSelectHandler::exec: fe(%d) selected TP: freq %d band %d HP %d LP %d const %d trans %d guard %d hierarchy %d\n", feindex, tmpI->second.feparams.frequency, tmpI->second.feparams.u.ofdm.bandwidth, tmpI->second.feparams.u.ofdm.code_rate_HP, tmpI->second.feparams.u.ofdm.code_rate_LP, tmpI->second.feparams.u.ofdm.constellation, tmpI->second.feparams.u.ofdm.transmission_mode, tmpI->second.feparams.u.ofdm.guard_interval, tmpI->second.feparams.u.ofdm.hierarchy_information);
					
				get_set.TP_band = tmpI->second.feparams.u.ofdm.bandwidth;
				get_set.TP_HP = tmpI->second.feparams.u.ofdm.code_rate_HP;
				get_set.TP_LP = tmpI->second.feparams.u.ofdm.code_rate_LP;
				get_set.TP_const = tmpI->second.feparams.u.ofdm.constellation;
				get_set.TP_trans = tmpI->second.feparams.u.ofdm.transmission_mode;
				get_set.TP_guard = tmpI->second.feparams.u.ofdm.guard_interval;
				get_set.TP_hierarchy = tmpI->second.feparams.u.ofdm.hierarchy_information;
			}
			break;

			case FE_ATSC:
				break;
		}	
	}
	
	if(retval == menu_return::RETURN_EXIT_ALL)
		return menu_return::RETURN_EXIT_ALL;

	return menu_return::RETURN_REPAINT;
}

// scan settings
CScanSettings::CScanSettings( /*int num*/)
	: configfile('\t')
{
	//satNameNoDiseqc[0] = 0;
	strcpy(satNameNoDiseqc, "none");
	bouquetMode     = CZapitClient::BM_UPDATEBOUQUETS;
	//scanType = CServiceScan::SCAN_TVRADIO;
	//delivery_system = DVB_S;
	
	//feindex = num;
}

// borrowed from cst neutrino-hd (femanager.cpp)
uint32_t CScanSettings::getConfigValue(int num, const char * name, uint32_t defval)
{
	char cfg_key[81];
	sprintf(cfg_key, "fe%d_%s", num, name);
	
	return configfile.getInt32(cfg_key, defval);
}

// borrowed from cst neutrino-hd (femanger.cpp)
void CScanSettings::setConfigValue(int num, const char * name, uint32_t val)
{
	char cfg_key[81];
	
	sprintf(cfg_key, "fe%d_%s", num, name);
	configfile.setInt32(cfg_key, val);
}

void CScanSettings::useDefaults()
{
	printf("[neutrino] CScanSettings::useDefaults\n");
	
	bouquetMode     = CZapitClient::BM_UPDATEBOUQUETS;
	scanType = CZapitClient::ST_ALL;
	diseqcMode      = NO_DISEQC;
	diseqcRepeat    = 0;
	TP_mod = 3;
	TP_fec = 3;

	//DVB-T
	TP_band = 0;
	TP_HP = 2;
	TP_LP = 1;
	TP_const = 1;
	TP_trans = 1;
	TP_guard = 3;
	TP_hierarchy = 0;

	strcpy(satNameNoDiseqc, "none");
}

bool CScanSettings::loadSettings(const char * const fileName, int feIndex)
{
	printf("[neutrino] CScanSettings::loadSettings: fe%d\n", feIndex);
	
	/* if scan.conf not exists load default */
	if(!configfile.loadConfig(fileName))
		useDefaults( );

	//diseqcMode = configfile.getInt32("diseqcMode"  , diseqcMode);
	diseqcMode = getConfigValue(feIndex, "diseqcMode", diseqcMode);
	//diseqcRepeat = configfile.getInt32("diseqcRepeat", diseqcRepeat);
	diseqcRepeat = getConfigValue(feIndex, "diseqcRepeat", diseqcRepeat);

	//bouquetMode = (CZapitClient::bouquetMode) configfile.getInt32("bouquetMode" , bouquetMode);
	bouquetMode = (CZapitClient::bouquetMode) getConfigValue(feIndex, "bouquetMode", bouquetMode);
	//scanType=(CZapitClient::scanType) configfile.getInt32("scanType", scanType);
	scanType = (CZapitClient::scanType) getConfigValue(feIndex, "scanType", scanType);
	//strcpy(satNameNoDiseqc, configfile.getString("satNameNoDiseqc", satNameNoDiseqc).c_str());
	char cfg_key[81];
	sprintf(cfg_key, "fe%d_satNameNoDiseqc", feIndex);
	strcpy(satNameNoDiseqc, configfile.getString(cfg_key, satNameNoDiseqc).c_str());

	//scan_mode = configfile.getInt32("scan_mode", 1); // NIT (0) or fast (1)
	scan_mode = getConfigValue(feIndex, "scan_mode", 1); // NIT (0) or fast (1)
	
	//TP_fec = configfile.getInt32("TP_fec", 1);
	TP_fec = getConfigValue(feIndex, "TP_fec", 1);
	//TP_pol = configfile.getInt32("TP_pol", 0);
	TP_pol = getConfigValue(feIndex, "TP_pol", 0);
	//TP_mod = configfile.getInt32("TP_mod", 3);
	TP_mod = getConfigValue(feIndex, "TP_mod", 3);
	//strcpy(TP_freq, configfile.getString("TP_freq", "10100000").c_str());
	sprintf(cfg_key, "fe%d_TP_freq", feIndex);
	strcpy(TP_freq, configfile.getString(cfg_key, "10100000").c_str());
	//strcpy(TP_rate, configfile.getString("TP_rate", "27500000").c_str());
	sprintf(cfg_key, "fe%d_TP_rate", feIndex);
	strcpy(TP_rate, configfile.getString(cfg_key, "27500000").c_str());
#if HAVE_DVB_API_VERSION >= 3
	if(TP_fec == 4) 
		TP_fec = 5;
#endif

	//DVB-T
	//TP_band = configfile.getInt32("TP_band", 0);
	TP_band = getConfigValue(feIndex, "TP_band", 0);
	//TP_HP = configfile.getInt32("TP_HP", 2);
	TP_HP = getConfigValue(feIndex, "TP_HP", 2);
	//TP_LP = configfile.getInt32("TP_LP", 1);
	TP_LP = getConfigValue(feIndex, "TP_LP", 1);
	//TP_const = configfile.getInt32("TP_const", 1);
	TP_const = getConfigValue(feIndex, "TP_const", 1);
	//TP_trans = configfile.getInt32("TP_trans", 1);
	TP_trans = getConfigValue(feIndex, "TP_trans", 1);
	//TP_guard = configfile.getInt32("TP_guard", 3);
	TP_guard = getConfigValue(feIndex, "TP_guard", 3);
	//TP_hierarchy = configfile.getInt32("TP_hierarchy", 0);
	TP_hierarchy = getConfigValue(feIndex, "TP_hierarchy", 0);
		
	//scanSectionsd = configfile.getInt32("scanSectionsd", 0);
	scanSectionsd = getConfigValue(feIndex, "scanSectionsd", 0);

	return true;
}

bool CScanSettings::saveSettings(const char * const fileName, int feIndex)
{
	printf("CScanSettings::saveSettings: fe%d\n", feIndex);
	
	#if 0
	configfile.setInt32( "diseqcMode", diseqcMode );
	configfile.setInt32( "diseqcRepeat", diseqcRepeat );
	
	configfile.setInt32( "bouquetMode", bouquetMode );
	configfile.setInt32( "scanType", scanType );
	configfile.setString( "satNameNoDiseqc", satNameNoDiseqc );
	
	configfile.setInt32("scan_mode", scan_mode);

	configfile.setInt32("TP_fec", TP_fec);
	configfile.setInt32("TP_pol", TP_pol);
	configfile.setInt32("TP_mod", TP_mod);
	configfile.setString("TP_freq", TP_freq);
	configfile.setString("TP_rate", TP_rate);

	configfile.setInt32("TP_band", TP_band);
	configfile.setInt32("TP_HP", TP_HP);
	configfile.setInt32("TP_LP", TP_LP);
	configfile.setInt32("TP_const", TP_const);
	configfile.setInt32("TP_trans", TP_trans);
	configfile.setInt32("TP_guard", TP_guard);
	configfile.setInt32("TP_hierarchy", TP_hierarchy);

	configfile.setInt32("scanSectionsd", scanSectionsd );
	#endif
	
	#if 1
	setConfigValue(feIndex, "diseqcMode", diseqcMode );
	setConfigValue(feIndex, "diseqcRepeat", diseqcRepeat );
	
	setConfigValue(feIndex, "bouquetMode", bouquetMode );
	setConfigValue(feIndex, "scanType", scanType );
	
	char cfg_key[81];
	sprintf(cfg_key, "fe%d_satNameNoDiseqc", feIndex);
	configfile.setString(cfg_key, satNameNoDiseqc );
	
	setConfigValue(feIndex, "scan_mode", scan_mode);

	setConfigValue(feIndex, "TP_fec", TP_fec);
	setConfigValue(feIndex, "TP_pol", TP_pol);
	setConfigValue(feIndex, "TP_mod", TP_mod);
	sprintf(cfg_key, "fe%d_TP_freq", feIndex);
	configfile.setString(cfg_key, TP_freq);
	sprintf(cfg_key, "fe%d_TP_rate", feIndex);
	configfile.setString(cfg_key, TP_rate);

	setConfigValue(feIndex, "TP_band", TP_band);
	setConfigValue(feIndex, "TP_HP", TP_HP);
	setConfigValue(feIndex, "TP_LP", TP_LP);
	setConfigValue(feIndex, "TP_const", TP_const);
	setConfigValue(feIndex, "TP_trans", TP_trans);
	setConfigValue(feIndex, "TP_guard", TP_guard);
	setConfigValue(feIndex, "TP_hierarchy", TP_hierarchy);

	setConfigValue(feIndex, "scanSectionsd", scanSectionsd );
	#endif

	if(configfile.getModifiedFlag())
	{
		/* save neu configuration */
		configfile.saveConfig(fileName);
	}

	return true;
}


