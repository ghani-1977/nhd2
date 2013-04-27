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

#include "decodethread.h"
SWDecoder::SWDecoder() : mFinish(false)
{
	memset(&mState, 0, sizeof(mState));
}

SWDecoder::SWDecoder(SWDecoder &rhs)
{
	/* lock rhs only since this is not usable yet */
	boost::lock_guard<boost::mutex> lock(rhs.mMutex);
	mFinish = rhs.mFinish;
	mState  = rhs.mState;
}


SWDecoder const &SWDecoder::operator= (SWDecoder const &rhs)
{
	if(&rhs != this)
	{
		/* but here we need to lock both */
		boost::lock_guard<boost::mutex> lock1(&mMutex < &rhs.mMutex ? mMutex : rhs.mMutex);
		boost::lock_guard<boost::mutex> lock2(&mMutex > &rhs.mMutex ? mMutex : rhs.mMutex);
		mFinish = rhs.mFinish;
		mState  = rhs.mState;
	}
	return *this;
}


void SWDecoder::operator()()
{
	int res = 0;
	std::cout << "SWDecoder thread started" << std::endl;
	setupBuffer();
	setupCodec();

	while(!mFinish)
	{
		decodeStream();
		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(5));
	}

	releaseCodec();
	std::cout << "SWDecoder thread stopped" << std::endl;
}


void SWDecoder::setupDVR(int &DVR)
{
	DVR = ::open("/dev/dvb/adapter0/dvr0", O_RDONLY);
	if(DVR < 0)
	{
		std::cerr << boost::format("SWDecoder::setupDVR: Error opening dvr device:%s") % strerror(errno) << std::endl;
	}
	else
	{
		int res = 0;
		int flag = ::fcntl(DVR, F_GETFL);
		if(flag < 0) 
		{
			std::cerr << boost::format("SWDecoder::setupDVR: Error getting flags from file handle:%s") % strerror(errno) << std::endl;
		}
		else
		{
			res = ::fcntl(DVR, F_SETFL, flag | O_NONBLOCK);
			if(res < 0 )
			{
				std::cerr << boost::format("SWDecoder::setupDVR: Error setting file handle to nonblocking:%s") % strerror(errno) << std::endl;
			}
		}
	}
}


void SWDecoder::releaseDVR(int DVR)
{
	if(DVR >= 0)
	{
		::close(DVR);
		DVR = -1;
	}
}


int SWDecoder::readDVR(int DVR, unsigned char *buffer, int len, int timeout)
{
	int res = -1;
	struct pollfd pfn;
	memset(&pfn, 0, sizeof(pfn));
	pfn.fd = DVR,
	pfn.events = POLLIN | POLLPRI;
	
	res = poll(&pfn, 1, timeout);
	if(res > 0)
	{
		if(DVR >= 0)
		{	/* handle is nonblocking */
			res = ::read(DVR, buffer, len);
			if(res < 0 && res != EAGAIN) 
			{
				std::cerr << boost::format("SWDecoder::readDVR: Error reading bytes: %s") % strerror(errno) << std::endl;
			}
		}
	} 
	else
	{
		if(res < 0)
		{
			std::cerr << boost::format("SWDecoder::readDVR: Error polling: %s") % strerror(errno) << std::endl;
		}
	}
	return res;
}


void SWDecoder::setupCodec()
{
	int res = 0;
	av_register_all();

	memset(&mProto, 0, sizeof(mProto));
	
	mProto.name  = "dvrdecode";
	mProto.url_open  = ffmpeg_open;
	mProto.url_read  = ffmpeg_read;
	mProto.url_close = ffmpeg_close;
	res = av_register_protocol(&mProto);
	if(res)
	{
		std::cerr << boost::format("Error registering ffmpeg protocol:%s") % strerror(errno) << std::endl;
	}
}


void SWDecoder::decodeStream()
{
	int res = 0;
	if(!res) 
	{
		std::cout << "opening virtual file" << std::endl;
		AVInputFormat *pFmt = av_find_input_format("mpegts");
		res = av_open_input_file(&mState.pFormatCtx, boost::str(boost::format("dvrdecode://%p") % this).c_str(), pFmt, 0, 0);
		if(res)
		{
			std::cerr << boost::format("Error opening pseudo decoder file:%s") % strerror(errno) << std::endl;
		} else
		{
			std::cout << "finding virtual stream info" << std::endl;

			res = av_find_stream_info(mState.pFormatCtx);
			if(res)
			{
				std::cerr << boost::format("Error finding stream info:%s") % strerror(errno) << std::endl;
			}
			else
			{
				std::cout << "trying to dump format" << std::endl;
				dump_format(mState.pFormatCtx, 0, boost::str(boost::format("dvrdecode://%p") % this).c_str(), false);
				std::cout << "recognized format" << std::endl;
			}
			if(!res)
			{
				decodeFrames();
			}
		}

		if(mState.pFormatCtx)
		{
			av_close_input_file(mState.pFormatCtx);
		}
	}	
}


int SWDecoder::decodeFrames()
{
	int videoStream = -1;
	int i;
	for(i = 0; i < mState.pFormatCtx->nb_streams; i++)
	{
		if(mState.pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
		{
			videoStream = i;
			break;
		}
	}
	if(videoStream == -1)
	{
		std::cerr << "SWDecoder::decodeFrames: no video stream found" << std::endl;
		return -1;
	}
	
	AVCodecContext *pCodecCtx = mState.pFormatCtx->streams[videoStream]->codec;
	AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if(!pCodec)
	{
		std::cerr << "SWDecoder::decodeFrames: no codec found" << std::endl;
		return -1;
	}
	
	if(avcodec_open(pCodecCtx, pCodec)<0)
	{
		std::cerr << "SWDecoder::decodeFrames: error opening codec" << std::endl;
		return -1;
	}
	
	AVFrame *pFrame = avcodec_alloc_frame();
	AVFrame *pFrameRGB = avcodec_alloc_frame();
	
	int numBytes = avpicture_get_size(PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
	
	AVPacket packet;
	int frameFinished;
	while(av_read_frame(mState.pFormatCtx, &packet) >= 0 && !mFinish)
	{
		if(packet.stream_index == videoStream)
		{
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, packet.data, packet.size);
			if(frameFinished)
			{
				pBuffer_t displaybuffer = acquireDVRBuffer();
				if(displaybuffer)
				{ /* non available, use dummy else the DVR may overflow */
					struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
																	pCodecCtx->pix_fmt,
																	pCodecCtx->width, pCodecCtx->height,
																	PIX_FMT_RGB32,
																	SWS_BICUBIC,
																	0, 0, 0);
					if(!img_convert_ctx)
					{
						std::cout << "SWDecoder::decodeFrames: error setting up image convert context" << std::endl;
					}
					else
					{
						if(displaybuffer->size() < numBytes)
						{
							displaybuffer->resize(numBytes);
						}
						avpicture_fill(reinterpret_cast<AVPicture *>(pFrameRGB), &(*displaybuffer)[0], PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
						sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
												   pFrameRGB->data, pFrameRGB->linesize);
						sws_freeContext(img_convert_ctx);
						/* enqueue for display */
						displaybuffer->width(pCodecCtx->width);
						displaybuffer->height(pCodecCtx->height);
						returnDVRBuffer(displaybuffer);
					}
				}
				else
				{
					//std::cout << "all buffer full, skipping..." << std::endl;
				}
			}
		}
		av_free_packet(&packet);
	}
	
	av_free(pFrameRGB);
	av_free(pFrame);
	avcodec_close(pCodecCtx);
}


void SWDecoder::releaseCodec()
{
	if(mState.pCodecCtx)
	{
		avcodec_close(mState.pCodecCtx);
	}
}


int SWDecoder::ffmpeg_open(URLContext *h, char const *filename, int flags)
{
	ffmpeg_ctx *ctx = static_cast<ffmpeg_ctx*>(malloc(sizeof(ffmpeg_ctx)));
	memset(ctx, 0, sizeof(ffmpeg_ctx));
	h->priv_data = ctx;
	std::cout << __FUNCTION__ << std::endl;
	setupDVR(ctx->DVR);
	return ctx->DVR;
}


int SWDecoder::ffmpeg_close(URLContext *h)
{
	std::cout << __FUNCTION__ << std::endl;
	ffmpeg_ctx *ctx = static_cast<ffmpeg_ctx*>(h->priv_data);
	if(ctx)
	{
		releaseDVR(ctx->DVR);
		free(ctx);
		h->priv_data = 0;
	}
	return 0;
}


int SWDecoder::ffmpeg_read(URLContext *h, unsigned char *buf, int size)
{
	int res = -1;
	ffmpeg_ctx *ctx = static_cast<ffmpeg_ctx*>(h->priv_data);
	if(ctx)
	{

		res = SWDecoder::readDVR(ctx->DVR, buf, size, 10000);
		if(res > 0)
		{
			if(!(res % 188) && (buf[0] == 0x47)) /* assume we start with a full packet */
			{
				int pid = ((static_cast<int>(buf[1]) << 8) | buf[2]) & 0x1fff;
				if(!pid)
				{
					ctx->pmts.clear(); /* we need the pmt pids to tell them apart from the stream */
					/* this code does not deal with multiple sections! */
					int prognrs = ((((unsigned int)buf[5+1]&0x0f)<<8) | buf[5+2] - 5 - 4) / 4;
					for(int idx = 0; idx < prognrs; ++idx)
					{
						int pid = (((unsigned int)buf[5+10+(idx*4)] << 8) | buf[5+11+(idx*4)]) & 0x1fff;
						ctx->pmts.push_back(pid);
					}
				}
				else
				{
					if(std::find(ctx->pmts.begin(), ctx->pmts.end(), pid) == ctx->pmts.end())
					{ /* it is not one of the pmts */
						if(ctx->streampid && pid != ctx->streampid)
						{
							res = -1; /* stream changed, trigger close */
							std::cout << __FUNCTION__ << boost::format("new streampid %04x, retrigger") % ctx->streampid << std::endl;
						}
						ctx->streampid = pid;
					}
				}
			}
		}
	}
	return res;
}


void SWDecoder::setupBuffer()
{
	boost::lock_guard<boost::mutex> lock(mMutex);
	const int maxcount = 10;
	if(mDVRBufferlist.size() == 0)
	{
		for(int i = 0; i < maxcount; ++i)
		{   /* fill buffer dispenser */
			mDVRBufferlist.push_back(pBuffer_t(new SWFramebuffer));
		}
	}
}


void SWDecoder::returnDVRBuffer(pBuffer_t pFrameBuffer)
{
	boost::lock_guard<boost::mutex> lock(mMutex);
	mDisplayBufferlist.push_back(pFrameBuffer);
}


SWDecoder::pBuffer_t SWDecoder::acquireDVRBuffer()
{
	boost::lock_guard<boost::mutex> lock(mMutex);
	pBuffer_t retbuf;
	if(!mDVRBufferlist.empty())
	{
		retbuf = mDVRBufferlist.front();
		mDVRBufferlist.pop_front();
	}
	return retbuf;
}


void SWDecoder::returnDisplayBuffer(pBuffer_t pFrameBuffer)
{
	boost::lock_guard<boost::mutex> lock(mMutex);
	mDVRBufferlist.push_back(pFrameBuffer);
}


SWDecoder::pBuffer_t SWDecoder::acquireDisplayBuffer()
{
	boost::lock_guard<boost::mutex> lock(mMutex);
	pBuffer_t retbuf;
	if(!mDisplayBufferlist.empty())
	{
		retbuf = mDisplayBufferlist.front();
		mDisplayBufferlist.pop_front();
	}
	return retbuf;
}


