/*
	nglcd.h -- Neutrino GraphLCD driver

	Copyright (C) 2012 martii

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __glcd_h__
#define __glcd_h__

#include <string>
#include <vector>
#include <time.h>
#include <string>
#include <sys/types.h>
#include <semaphore.h>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/font.h>
#include <glcddrivers/config.h>
#include <glcddrivers/driver.h>
#include <glcddrivers/drivers.h>


class nGLCD
{
	private:
		GLCD::cDriver * lcd;
		GLCD::cFont font_channel;
		GLCD::cFont font_epg;
		GLCD::cFont font_time;
		int fontsize_channel;
		int fontsize_epg;
		int fontsize_time;
		int percent_channel;
		int percent_time;
		int percent_epg;
		int percent_bar;
		int percent_space;
		GLCD::cBitmap * bitmap;
		std::string Channel;
		std::string Epg;
		std::string stagingChannel;
		std::string stagingEpg;
		int Scale;
		time_t now;
		struct tm *tm;
		bool channelLocked;
		bool doRescan;
		bool doSuspend;
		bool doStandby;
		bool doStandbyTime;
		bool doExit;
		bool doScrollChannel;
		bool doScrollEpg;
		bool doShowVolume;
		pthread_t thrGLCD;
		pthread_mutex_t mutex;
		void updateFonts();
		void Exec();
		std::string scrollChannel;
		std::string scrollEpg;
	public:
		bool fonts_initialized;
		bool doMirrorOSD;
		nGLCD();
		~nGLCD();
		static nGLCD * getInstance();
		void DeInit();
		static void Lock();
		static void Unlock();
		void mainLock();
		void mainUnlock();
		static void lockChannel(std::string &c);
		static void unlockChannel();
		static void * Run(void *);
		static void MirrorOSD(bool b = true);
		static void Update();
		static void Suspend();
		static void StandbyMode(bool);
		static void ShowVolume(bool);
		static void Resume();
		static void Exit();
		void Rescan();
		sem_t sem;
};

#endif
