/*
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
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h> 

#include <sys/types.h>

#include "record_cs.h"

#include <system/debug.h>
#include <zapit/client/zapittypes.h>


static const char * FILENAME = "[record_cs.cpp]";


/* helper function to call the cpp thread loop */
void *execute_record_thread(void *c)
{
	cRecord *obj = (cRecord *)c;
	obj->RecordThread();
	
	return NULL;
}

cRecord::cRecord(int num)
{
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	demux_num = num /*RECORD_DEMUX*/;
	fe_num = num;
	dmx = NULL;
	record_thread_running = false;
	file_fd = -1;
	exit_flag = RECORD_STOPPED;
}

cRecord::~cRecord()
{
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	Stop();
}

bool cRecord::Open(int /*numpids*/)
{
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	exit_flag = RECORD_STOPPED;
	return true;
}

bool cRecord::Start(int fd, unsigned short vpid, unsigned short * apids, int numpids)
{
	dprintf(DEBUG_INFO, "fd %d, vpid 0x%02x\n", fd, vpid);
	int i;

	if (!dmx)
		dmx = new cDemux( demux_num );

	dmx->Open(DMX_TP_CHANNEL, 2 * 3008 * 62, fe_num );
	dmx->pesFilter(vpid);

	for (i = 0; i < numpids; i++)
		dmx->addPid(apids[i]);

	file_fd = fd;
	exit_flag = RECORD_RUNNING;

	i = pthread_create(&record_thread, 0, execute_record_thread, this);
	if (i != 0)
	{
		exit_flag = RECORD_FAILED_READ;
		errno = i;
		dprintf(DEBUG_INFO, "error creating thread!\n");
		delete dmx;
		dmx = NULL;
		return false;
	}
	record_thread_running = true;
	return true;
}

bool cRecord::Stop(void)
{
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);

	if (exit_flag != RECORD_RUNNING)
		printf("status not RUNNING? (%d)\n", exit_flag);

	exit_flag = RECORD_STOPPED;
	if (record_thread_running)
		pthread_join(record_thread, NULL);
	record_thread_running = false;

	/* We should probably do that from the destructor... */
	if (!dmx)
		printf("dmx == NULL?\n");
	else
		delete dmx;
	dmx = NULL;

	if (file_fd != -1)
		close(file_fd);
	else
		printf("file_fd not open??\n");
	file_fd = -1;
	return true;
}

void cRecord::RecordThread()
{
	//dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
#define BUFSIZE (1 << 19) /* 512 kB */
	ssize_t r = 0;
	int buf_pos = 0;
	uint8_t *buf;
	buf = (uint8_t *)malloc(BUFSIZE);

	if (!buf)
	{
		exit_flag = RECORD_FAILED_MEMORY;
		//dprintf(DEBUG_INFO, "unable to allocate buffer! (out of memory)\n");
	}

	dmx->Start();
	
	while (exit_flag == RECORD_RUNNING)
	{
		if (buf_pos < BUFSIZE)
		{
			r = dmx->Read(buf + buf_pos, BUFSIZE - 1 - buf_pos, 100);
			//dprintf(DEBUG_INFO, "buf_pos %6d r %6d / %6d\n", buf_pos, (int)r, BUFSIZE - 1 - buf_pos);
			if (r < 0)
			{
				if (errno != EAGAIN)
				{
					//dprintf(DEBUG_INFO, "read failed\n");
					exit_flag = RECORD_FAILED_READ;
					break;
				}
				//dprintf(DEBUG_INFO, "EAGAIN\n");
			}
			else
				buf_pos += r;
		}
		//else
		//	printf("buffer full! Overflow?\n");
		
		if (buf_pos > (BUFSIZE / 3)) /* start writeout */
		{
			size_t towrite = BUFSIZE / 2;
			if (buf_pos < BUFSIZE / 2)
				towrite = buf_pos;
			r = write(file_fd, buf, towrite);
			if (r < 0)
			{
				exit_flag = RECORD_FAILED_FILE;
				//dprintf(DEBUG_INFO, "write error\n");
				break;
			}
			buf_pos -= r;
			memmove(buf, buf + r, buf_pos);
			//dprintf(DEBUG_INFO, "buf_pos %6d w %6d / %6d\n", buf_pos, (int)r, (int)towrite);
		}
	}
	dmx->Stop();
	while (buf_pos > 0) /* write out the unwritten buffer content */
	{
		r = write(file_fd, buf, buf_pos);
		if (r < 0)
		{
			exit_flag = RECORD_FAILED_FILE;
			//dprintf(DEBUG_INFO, "write error\n");
			break;
		}
		buf_pos -= r;
		memmove(buf, buf + r, buf_pos);
	}
	free(buf);

	//dprintf(DEBUG_INFO, "end\n");
	
	pthread_exit(NULL);
}

