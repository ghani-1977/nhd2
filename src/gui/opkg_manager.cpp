/*
	Experimental OPKG-Manager - Neutrino-GUI

	Based upon Neutrino-GUI 
	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Implementation: 
	Copyright (C) 2012 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

        License: GPL

        This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the
	Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
	Boston, MA  02110-1301, USA.

		
	NOTE for ignorant distributors:
	It's not allowed to distribute any compiled parts of this code, if you don't accept the terms of GPL.
	Please read it and understand it right!
	This means for you: Hold it, if not, leave it! You could face legal action! 
	Otherwise ask the copyright owners, anything else would be theft!
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "gui/opkg_manager.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>
#include "gui/widget/progresswindow.h"
#include <driver/screen_max.h>

#include <system/debug.h>

#include <stdio.h>


COPKGManager::COPKGManager()
{
	width = 560;
	vp_pkg_menu = NULL;
	v_pkg_list.clear();
	v_pkg_installed.clear();
	v_pkg_upgradable.clear();
}


COPKGManager::~COPKGManager()
{
	
}

const opkg_cmd_struct_t pkg_types[OM_MAX] =
{
	{OM_LIST, 		"opkg-cl list"},
	{OM_LIST_INSTALLED, 	"opkg-cl list-installed"},
	{OM_LIST_UPGRADEABLE,	"opkg-cl list-upgradable"},
	{OM_UPDATE,		"opkg-cl update"},
	{OM_UPGRADE,		"opkg-cl upgrade"},
};

const char *pkg_menu_names[] = {
	"List All",
	"List Installed",
	"List Upgradable",
	"Update Package List",
	"Upgrade System",
};

int COPKGManager::exec(CMenuTarget* parent, const std::string &actionKey)
{
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
		parent->hide();
	
	for(uint pi = 0; pi < OM_MAX; pi++)
	{
		if(actionKey == pkg_types[pi].cmdstr)
		{
			if (pi < OM_UPDATE)
				res = showPkgMenu(pi);
			else
			{
				if(execCmd(pkg_types[pi].cmdstr))
				{
					DisplayInfoMessage("Command successfull, restart of Neutrino may be required...");
					//CNeutrinoApp::getInstance()->exec(NULL, "restart");
					return res;
				}
				else
					DisplayInfoMessage("Command failed");
			}
			
			return res;
		}
	}
	
	if (vp_pkg_menu)
	{
		for(uint i = 0; i < vp_pkg_menu->size(); i++)
		{
			if(actionKey == vp_pkg_menu->at(i)) 
			{
				if(execCmd(pkg_types[OM_UPDATE].cmdstr))
				{
					std::string action_name = "opkg-cl -V 3 install " + getBlankPkgName(vp_pkg_menu->at(i));
					if(execCmd(action_name.c_str()))
					{
						DisplayInfoMessage("Update successfull, restart of Neutrino required...");
						//CNeutrinoApp::getInstance()->exec(NULL, "restart");
						return res;
					}
				}
				else
					DisplayInfoMessage("Update failed");
				
				return res;
			}
		}
	}
	
	res = showMenu(); 
	
	return res;
}



//show items
int COPKGManager::showPkgMenu(const int pkg_content_id)
{
	CHintBox * loadingBox;

	loadingBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, "Loading package list");	// UTF-8	
	loadingBox->paint();

	if(!execCmd(pkg_types[OM_UPDATE].cmdstr))
		DisplayInfoMessage("Update failed");
		
	CMenuWidget *menu = new CMenuWidget("OPKG-Manager", NEUTRINO_ICON_UPDATE);
	
	//menu->addIntroItems();
	getPkgData(pkg_content_id);
	if (vp_pkg_menu)
	{
		for(uint i = 0; i < vp_pkg_menu->size(); i++)
		{
			printf("Update to %s\n", vp_pkg_menu->at(i).c_str());
			//std::string action_name = getBlankPkgName(vp_pkg_menu->at(i));
			menu->addItem( new CMenuForwarderNonLocalized(vp_pkg_menu->at(i).c_str(), true, NULL , this, vp_pkg_menu->at(i).c_str()));
		}
	}

	loadingBox->hide();
	int res = menu->exec (NULL, "");
	menu->hide ();
	delete menu;
	delete loadingBox;
	return res;
}

int COPKGManager::showMenu()
{
	CMenuWidget *menu = new CMenuWidget("OPKG-Manager", NEUTRINO_ICON_UPDATE);
	
	//menu->addIntroItems();
	for(uint i = 0; i < OM_MAX; i++)
	{
		menu->addItem( new CMenuForwarderNonLocalized(pkg_menu_names[i], true, NULL , this, pkg_types[i].cmdstr));
	}


	int res = menu->exec (NULL, "");
	menu->hide ();
	delete menu;
	return res;
}

//returns true if opkg support is available
bool COPKGManager::hasOpkgSupport()
{
	std::string deps[] = {"/bin/opkg-cl", "/bin/opkg-key", "/etc/opkg/opkg.conf", "/var/lib/opkg"};
	bool ret = true;
	
	for (uint i = 0; i < (sizeof(deps) / sizeof(deps[0])); i++)
	{
		if(access(deps[i].c_str(), R_OK) !=0)
		{
			printf("[neutrino opkg] %s not found\n", deps[i].c_str());
			ret = false;
		}
	}
	
	return ret;
}


void COPKGManager::getPkgData(const int pkg_content_id)
{
	char cmd[100];
	FILE * f;
	snprintf(cmd, sizeof(cmd), pkg_types[pkg_content_id].cmdstr);
	
	printf("COPKGManager: executing %s\n", cmd);
	
	f = popen(cmd, "r");
	
	if (!f) //failed
	{
		DisplayInfoMessage("Command failed");
		sleep(2);
		return;
	}
	
	char buf[OM_MAX_LINE_LENGTH];
	setbuf(f, NULL);
	int in, pos;
	bool is_pkgline;
	pos = 0;
	
	switch (pkg_content_id) 
	{
		case OM_LIST: //list of pkgs
		{
			v_pkg_list.clear();
			vp_pkg_menu = &v_pkg_list;
			break;
		}
		case OM_LIST_INSTALLED: //installed pkgs
		{
			v_pkg_installed.clear();
			vp_pkg_menu = &v_pkg_installed;
			break;
		}
		case OM_LIST_UPGRADEABLE:
		{
			v_pkg_upgradable.clear();
			vp_pkg_menu = &v_pkg_upgradable;
			break;
		}
		default:
			vp_pkg_menu = NULL;
			printf("unknown content id! \n\t");
			break;
	}

	while (true)
	{
		in = fgetc(f);
		if (in == EOF)
			break;

		buf[pos] = (char)in;
		if (pos == 0)
			is_pkgline = ((in != ' ') && (in != '\t'));
		/* avoid buffer overflow */
		if (pos+1 > OM_MAX_LINE_LENGTH)
			in = '\n';
		else
			pos++;
		buf[pos] = 0;
		
		if (in == '\b' || in == '\n')
		{
			pos = 0; /* start a new line */
			if ((in == '\n') && is_pkgline)
			{
				//clean up string
				int ipos = -1;
				std::string line = buf;
				while( (ipos = line.find('\n')) != -1 )
					line = line.erase(ipos,1);
								
				//add to lists
				switch (pkg_content_id) 
				{
					case OM_LIST: //list of pkgs
					{
						v_pkg_list.push_back(line);
						//printf("%s\n", buf);
						break;
					}
					case OM_LIST_INSTALLED: //installed pkgs
					{
						v_pkg_installed.push_back(line);
						//printf("%s\n", buf);
						break;
					}
					case OM_LIST_UPGRADEABLE:
					{
						v_pkg_upgradable.push_back(line);
						//printf("%s\n", buf);
						break;
					}
					default:
						printf("unknown output! \n\t");
						printf("%s\n", buf);
						break;
				}
			}
		}
	}

 	pclose(f);
}

std::string COPKGManager::getBlankPkgName(const std::string& line)
{
	int l_pos = line.find(" ");
	std::string name = line.substr(0, l_pos);
	return name;
}

bool COPKGManager::execCmd(const char * cmdstr)
{
	char cmd[100];
	FILE * f;
	snprintf(cmd, sizeof(cmd), cmdstr);
	
	printf("COPKGManager: executing %s\n", cmd);
	
	f = popen(cmd, "r");
	
	if (!f) //failed
	{
		DisplayInfoMessage("Command failed");
		sleep(2);
		return false;
	}
	

 	pclose(f);

	return true;
}
