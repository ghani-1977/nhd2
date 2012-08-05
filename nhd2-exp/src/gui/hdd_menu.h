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


#ifndef __hdd_menu__
#define __hdd_menu__


#include "widget/menue.h"
#include "gui/filebrowser.h"

using namespace std;

// dst
class CHDDDestExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string&);
};

// format
class CHDDFmtExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string&);
};

// checkfs
class CHDDChkExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string&);
};

// mount
class CHDDMountMSGExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string&);
};

// umount
class CHDDuMountMSGExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string&);
};

// init
class CHDDInit : public CMenuTarget
{
	public:
		int exec(CMenuTarget * parent, const std::string&);
};

// explore
class CHDDBrowser : public CMenuTarget
{	
	public:
		int exec(CMenuTarget * parent, const std::string& actionKey);
};

// HDD menu handler
class CHDDMenuHandler : public CMenuTarget
{
	//private:
		//CMenuForwarderNonLocalized * hdd_mounted;
		
	public:
		int  exec( CMenuTarget* parent,  const std::string &actionKey);
		int  hddMenu();
};

#if 0
// hdd notifier
class CHDDMenuHandlerNotifier : public CMenuTarget
{
	private:
		std::string dev;
	public:
		int exec(CMenuTarget* parent, const std::string& actionKey);
		CHDDMenuHandlerNotifier(const std::string& actionKey);
};
#endif


#endif	//hdd_menu_h

