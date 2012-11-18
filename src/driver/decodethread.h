/*
	Simple TS Decoder, currently video only

	Copyright 2010 Carsten Juttner <carjay@gmx.net>

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

#define __STDC_CONSTANT_MACROS
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

extern "C"
{
#include <sys/types.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class SWDecoder
{
public:
	SWDecoder();
	SWDecoder(SWDecoder &rhs);
	SWDecoder const &operator= (SWDecoder const &rhs);
	
	void operator()();
	void doFinish() { mFinish = true; } /* quick thread */

	/* called from GL thread */
	class SWFramebuffer : public std::vector<unsigned char>
	{
	public:
		SWFramebuffer() : mWidth(0), mHeight(0) {}
		void width(int w) { mWidth = w; }
		void height(int h) { mHeight = h; }
		int width() const { return mWidth; }
		int height() const { return mHeight; }
	private:
		int mWidth;
		int mHeight;
	};
	typedef boost::shared_ptr< SWFramebuffer > pBuffer_t; /* smart pointer to frame buffer object */
	void returnDisplayBuffer(pBuffer_t pFrameBuffer);
	pBuffer_t acquireDisplayBuffer();
	
private:
	mutable boost::mutex mMutex;

	bool mFinish;
	void setupBuffer();

	void decodeStream();
	void setupCodec();
	void releaseCodec();
	int decodeFrames();

	/* AVCodec acquires ownership of a new buffer, fills it in and then OpenGL will (at some point) display it.
	   Note that this of course does not guarantee realtime playback. It is merely a silly interface so we at
	   least see some colourful images in the background :) . Feel free to scrap or improve... */
	std::deque<pBuffer_t> mDVRBufferlist; /* list of shared pointers to be able to transfer ownership */
	std::deque<pBuffer_t> mDisplayBufferlist; /* list of shared pointers to be able to transfer ownership */
	/* simple queue api */
	void returnDVRBuffer(pBuffer_t pFrameBuffer);
	pBuffer_t acquireDVRBuffer();
	
	struct ffmpeg_ctx
	{ /* used as private context for ffmpeg */
		int DVR;
		int streampid;
		std::vector<int> pmts;
	};
	/* make all functions static, thin layer to read from the DVR */
	static void setupDVR(int &DVR);
	static void releaseDVR(int DVR);
	static int readDVR(int DVR, unsigned char *buffer, int len, int timeout);
	static int ffmpeg_open(URLContext *h, char const *filename, int flags);
	static int ffmpeg_close(URLContext *h);
	static int ffmpeg_read(URLContext *h, unsigned char *buf, int size);
	
	struct {
		AVFormatContext *pFormatCtx;
		AVCodecContext  *pCodecCtx;
	} mState;
	URLProtocol mProto; /* must not be on the stack since it is stored inside ffmpeg */
};

