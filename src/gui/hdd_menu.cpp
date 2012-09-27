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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/mount.h>

#include <global.h>
#include <neutrino.h>
#include <gui/widget/icons.h>
#include "gui/widget/menue.h"
#include "gui/widget/stringinput.h"
#include "gui/widget/messagebox.h"
#include "gui/widget/hintbox.h"
#include "gui/widget/progresswindow.h"

#include "system/setting_helpers.h"
#include "system/settings.h"
#include "system/debug.h"

#include <gui/hdd_menu.h>

#include <blkid/blkid.h>



bool has_hdd = false;

#define HDD_NOISE_OPTION_COUNT 4
const CMenuOptionChooser::keyval HDD_NOISE_OPTIONS[HDD_NOISE_OPTION_COUNT] =
{
	{ 0,   LOCALE_OPTIONS_OFF },
	{ 128, LOCALE_HDD_SLOW },
	{ 190, LOCALE_HDD_MIDDLE },
	{ 254, LOCALE_HDD_FAST }
};

#define HDD_SLEEP_OPTION_COUNT 7
const CMenuOptionChooser::keyval HDD_SLEEP_OPTIONS[HDD_SLEEP_OPTION_COUNT] =
{
	{0, LOCALE_OPTIONS_OFF},
	{12, LOCALE_HDD_1MIN},
	{60, LOCALE_HDD_5MIN},
	{120, LOCALE_HDD_10MIN},
	{240, LOCALE_HDD_20MIN},
	{241, LOCALE_HDD_30MIN},
	{242, LOCALE_HDD_60MIN}
};

static int my_filter(const struct dirent * dent)
{
	if(dent->d_name[0] == 's' && dent->d_name[1] == 'd')
		return 1;
	
	return 0;
}

int CHDDMenuHandler::exec(CMenuTarget * parent, const std::string &actionKey)
{
	//int   res = menu_return::RETURN_REPAINT;
	
	//test
	if (parent)
		parent->hide();
	
	if(actionKey == "savehddsettings") 
	{
		//CNeutrinoApp::getInstance()->saveSetup(NEUTRINO_SETTINGS_FILE);
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		//return res;
	}
	else if(actionKey == "activateNow")
	{
		/* hint box */
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MAINSETTINGS_SAVESETTINGSNOW_HINT)); // UTF-8
		hintBox->paint();
		
		CHDDDestExec * hddActiv = new CHDDDestExec();
		hddActiv->exec(NULL, "");
	
		delete hddActiv;
		
		hintBox->hide();
		delete hintBox;
		
		//return res;
	}
	
	//if (parent)
		//parent->hide();
	
	return hddMenu();

	//return doMenu ();
	//return res;
}

/* return 1 if mounted and -1 if not mounted */
static int check_if_mounted(char * dev)
{
	char buffer[255];
	FILE *f = fopen("/proc/mounts", "r");
	
	if(f != NULL)
	{
		while (fgets (buffer, 255, f) != NULL) 
		{
			if(strstr(buffer, dev)) 
			{
				//printf("HDD: mountd\n");
				fclose(f);
				return 1;
			}
		}
		fclose(f);
	}

	//printf("HDD: not mountd\n");

	return 0;
}

int CHDDMenuHandler::hddMenu()
{
	FILE * f;
	int fd;
	struct dirent **namelist;
	int ret;

	int n = scandir("/sys/block", &namelist, my_filter, alphasort);

	if (n < 0) 
	{
                perror("CHDDMenuHandler::doMenu: scandir(\"/sys/block\") failed");

                return menu_return::RETURN_REPAINT;
        }

	CMenuWidget * hddmenu = new CMenuWidget(LOCALE_HDD_SETTINGS, NEUTRINO_ICON_SETTINGS);
	
	// intros
	//hddmenu->addItem(GenericMenuSeparator);
	hddmenu->addItem(GenericMenuBack);
	hddmenu->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	hddmenu->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "savehddsettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	hddmenu->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// activate settings
	//hddmenu->addItem(new CMenuForwarder(LOCALE_HDD_ACTIVATE, true, "", new CHDDDestExec() ));
	hddmenu->addItem(new CMenuForwarder(LOCALE_HDD_ACTIVATE, true, NULL, this, "activateNow", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));

	// sleep time
	hddmenu->addItem( new CMenuOptionChooser(LOCALE_HDD_SLEEP, &g_settings.hdd_sleep, HDD_SLEEP_OPTIONS, HDD_SLEEP_OPTION_COUNT, true, NULL, /*CRCInput::RC_nokey, ""*/ CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW, true));
	
	// noise
	hddmenu->addItem( new CMenuOptionChooser(LOCALE_HDD_NOISE, &g_settings.hdd_noise, HDD_NOISE_OPTIONS, HDD_NOISE_OPTION_COUNT, true, NULL, /*CRCInput::RC_nokey, ""*/ CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE, true ));

	hddmenu->addItem( GenericMenuSeparatorLine );
	
	//hdd manage
	//test
	CMenuWidget * tempMenu[n];

	for(int i = 0; i < n; i++) 
	{
		char str[256];
		char vendor[128];
		char model[128];
		int64_t bytes;
		int64_t megabytes;
		int removable = 0;

		printf("[neutrino] HDD: checking /sys/block/%s\n", namelist[i]->d_name);
		
		sprintf(str, "/dev/%s", namelist[i]->d_name);
		fd = open(str, O_RDONLY);

		if(fd < 0) 
		{
			printf("[neutrino] HDD: Cant open %s\n", str);
			continue;
		}
		
		if (ioctl(fd, BLKGETSIZE64, &bytes))
			perror("BLKGETSIZE64");
                
                close(fd);

		megabytes = bytes/1000000;

		// vendor
		sprintf(str, "/sys/block/%s/device/vendor", namelist[i]->d_name);
		f = fopen(str, "r");

		if(!f) 
		{
			printf("[neutrino] HDD: Cant open %s\n", str);
			continue;
		}
		fscanf(f, "%s", vendor);
		fclose(f);

		// model
		sprintf(str, "/sys/block/%s/device/model", namelist[i]->d_name);
		f = fopen(str, "r");
		
		if(!f) 
		{
			printf("[neutrino] HDD: Cant open %s\n", str);
			continue;
		}
		fscanf(f, "%s", model);
		fclose(f);

		// removable
		sprintf(str, "/sys/block/%s/removable", namelist[i]->d_name);
		f = fopen(str, "r");
		
		if(!f) 
		{
			printf("[neutrino] HDD: Cant open %s\n", str);
			continue;
		}
		fscanf(f, "%d", &removable);
		fclose(f);

		sprintf(str, "%s %s (%s-%s %lld %s)", g_Locale->getText(LOCALE_HDD_MANAGE), namelist[i]->d_name, vendor, model, megabytes < 10000 ? megabytes : megabytes/1000, megabytes < 10000 ? "MB" : "GB");

		printf("[neutrino] HDD: %s\n", str);

		/* hdd menu */
		tempMenu[i] = new CMenuWidget(str, NEUTRINO_ICON_SETTINGS);
		
		// intros
		//tempMenu[i]->addItem(GenericMenuSeparator);
		tempMenu[i]->addItem( GenericMenuBack );
		tempMenu[i]->addItem( GenericMenuSeparatorLine );
		
		//prepare hdd
		//tempMenu[i]->addItem(new CMenuForwarderNonLocalized("Partage HDD", true, NULL, NULL));
		tempMenu[i]->addItem(new CMenuForwarder(LOCALE_HDD_INIT, true, "", new CHDDInit, namelist[i]->d_name, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
		tempMenu[i]->addItem( GenericMenuSeparatorLine );

		
		/* check for parts */
		#define MAX_PARTS 4
		char DEVICE[256];
		char PART[256];
		
		CMenuWidget * PartMenu[MAX_PARTS];
		
		for (int j=1; j<= MAX_PARTS; j++)
		{
			memset(PART, 0, sizeof(PART));
			memset(DEVICE, 0, sizeof(DEVICE));
			
			sprintf(PART, "%s%d", namelist[i]->d_name, j);
			sprintf(DEVICE, "/dev/%s%d", namelist[i]->d_name, j);
			
			fd = open(DEVICE, O_RDONLY);

			if( fd < 0) 
			{
				//printf("[neutrino] HDD: %s not exist\n", DEVICE);
				close(fd);
				continue;
			}
			
			/* part submenu */
			PartMenu[j] = new CMenuWidget(PART, NEUTRINO_ICON_SETTINGS);
			
			// intros
			//PartMenu[j]->addItem(GenericMenuSeparator);
			PartMenu[j]->addItem( GenericMenuBack );
			PartMenu[j]->addItem( GenericMenuSeparatorLine );
			
			/* format part */
			PartMenu[j]->addItem(new CMenuForwarder(LOCALE_HDD_FORMAT, true, "", new CHDDFmtExec, PART, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
			
			/* fs check */
			PartMenu[j]->addItem(new CMenuForwarder(LOCALE_HDD_CHECK, true, "", new CHDDChkExec, PART, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
			
			/* mount part */
			PartMenu[j]->addItem(new CMenuForwarder(LOCALE_HDD_MOUNT, true, "", new CHDDMountMSGExec, PART, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));

			/* umount part */
			PartMenu[j]->addItem(new CMenuForwarder(LOCALE_HDD_UMOUNT, true, "", new CHDDuMountMSGExec, PART, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
			
			// hdd explorer
			PartMenu[j]->addItem( GenericMenuSeparatorLine );
			PartMenu[j]->addItem(new CMenuForwarder(LOCALE_HDD_BROWSER, true, "", new CHDDBrowser(), PART, CRCInput::RC_1));
			
			
			//tempMenu[i]->addItem(new CMenuForwarderNonLocalized(PART, strcmp(fstype, (char *)"swap")?true:false, fstype, PartMenu[j]));
			tempMenu[i]->addItem(new CMenuForwarderNonLocalized(PART, true, NULL, PartMenu[j]));
			
			close(fd);
		}
		/* END check for Parts */
		
		hddmenu->addItem(new CMenuForwarderNonLocalized(str, true, NULL, tempMenu[i]));

		/* result */
		has_hdd = true;
		
		//test
		free(namelist[i]);
	}
	
	if (n >= 0)
                free(namelist);
	
	/* no parts found */

	ret = hddmenu->exec(NULL, "");
	
	// delet temp menus
	for(int i = 0; i < n;i++) 
	{     
                if( has_hdd && tempMenu[i] != NULL )
		{
                        delete tempMenu[i];
                }
        }
	
	delete hddmenu;

	return ret;
}

// hdd init
int CHDDInit::exec(CMenuTarget * /*parent*/, const std::string& key)
{
	char cmd[100];
	CHintBox * hintbox;
	int res;
	FILE * f;
	char dst[128];
	CProgressWindow * progress;
	bool idone;

	printf("CHDDInit: key %s\n", key.c_str());

	res = ShowMsgUTF ( LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_INIT_WARN), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo );

	if(res != CMessageBox::mbrYes)
		return 0;
	
	f = fopen("/proc/sys/kernel/hotplug", "w");
	if(f) 
	{
		fprintf(f, "none\n");
		fclose(f);
	}
	
	progress = new CProgressWindow();
	progress->setTitle(LOCALE_HDD_INIT);
	progress->exec(NULL,"");
	progress->showStatusMessageUTF("Executing fdisk");
	progress->showGlobalStatus(0);
	
	//sprintf(cmd, "/sbin/init_hdd.sh /dev/%s", key.c_str());
	//printf("CHDDInit: executing %s\n", cmd);
	
	sprintf(cmd, "/sbin/init_hdd.sh /dev/%s", key.c_str());
	printf("CHDDInit: executing %s\n", cmd);
	
	f=popen(cmd, "r");
	if (!f) 
	{
		hintbox = new CHintBox(LOCALE_HDD_INIT, g_Locale->getText(LOCALE_HDD_INIT_FAILED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		return menu_return::RETURN_REPAINT;
	}
	
	char buf[256];
	idone = false;
	while(fgets(buf, 255, f) != NULL)
	{
		printf("%s", buf);
                if(!idone && strncmp(buf, "Building a new DOS disklabel.", 29)) 
		{
			idone = true;
			buf[21] = 0;
			progress->showGlobalStatus(20);
                        progress->showStatusMessageUTF(buf);
                } 
		else if(strncmp(buf, "Command (m for help): Command action", 36)) 
		{
			progress->showGlobalStatus(40);
                        progress->showStatusMessageUTF(buf);
		}
		else if(strncmp(buf, "The partition table", 19)) 
		{
			progress->showGlobalStatus(60);
                        progress->showStatusMessageUTF(buf);
		}
	}
	pclose(f);
	progress->showGlobalStatus(100);
	sleep(2);
	
	progress->hide();
	delete progress;

	f = fopen("/proc/sys/kernel/hotplug", "w");
	if(f) 
	{
		fprintf(f, "/sbin/hotplug\n");
		fclose(f);
	}

	sprintf(dst, "/media/%s2", key.c_str());
	

	sprintf(cmd, "%s/record", dst);
	safe_mkdir((char *) cmd);
	sprintf(cmd, "%s/movie", dst);
	safe_mkdir((char *) cmd);
	sprintf(cmd, "%s/picture", dst);
	safe_mkdir((char *) cmd);
	sprintf(cmd, "%s/epg", dst);
	safe_mkdir((char *) cmd);
	sprintf(cmd, "%s/music", dst);
	safe_mkdir((char *) cmd);
	sync();

	return menu_return::RETURN_REPAINT;
}

int CHDDBrowser::exec(CMenuTarget * parent, const std::string& actionKey)
{
	printf("CHDDBrowser: key %s\n", actionKey.c_str());
	
	parent->hide();
	
	CFileBrowser filebrowser;
	char dst[128];
	
	sprintf(dst, "/media/%s", actionKey.c_str());
	
	printf("CHDDBrowser: dst %s\n", dst);
	
	// filefilter
	CFileFilter filefilter;
	filefilter.addFilter("conf");
	
	filefilter.addFilter("ts");
	filefilter.addFilter("avi");
	filefilter.addFilter("mkv");
	filefilter.addFilter("wav");
	filefilter.addFilter("asf");
	filefilter.addFilter("aiff");
	filefilter.addFilter("mpg");
	filefilter.addFilter("mpeg");
	filefilter.addFilter("m2p");
	filefilter.addFilter("mpv");
	filefilter.addFilter("m2ts");
	filefilter.addFilter("vob");
	filefilter.addFilter("mp4");
	filefilter.addFilter("mov");
	filefilter.addFilter("flv");
	filefilter.addFilter("flac");
	filefilter.addFilter("wmv");
	filefilter.addFilter("wma");
	
	filefilter.addFilter("url");
	filefilter.addFilter("xml");
	filefilter.addFilter("m3u");
	filefilter.addFilter("pls");
	
	filefilter.addFilter("cdr");
	filefilter.addFilter("mp3");
	filefilter.addFilter("m2a");
	filefilter.addFilter("mpa");
	filefilter.addFilter("mp2");
	filefilter.addFilter("m3u");
	filefilter.addFilter("ogg");
	filefilter.addFilter("wav");
	filefilter.addFilter("flac");
	
	filefilter.addFilter("png");
	filefilter.addFilter("bmp");
	filefilter.addFilter("jpg");
	filefilter.addFilter("jpeg");
	filefilter.addFilter("gif");
	filefilter.addFilter("crw");
	
	filefilter.addFilter("xml");
	
	filebrowser.Filter = &filefilter;
	
	//filebrowser.Dir_Mode = true;	
	if(filebrowser.exec(dst))
		printf("done...\n");
	
	return menu_return::RETURN_REPAINT;
}
//

int CHDDDestExec::exec(CMenuTarget * /*parent*/, const std::string&)
{
        char cmd[100];
        struct dirent **namelist;

        int n = scandir("/sys/block", &namelist, my_filter, alphasort);

        if (n < 0)
                return 0;

        for (int i = 0; i < n; i++) 
	{
                printf("CHDDDestExec: noise %d sleep %d /dev/%s\n", g_settings.hdd_noise, g_settings.hdd_sleep, namelist[i]->d_name);
                //hdparm -M is not included in busybox hdparm!
                //we need full version of hdparm or should remove -M parameter here

                snprintf(cmd, sizeof(cmd), "hdparm -M%d -S%d /dev/%s >/dev/null 2>/dev/null &", g_settings.hdd_noise, g_settings.hdd_sleep, namelist[i]->d_name);

                system(cmd);

                free(namelist[i]);
        }

        free(namelist);
	
	has_hdd = true;

        return 1;
}

int CHDDFmtExec::exec(CMenuTarget* parent, const std::string& key)
{
	char cmd[100];
	CHintBox * hintbox;
	int res;
	FILE * f;
	char src[128], dst[128];
	CProgressWindow * progress;
	bool idone;

	sprintf(src, "/dev/%s", key.c_str());
	sprintf(dst, "/media/%s", key.c_str());

	printf("CHDDFmtExec: key %s\n", key.c_str());

	res = ShowMsgUTF ( LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_FORMAT_WARN), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo );

	if(res != CMessageBox::mbrYes)
		return 0;

	bool srun = system("killall -9 smbd");

	/* check if mounted then umount */
	if(check_if_mounted(src) == 1)
	{
		/* umount */
		res = umount(dst);
		
		if(res == -1) 
		{
			hintbox = new CHintBox(LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_UMOUNT_WARN));
			hintbox->paint();
			sleep(2);
			delete hintbox;
			goto _return;
		}
	}
	else
	{
		/* umount */
		strcpy(dst, "/hdd");
		res = umount(dst);
		
		if(res == -1) 
		{
			hintbox = new CHintBox(LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_UMOUNT_WARN));
			hintbox->paint();
			sleep(2);
			delete hintbox;
			goto _return;
		}
	}

	f = fopen("/proc/sys/kernel/hotplug", "w");
	if(f) 
	{
		fprintf(f, "none\n");
		fclose(f);
	}

	progress = new CProgressWindow();
	progress->setTitle(LOCALE_HDD_FORMAT);
	progress->exec(NULL,"");
	progress->showStatusMessageUTF("Executing fdisk");
	progress->showGlobalStatus(0);

	//create sda1
	#if 0
	sprintf(cmd, "/sbin/fdisk -u /dev/%s", key.c_str());
	printf("CHDDFmtExec: executing %s\n", cmd);

	f=popen(cmd, "w");
	if (!f) 
	{
		hintbox = new CHintBox(LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_FORMAT_FAILED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		goto _remount;
	}

	fprintf(f, "0,\n;\n;\n;\ny\n");
	pclose(f);
	#endif
	
	#if 0
	sprintf(cmd, "/sbin/init_hdd.sh /dev/%s", key.c_str());
	printf("CHDDInit: executing %s\n", cmd);
	
	f=popen(cmd, "r");
	if (!f) 
	{
		hintbox = new CHintBox(LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_FORMAT_FAILED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		//return menu_return::RETURN_REPAINT;
		goto _remount;
	}
	#endif
	
	//format part ext3
	sprintf(cmd, "/sbin/mkfs.ext3 -T largefile -m0 %s", src);

	printf("CHDDFmtExec: executing %s\n", cmd);

	f=popen(cmd, "r");
	if (!f) 
	{
		hintbox = new CHintBox(LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_FORMAT_FAILED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		goto _remount;
	}

	char buf[256];
	idone = false;
	while(fgets(buf, 255, f) != NULL)
	{
		printf("%s", buf);
                if(!idone && strncmp(buf, "Writing inode", 13)) 
		{
			idone = true;
			buf[21] = 0;
			progress->showGlobalStatus(20);
                        progress->showStatusMessageUTF(buf);
                } 
		else if(strncmp(buf, "Creating", 8)) 
		{
			progress->showGlobalStatus(40);
                        progress->showStatusMessageUTF(buf);
		}
		else if(strncmp(buf, "Writing superblocks", 19)) 
		{
			progress->showGlobalStatus(60);
                        progress->showStatusMessageUTF(buf);
		}
	}
	pclose(f);
	progress->showGlobalStatus(100);
	sleep(2);

	sprintf(cmd, "/sbin/tune2fs -r 0 -c 0 -i 0 %s", src);
	printf("CHDDFmtExec: executing %s\n", cmd);
	system(cmd);

_remount:
	progress->hide();
	delete progress;

	//ext3 fs
	res = mount(src, dst, "ext3", 0, NULL);

	f = fopen("/proc/sys/kernel/hotplug", "w");
	if(f) 
	{
		fprintf(f, "/sbin/hotplug\n");
		fclose(f);
	}

	if(!res) 
	{
		sprintf(cmd, "%s/record", dst);
		safe_mkdir((char *) cmd);
		sprintf(cmd, "%s/movie", dst);
		safe_mkdir((char *) cmd);
		sprintf(cmd, "%s/picture", dst);
		safe_mkdir((char *) cmd);
		sprintf(cmd, "%s/epg", dst);
		safe_mkdir((char *) cmd);
		sprintf(cmd, "%s/music", dst);
		safe_mkdir((char *) cmd);
		sync();
	}
	
_return:
	if(!srun) 
		system("smbd");

	return menu_return::RETURN_REPAINT;
}

int CHDDChkExec::exec(CMenuTarget* parent, const std::string& key)
{
	char cmd[100];
	CHintBox * hintbox;
	int res;
	char src[128], dst[128];
	char *fstype;
	FILE * f;
	CProgressWindow * progress;
	int oldpass = 0, pass, step, total;
	int percent = 0, opercent = 0;

	sprintf(src, "/dev/%s", key.c_str());
	sprintf(dst, "/media/%s", key.c_str());

	printf("CHDDChkExec: key %s\n", key.c_str());

	bool srun = system("killall -9 smbd");

	//res = check_and_umount(src, dst);
	if(check_if_mounted(src) == 1)
	{
		umount(dst);
	}
	else
	{
		strcpy(dst, "/hdd");
		umount(dst);
	}

	/* if umounted */
	//check if we have ext3 fstype
	fstype=blkid_get_tag_value(NULL, "TYPE", src);
	printf("fstype: %s\n", fstype);
	
	std::string fsType = fstype;

	#if 0
	/* if we don't have ext3/ext2 */
	if( fsType != "ext3" || fsType != "ext2")
	{
		hintbox = new CHintBox(LOCALE_HDD_CHECK, g_Locale->getText(LOCALE_HDD_CHECK_FAILED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		goto ret1;
	}
	#endif

	sprintf(cmd, "/sbin/fsck.%s -C 1 -f -y %s", fstype, src);

	printf("CHDDChkExec: Executing %s\n", cmd);
	
	f=popen(cmd, "r");
	
	// handle error
	if(!f) 
	{
		hintbox = new CHintBox(LOCALE_HDD_CHECK, g_Locale->getText(LOCALE_HDD_CHECK_FAILED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		goto ret1;
	}

	progress = new CProgressWindow();
	progress->setTitle(LOCALE_HDD_CHECK);
	progress->exec(NULL,"");

	char buf[256];
	while(fgets(buf, 255, f) != NULL)
	{
		if(isdigit(buf[0])) 
		{
			sscanf(buf, "%d %d %d\n", &pass, &step, &total);
			if(total == 0)
				total = 1;
			if(oldpass != pass) 
			{
				oldpass = pass;
				progress->showGlobalStatus(pass > 0 ? (pass-1)*20: 0);
			}
			percent = (step * 100) / total;
			if(opercent != percent) 
			{
				opercent = percent;
				//printf("CHDDChkExec: pass %d : %d\n", pass, percent);
				progress->showLocalStatus(percent);
			}
		}
		else if(!strncmp(buf, "Pass", 4))
			progress->showStatusMessageUTF(buf);
	}
	//printf("CHDDChkExec: %s\n", buf);
	pclose(f);
	progress->showGlobalStatus(100);
	progress->showStatusMessageUTF(buf);
	sleep(2);
	progress->hide();
	delete progress;

ret1:
	fstype=blkid_get_tag_value(NULL, "TYPE", src);
	//printf("fstype: %s\n", fstype);

	res = mount(src, dst, fstype, 0, NULL);

	printf("CHDDChkExec: mount res %d\n", res);
	
	if(!srun) 
		system("smbd");
	
	return menu_return::RETURN_REPAINT;
}

int CHDDMountMSGExec::exec(CMenuTarget* parent, const std::string& key)
{
	CHintBox * hintbox;
	int res;
	char src[128], dst[128];
	char *fstype;

	sprintf(src, "/dev/%s", key.c_str());
	sprintf(dst, "/media/%s", key.c_str());

	printf("CHDDMountMSGExec: %s\n", (char *)key.c_str());

	res = check_if_mounted((char *)key.c_str());

	/* if mounted */
	if(res == 1) 
	{
		hintbox = new CHintBox(LOCALE_HDD_MOUNT, g_Locale->getText(LOCALE_HDD_MOUNTED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		return menu_return::RETURN_REPAINT;
	}
	else
	{
		/* get fs type */
		fstype=blkid_get_tag_value(NULL, "TYPE", src);
		printf("fstype: %s\n", fstype);

		if(fstype != NULL)
		{
			res = mount(src, dst, fstype, 0, NULL);
	
			printf("CHDDMountExec: mount res %d\n", res);

			if(res == 0) 
			{
				hintbox = new CHintBox(LOCALE_HDD_MOUNT, g_Locale->getText(LOCALE_HDD_MOUNTED));
				hintbox->paint();
				sleep(2);
				delete hintbox;
				
				return menu_return::RETURN_REPAINT;
			}
			else
			{
				// if dest dir don't exists mount to /hdd
				res = mount(src, "/hdd", fstype, 0, NULL);
	
				printf("CHDDMountExec: mount res %d\n", res);
				
				if(res == 0)
				{
					hintbox = new CHintBox(LOCALE_HDD_MOUNT, g_Locale->getText(LOCALE_HDD_MOUNTED));
					hintbox->paint();
					sleep(2);
					delete hintbox;
					
					return menu_return::RETURN_REPAINT;
				}
				else
				{
					hintbox = new CHintBox(LOCALE_HDD_MOUNT, g_Locale->getText(LOCALE_HDD_MOUNTFAILED));
					hintbox->paint();
					sleep(2);
					delete hintbox;
					return menu_return::RETURN_REPAINT;
				}
			}
		}
		else
		{
			hintbox = new CHintBox(LOCALE_HDD_MOUNT, g_Locale->getText(LOCALE_HDD_MOUNTFAILED));
			hintbox->paint();
			sleep(2);
			delete hintbox;
			return menu_return::RETURN_REPAINT;
		}
	}
	
	return menu_return::RETURN_REPAINT;
}

int CHDDuMountMSGExec::exec(CMenuTarget* parent, const std::string& key)
{
	CHintBox * hintbox;
	int res;
	char src[128], dst[128];

	sprintf(src, "/dev/%s", key.c_str());
	sprintf(dst, "/media/%s", key.c_str());

	printf("CHDDuMountExec: %s\n", src);

	/* umount */
	if(check_if_mounted(src) == 1)
	{
		res = umount(dst);
		printf("CHDDuMountExec: umount res %d\n", res);

		if(res == 0)	/* umounted */
		{
			hintbox = new CHintBox(LOCALE_HDD_MOUNT, g_Locale->getText(LOCALE_HDD_UMOUNTED));
			hintbox->paint();
			sleep(2);
			delete hintbox;
			
			return menu_return::RETURN_REPAINT;
		}
		else
		{
			hintbox = new CHintBox(LOCALE_HDD_MOUNT, g_Locale->getText(LOCALE_HDD_UMOUNT_WARN));
			hintbox->paint();
			sleep(2);
			delete hintbox;
			return menu_return::RETURN_REPAINT;
		}
	}
	else
	{
		// perhaps mounted to /hdd
		strcpy(dst, "/hdd");
		
		if(check_if_mounted(dst) == 1)
		{
			res = umount(dst);
			printf("CHDDuMountExec: umount res %d\n", res);

			if(res == 0)	/* umounted */
			{
				hintbox = new CHintBox(LOCALE_HDD_MOUNT, g_Locale->getText(LOCALE_HDD_UMOUNTED));
				hintbox->paint();
				sleep(2);
				delete hintbox;
				
				return menu_return::RETURN_REPAINT;
			}
			else
			{
				hintbox = new CHintBox(LOCALE_HDD_MOUNT, g_Locale->getText(LOCALE_HDD_UMOUNT_WARN));
				hintbox->paint();
				sleep(2);
				delete hintbox;
				return menu_return::RETURN_REPAINT;
			}
		}
		
		// not mounted
		printf("not mounted\n");
		hintbox = new CHintBox(LOCALE_HDD_MOUNT, g_Locale->getText(LOCALE_HDD_UMOUNTED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		return menu_return::RETURN_REPAINT;
	}
	
	return menu_return::RETURN_REPAINT;
}


