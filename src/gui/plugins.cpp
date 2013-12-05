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

#include <gui/plugins.h>

#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

#include <dirent.h>
#include <dlfcn.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <global.h>
#include <neutrino.h>

/*zapit includes*/
#include <client/zapittools.h>

#include <daemonc/remotecontrol.h>
#include <system/safe_system.h>


extern CPlugins       * g_PluginList;    /* neutrino.cpp */
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */


bool CPlugins::plugin_exists(const std::string & filename)
{
	return (find_plugin(filename) >= 0);
}

int CPlugins::find_plugin(const std::string & filename)
{
	for (int i = 0; i <  (int) plugin_list.size(); i++)
	{
		if ( (filename.compare(plugin_list[i].filename) == 0) || (filename.compare(plugin_list[i].filename + ".cfg") == 0) )
			return i;
	}

	return -1;
}

bool CPlugins::pluginfile_exists(const std::string & filename)
{
	FILE *file = fopen(filename.c_str(), "r");
	if (file != NULL)
	{
		fclose(file);
		return true;
	} 
	else
	{
		return false;
	}
}

void CPlugins::scanDir(const char * dir)
{
	struct dirent **namelist;
	std::string fname;

	int number_of_files = scandir(dir, &namelist, 0, alphasort);

	for (int i = 0; i < number_of_files; i++)
	{
		std::string filename;

		filename = namelist[i]->d_name;
		int pos = filename.find(".cfg");
		if (pos > -1)
		{
			plugin new_plugin;
			new_plugin.filename = filename.substr(0, pos);
			fname = dir;
			fname += '/';
			new_plugin.cfgfile = fname.append(new_plugin.filename);
			new_plugin.cfgfile.append(".cfg");
			parseCfg(&new_plugin);
			bool plugin_ok = parseCfg(&new_plugin);

			if (plugin_ok) 
			{
				new_plugin.pluginfile = fname;
				if (new_plugin.type == CPlugins::P_TYPE_SCRIPT)
				{
					new_plugin.pluginfile.append(".sh");
				} 
				else 
				{
					new_plugin.pluginfile.append(".so");
				}
				// We do not check if new_plugin.pluginfile exists since .cfg in
				// PLUGINDIR_VAR can overwrite settings in read only dir
				// PLUGINDIR. This needs PLUGINDIR_VAR to be scanned at
				// first -> .cfg in PLUGINDIR will be skipped since plugin
				// already exists in the list.
				// This behavior is used to make sure plugins can be disabled
				// by creating a .cfg in PLUGINDIR_VAR (PLUGINDIR often is read only).

				if (!plugin_exists(new_plugin.filename))
				{
					plugin_list.push_back(new_plugin);
					number_of_plugins++;
				}
			}
		}
	}
}

void CPlugins::loadPlugins()
{
	frameBuffer = CFrameBuffer::getInstance();
	number_of_plugins = 0;
	plugin_list.clear();

	// for compatibility with neutrinoHD
	scanDir("/lib/tuxbox/plugins");
	scanDir("/var/plugins");
	
	scanDir(PLUGINDIR);
	
	sort(plugin_list.begin(), plugin_list.end());
}

CPlugins::~CPlugins()
{
	plugin_list.clear();
}

bool CPlugins::parseCfg(plugin *plugin_data)
{
	std::ifstream inFile;
	std::string line[20];
	int linecount = 0;
	bool reject = false;

	inFile.open(plugin_data->cfgfile.c_str());

	while (linecount < 20 && getline(inFile, line[linecount++]))
	{};

	plugin_data->fb = false;
	plugin_data->rc = false;

	plugin_data->lcd = false;
	plugin_data->vtxtpid = false;
	plugin_data->showpig = false;
	
	plugin_data->needoffset = false;
	plugin_data->hide = false;
	plugin_data->type = CPlugins::P_TYPE_DISABLED;

	for (int i = 0; i < linecount; i++)
	{
		std::istringstream iss(line[i]);
		std::string cmd;
		std::string parm;

		getline(iss, cmd, '=');
		getline(iss, parm, '=');

		if (cmd == "pluginversion")
		{
			plugin_data->version = atoi(parm.c_str());
		}
		else if (cmd == "name")
		{
			plugin_data->name = parm;
		}
		else if (cmd == "desc")
		{
			plugin_data->description = parm;
		}
		else if (cmd == "depend")
		{
			plugin_data->depend = parm;
		}
		else if (cmd == "type")
		{
			plugin_data->type = getPluginType(atoi(parm.c_str()));
		}
		else if (cmd == "needfb")
		{
			plugin_data->fb = ((parm == "1")?true:false);
		}
		else if (cmd == "needrc")
		{
			plugin_data->rc = ((parm == "1")?true:false);
		}		
		else if (cmd == "needlcd")
		{
			plugin_data->lcd = ((parm == "1")?true:false);
		}
		else if (cmd == "needvtxtpid")
		{
			plugin_data->vtxtpid = ((parm == "1")?true:false);
		}
		else if (cmd == "pigon")
		{
			plugin_data->showpig = ((parm == "1")?true:false);
		}		
		else if (cmd == "needoffsets")
		{
			plugin_data->needoffset = ((parm == "1")?true:false);
		}
		else if (cmd == "hide")
		{
			plugin_data->hide = ((parm == "1")?true:false);
		}
		else if (cmd == "needenigma")
		{
			reject = ((parm == "1")?true:false);
		}
		else if (cmd == "icon")
		{
			plugin_data->icon = parm;
		}
	}

	inFile.close();
	return !reject;
}

PluginParam * CPlugins::makeParam(const char * const id, const char * const value, PluginParam * const next)
{
	PluginParam * startparam = new PluginParam;

	startparam->next = next;
	startparam->id   = id;
	startparam->val  = strdup(value);

	return startparam;
}

PluginParam * CPlugins::makeParam(const char * const id, const int value, PluginParam * const next)
{
	char aval[10];

	sprintf(aval, "%d", value);

	return makeParam(id, aval, next);
}

void CPlugins::start_plugin_by_name(const std::string & filename, int param)
{
	for (int i = 0; i <  (int) plugin_list.size(); i++)
	{
		if (filename.compare(g_PluginList->getName(i))==0)
		{
			startPlugin(i,param);
			return;
		}
	}
}

void CPlugins::startPlugin(const char * const name)
{
	int pluginnr = find_plugin(name);
	if (pluginnr > -1)
		startPlugin(pluginnr, 0);
	else
		printf("[CPlugins] could not find %s\n", name);

}

void CPlugins::startScriptPlugin(int number)
{
	const char * script = plugin_list[number].pluginfile.c_str();
	
	printf("[CPlugins] executing script %s\n", script);
	
	if (!pluginfile_exists(plugin_list[number].pluginfile))
	{
		printf("[CPlugins] could not find %s,\nperhaps wrong plugin type in %s\n", script, plugin_list[number].cfgfile.c_str());
		return;
	}
	
	if( !safe_system(script) )
	{
		printf("CPlugins::startScriptPlugin: script %s successfull started\n", script);
	} 
	else 
	{	
		printf("[CPlugins] can't execute %s\n",script);
	}
}

void CPlugins::startPlugin(int number, int param)
{
	printf("CPlugins::startPlugin: %s\n", plugin_list[number].pluginfile.c_str());
	
	g_RCInput->clearRCMsg();
	
	// script type
	if (plugin_list[number].type == CPlugins::P_TYPE_SCRIPT)
	{
		g_RCInput->stopInput();

		startScriptPlugin(number);
		
		frameBuffer->paintBackground();
#if !defined USE_OPENGL
		frameBuffer->blit();
#endif			
		
		g_RCInput->restartInput();
		g_RCInput->clearRCMsg();

		return;
	}
	
	// .so plugins
	if (!pluginfile_exists(plugin_list[number].pluginfile))
	{
		printf("[CPlugins] could not find %s,\nperhaps wrong plugin type in %s\n", plugin_list[number].pluginfile.c_str(), plugin_list[number].cfgfile.c_str());
		return;
	}

	/* export neutrino settings to the environment */
	if (plugin_list[number].needoffset)
	{
		char tmp[32];
		sprintf(tmp, "%d", g_settings.screen_StartX);
		setenv("SCREEN_OFF_X", tmp, 1);
		sprintf(tmp, "%d", g_settings.screen_StartY);
		setenv("SCREEN_OFF_Y", tmp, 1);
		sprintf(tmp, "%d", g_settings.screen_EndX);
		setenv("SCREEN_END_X", tmp, 1);
		sprintf(tmp, "%d", g_settings.screen_EndY);
		setenv("SCREEN_END_Y", tmp, 1);
	}

	if (plugin_list[number].type == CPlugins::P_TYPE_TOOL)
	{		
		/* stop rc input */
		g_RCInput->stopInput();
		
		printf("Starting %s\n", plugin_list[number].pluginfile.c_str());
		
		safe_system((char *) plugin_list[number].pluginfile.c_str());
		
		frameBuffer->paintBackground();
#if !defined USE_OPENGL
		frameBuffer->blit();
#endif	
	
		g_RCInput->restartInput();
		g_RCInput->clearRCMsg();
	}
	else if (plugin_list[number].type == CPlugins::P_TYPE_NEUTRINO)
	{
		PluginExec execPlugin;
		char depstring[129];
		char			*argv[20];
		void			*libhandle[20];
		int			argc = 0, i = 0, lcd_fd=-1;
		char			*p;
		char			*np;
		void			*handle;
		char *        		error;
		int           		vtpid      =  0;
		PluginParam * 		startparam =  0;

		g_RCInput->clearRCMsg();
	
		// fb
		if (plugin_list[number].fb)
		{
			// filehandle pointer
			startparam = makeParam(P_ID_FBUFFER, frameBuffer->getFileHandle(), startparam);
		}
		
		// rc
		if (plugin_list[number].rc)
		{
			//startparam = makeParam(P_ID_RCINPUT  , g_RCInput->getFileHandle()      , startparam);
			startparam = makeParam(P_ID_RCBLK_ANF, g_settings.repeat_genericblocker, startparam);
			startparam = makeParam(P_ID_RCBLK_REP, g_settings.repeat_blocker       , startparam);
		}
		/*
		else
		{
			g_RCInput->stopInput();
		}
		*/
	
		// lcd	
		if (plugin_list[number].lcd)
		{
			CVFD::getInstance()->pause();

#if defined (PLATFORM_CUBEREVO) || defined (PLATFORM_CUBEREVO_MINI) || defined (PLATFORM_CUBEREVO_MINI2) || defined (PLATFORM_CUBEREVO_MINI_FTA) || defined (PLATFORM_CUBEREVO_250HD) || defined (PLATFORM_CUBEREVO_2000HD) || defined (PLATFORM_CUBEREVO_9500HD)
			lcd_fd = open("/dev/dbox/fp0", O_RDWR);
#else
			lcd_fd = open("/dev/vfd", O_RDWR);
#endif		

			startparam = makeParam(P_ID_LCD, lcd_fd, startparam);
		}	
	
		// vtxtpid	
		if (plugin_list[number].vtxtpid)
		{
			vtpid = g_RemoteControl->current_PIDs.PIDs.vtxtpid;

			if (param>0)
				vtpid=param;
			
			startparam = makeParam(P_ID_VTXTPID, vtpid, startparam);
		}	
	
		// offset
		if (plugin_list[number].needoffset)
		{
			startparam = makeParam(P_ID_VFORMAT  , g_settings.video_Format         , startparam);
			startparam = makeParam(P_ID_OFF_X    , g_settings.screen_StartX        , startparam);
			startparam = makeParam(P_ID_OFF_Y    , g_settings.screen_StartY        , startparam);
			startparam = makeParam(P_ID_END_X    , g_settings.screen_EndX          , startparam);
			startparam = makeParam(P_ID_END_Y    , g_settings.screen_EndY          , startparam);
		}

		PluginParam *par = startparam;
		for ( ; par; par=par->next )
		{
			printf("[CPlugins] (id, val):(%s, %s)\n", par->id, par->val);
		}
		std::string pluginname = plugin_list[number].filename;

		strcpy(depstring, plugin_list[number].depend.c_str());

		argc = 0;
		if ( depstring[0] )
		{
			p = depstring;
			while ( 1 )
			{
				argv[ argc ] = p;
				argc++;
				np = strchr(p, ',');
				if ( !np )
					break;

				*np = 0;
				p = np + 1;
				if ( argc == 20 )	// mehr nicht !
					break;
			}
		}
	
		for ( i = 0; i < argc; i++ )
		{
			std::string libname = argv[i];
			printf("[CPlugins] try load shared lib : %s\n",argv[i]);
			libhandle[i] = dlopen ( *argv[i] == '/' ?
						argv[i] : (PLUGINDIR "/"+libname).c_str(),
						RTLD_NOW | RTLD_GLOBAL );
			if ( !libhandle[i] )
			{
				fputs (dlerror(), stderr);
				break;
			}
		}
	
		if ( i == argc )		// alles geladen
		{
			handle = dlopen ( plugin_list[number].pluginfile.c_str(), RTLD_NOW);
			if (!handle)
			{
				fputs (dlerror(), stderr);
			} 
			else 
			{
				execPlugin = (PluginExec) dlsym(handle, "plugin_exec");
				if ((error = dlerror()) != NULL)
				{
					fputs(error, stderr);
					dlclose(handle);
				} 
				else 
				{
					printf("[CPlugins] try exec...\n");
					
					execPlugin(startparam);
					dlclose(handle);
					printf("[CPlugins] exec done...\n");
				}
			}

			// restart rc
			//if (!plugin_list[number].rc)
			//	g_RCInput->restartInput();
			
			g_RCInput->clearRCMsg();

			// resume lcd
			if (plugin_list[number].lcd)
			{
				if (lcd_fd != -1)
					close(lcd_fd);
				CVFD::getInstance()->resume();
			}

			// restore fb
			if (plugin_list[number].fb)
			{
#ifdef FB_BLIT
				frameBuffer->blit();
#endif			
			}
		}

		// unload shared libs
		/*
		for ( i=0; i<argc; i++ )
		{
			if ( libhandle[i] )
				dlclose(libhandle[i]);
			else
				break;
		}
		*/

		for (par = startparam ; par; )
		{
			/* we must not free par->id, since it is the original */
			free(par->val);
			PluginParam * tmp = par;
			par = par->next;
			delete tmp;
		}
	}
}

bool CPlugins::hasPlugin(CPlugins::p_type_t type)
{
	for (std::vector<plugin>::iterator it=plugin_list.begin(); it!=plugin_list.end(); it++)
	{
		if (it->type == type && !it->hide)
			return true;
	}
	return false;
}

CPlugins::p_type_t CPlugins::getPluginType(int type)
{
	switch (type)
	{
		case PLUGIN_TYPE_DISABLED:
			return P_TYPE_DISABLED;
			break;
			
		case PLUGIN_TYPE_GAME:
			return P_TYPE_GAME;
			break;
			
		case PLUGIN_TYPE_TOOL:
			return P_TYPE_TOOL;
			break;
			
		case PLUGIN_TYPE_SCRIPT:
			return P_TYPE_SCRIPT;
			break;
			
		case PLUGIN_TYPE_NEUTRINO:
			return P_TYPE_NEUTRINO;
			break;
			
		default:
			return P_TYPE_DISABLED;
	}
}


