/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
                      2003 thegoodguy

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

#include <driver/framebuffer.h>

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>
#include <math.h>

#include <linux/kd.h>

#include <stdint.h>

#include <gui/color.h>
#include <global.h>
#include <system/debug.h>

// generic
#ifdef USE_OPENGL
#include <GL/glew.h>
#include "rcinput.h"
#include "glthread.h"
#endif


#define BACKGROUNDIMAGEWIDTH 	DEFAULT_XRES
#define BACKGROUNDIMAGEHEIGHT	DEFAULT_YRES

#define ICON_CACHE_SIZE 1024*1024*2 // 2mb


// png/jpg/bmp/gif/crw
CFormathandler * fh_root;
void init_handlers(void);
void add_format(int (*picsize)(const char *,int *,int*,int,int),int (*picread)(const char *,unsigned char **,int*,int*), int (*id)(const char*));


static uint32_t * virtual_fb = NULL;

inline unsigned int make16color(uint16_t r, uint16_t g, uint16_t b, uint16_t t,
                                  uint32_t  /*rl*/ = 0, uint32_t  /*ro*/ = 0,
                                  uint32_t  /*gl*/ = 0, uint32_t  /*go*/ = 0,
                                  uint32_t  /*bl*/ = 0, uint32_t  /*bo*/ = 0,
                                  uint32_t  /*tl*/ = 0, uint32_t  /*to*/ = 0)
{
        return ((t << 24) & 0xFF000000) | ((r << 8) & 0xFF0000) | ((g << 0) & 0xFF00) | (b >> 8 & 0xFF);
}

CFrameBuffer::CFrameBuffer()
: active ( true )
{
	iconBasePath = "";
	available  = 0;
	cmap.start = 0;
	cmap.len = 256;
	cmap.red = red;
	cmap.green = green;
	cmap.blue  = blue;
	cmap.transp = trans;
	backgroundColor = 0;
	useBackgroundPaint = false;
	background = NULL;
	backupBackground = NULL;
	backgroundFilename = "";
	fd  = 0;
	m_manual_blit = -1;
	
//FIXME: test
	memset(red, 0, 256*sizeof(__u16));
	memset(green, 0, 256*sizeof(__u16));
	memset(blue, 0, 256*sizeof(__u16));
	memset(trans, 0, 256*sizeof(__u16));

	// png/jpg/bmp/crw handlers
	fh_root = NULL;
	init_handlers();
}

CFrameBuffer* CFrameBuffer::getInstance()
{
	static CFrameBuffer * frameBuffer = NULL;

	if(!frameBuffer) 
	{
		frameBuffer = new CFrameBuffer();
		dprintf(DEBUG_NORMAL, "CFrameBuffer::getInstance: frameBuffer Instance created\n");
	} 

	return frameBuffer;
}

void CFrameBuffer::init(const char * const fbDevice)
{
	int tr = 0xFF;
	
#ifdef USE_OPENGL
	fd = -1;
	if(!mpGLThreadObj)
	{
		screeninfo.bits_per_pixel = DEFAULT_BPP;
		screeninfo.xres = DEFAULT_XRES;
		screeninfo.xres_virtual = screeninfo.xres;
		screeninfo.yres = DEFAULT_YRES;
		screeninfo.yres_virtual = screeninfo.yres;
		screeninfo.bits_per_pixel = DEFAULT_BPP;
		screeninfo.blue.length = 8;
		screeninfo.blue.offset = 0;
		screeninfo.green.length = 8;
		screeninfo.green.offset = 8;
		screeninfo.red.length = 8;
		screeninfo.red.offset = 16;
		screeninfo.transp.length = 8;
		screeninfo.transp.offset = 24;
		mpGLThreadObj = new GLThreadObj(screeninfo.xres, screeninfo.yres);

		if(mpGLThreadObj)
		{ 
			/* kick off the GL thread for the window */
			mpGLThreadObj->Start();
			mpGLThreadObj->waitInit();
		}
	}
	lfb = reinterpret_cast<fb_pixel_t*>(mpGLThreadObj->getOSDBuffer());
	memset(lfb, 0x7f, screeninfo.xres * screeninfo.yres * 4);
#else	

	fd = open(fbDevice, O_RDWR);

	if(!fd) 
		fd = open(fbDevice, O_RDWR);

	if (fd < 0) 
	{
		perror(fbDevice);
		goto nolfb;
	}

	// get variable screeninfo
	if (ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo) < 0) 
	{
		perror("FBIOGET_VSCREENINFO");
		goto nolfb;
	}

	memcpy(&oldscreen, &screeninfo, sizeof(screeninfo));

	// get fix screen info
	fb_fix_screeninfo fix;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix)<0) 
	{
		perror("FBIOGET_FSCREENINFO");
		goto nolfb;
	}

	available = fix.smem_len;
	
	dprintf(DEBUG_NORMAL, "CFrameBuffer::init %dk video mem\n", available/1024);
	
	lfb = (fb_pixel_t *)mmap(0, available, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

	if (!lfb) 
	{
		perror("mmap");
		goto nolfb;
	}

#ifdef __sh__ 
	//we add 2MB at the end of the buffer, the rest does the blitter 
	lfb += 1920 * 1080;

	if (available / (1024 * 1024) < 12)
	{
		printf("CFrameBuffer::init: to less memory for stmfb given, need at least 12mb\n"); 
		goto nolfb;
	}
#else
	enableManualBlit();
#endif /*sh*/ 
#endif /* USE_OPENGL */

	// icons cache
	cache_size = 0;
	
	// Windows Colors
	paletteSetColor(0x1, 0x010101, tr);
        paletteSetColor(0x2, 0x800000, tr);
        paletteSetColor(0x3, 0x008000, tr);
        paletteSetColor(0x4, 0x808000, tr);
        paletteSetColor(0x5, 0x000080, tr);
        paletteSetColor(0x6, 0x800080, tr);
        paletteSetColor(0x7, 0x008080, tr);
        paletteSetColor(0x8, 0xA0A0A0, tr);
        paletteSetColor(0x9, 0x505050, tr);
        paletteSetColor(0xA, 0xFF0000, tr);
        paletteSetColor(0xB, 0x00FF00, tr);
        paletteSetColor(0xC, 0xFFFF00, tr);
        paletteSetColor(0xD, 0x0000FF, tr);
        paletteSetColor(0xE, 0xFF00FF, tr);
        paletteSetColor(0xF, 0x00FFFF, tr);
        paletteSetColor(0x10, 0xFFFFFF, tr);
        paletteSetColor(0x11, 0x000000, tr);
        paletteSetColor(COL_BACKGROUND, 0x000000, 0xffff);

        paletteSet();

        useBackground(false);

	return;

nolfb:
	dprintf(DEBUG_NORMAL, "CFrameBuffer::init: framebuffer not available.\n");
	lfb = 0;
}


CFrameBuffer::~CFrameBuffer()
{
	std::map<std::string, Icon>::iterator it;

	for(it = icon_cache.begin(); it != icon_cache.end(); it++) 
	{
		free(it->second.data);
	}
	icon_cache.clear();
	
	if (background) 
	{
		delete[] background;
		background = NULL;
	}

	if (backupBackground) 
	{
		delete[] backupBackground;
		backupBackground = NULL;
	}

	if (lfb)
		munmap(lfb, available);
	
	if (virtual_fb)
	{
		delete[] virtual_fb;
		virtual_fb = NULL;
	}
	
#ifdef USE_OPENGL
	active = false; /* keep people/infoclocks from accessing */
	mpGLThreadObj->shutDown();
	mpGLThreadObj->join();
#else	
	close(fd);
#endif	
}

int CFrameBuffer::getFileHandle() const
{
	return fd;
}

unsigned int CFrameBuffer::getStride() const
{
	return stride;
}

unsigned int CFrameBuffer::getScreenWidth(bool real)
{
	if(real)
		return xRes;
	else
		return g_settings.screen_EndX - g_settings.screen_StartX;
}

unsigned int CFrameBuffer::getScreenHeight(bool real)
{
	if(real)
		return yRes;
	else
		return g_settings.screen_EndY - g_settings.screen_StartY;
}

unsigned int CFrameBuffer::getScreenX()
{
	return g_settings.screen_StartX;
}

unsigned int CFrameBuffer::getScreenY()
{
	return g_settings.screen_StartY;
}

fb_pixel_t * CFrameBuffer::getFrameBufferPointer() const
{	  
	if (active || (virtual_fb == NULL))
	{
		return lfb;		
	}	
	else
		return (fb_pixel_t *) virtual_fb;
}

unsigned int CFrameBuffer::getAvailableMem() const
{
	return available;
}

bool CFrameBuffer::getActive() const
{
	return (active || (virtual_fb != NULL));
}

void CFrameBuffer::setActive(bool enable)
{
	active = enable;
		
#if !defined __sh__ && !defined USE_OPENGL
	if(enable)
	{
		// set manual blit when fb is activ
		if (m_manual_blit == 0) 
		{
			enableManualBlit();
		}
	}
	else
	{
		if (m_manual_blit == 1) 
		{
			disableManualBlit();
		}
	}
#endif
}

t_fb_var_screeninfo *CFrameBuffer::getScreenInfo()
{
	return &screeninfo;
}

// borrowed from e2 :-P
void CFrameBuffer::setFrameBufferMode(unsigned int nxRes, unsigned int nyRes, unsigned int nbpp)
{
	screeninfo.xres_virtual = screeninfo.xres = nxRes;
	screeninfo.yres_virtual = (screeninfo.yres = nyRes)*2;
	screeninfo.height = 0;
	screeninfo.width = 0;
	screeninfo.xoffset = screeninfo.yoffset = 0;
	screeninfo.bits_per_pixel = nbpp;

	switch (nbpp) 
	{
		case 16:
			// ARGB 1555
			screeninfo.transp.offset = 15;
			screeninfo.transp.length = 1;
			screeninfo.red.offset = 10;
			screeninfo.red.length = 5;
			screeninfo.green.offset = 5;
			screeninfo.green.length = 5;
			screeninfo.blue.offset = 0;
			screeninfo.blue.length = 5;
			break;
		case 32:
			// ARGB 8888
			screeninfo.transp.offset = 24;
			screeninfo.transp.length = 8;
			screeninfo.red.offset = 16;
			screeninfo.red.length = 8;
			screeninfo.green.offset = 8;
			screeninfo.green.length = 8;
			screeninfo.blue.offset = 0;
			screeninfo.blue.length = 8;
			break;
	}
	
	// num of pages
	m_number_of_pages = screeninfo.yres_virtual / nyRes;
	
	if (ioctl(fd, FBIOPUT_VSCREENINFO, &screeninfo) < 0)
	{
		// try single buffering
		screeninfo.yres_virtual = screeninfo.yres = nyRes;

		if (ioctl(fd, FBIOPUT_VSCREENINFO, &screeninfo) < 0)
		{
			perror("FBIOPUT_VSCREENINFO");
		}
		
		printf("CFrameBuffer::setVideoMode: double buffering not available.\n");
	} 
	else
		printf("CFrameBuffer::setVideoMode: double buffering available!\n");
	
	ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo);

	if ((screeninfo.xres != nxRes) && (screeninfo.yres != nyRes) && (screeninfo.bits_per_pixel != nbpp))
	{
		printf("CFrameBuffer::setVideoMode: failed: wanted: %dx%dx%d, got %dx%dx%d\n", nxRes, nyRes, nbpp, screeninfo.xres, screeninfo.yres, screeninfo.bits_per_pixel);
	}
	
	xRes = screeninfo.xres;
	yRes = screeninfo.yres;
	bpp  = screeninfo.bits_per_pixel;
	
	// stride
	fb_fix_screeninfo fix;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix)<0)
	{
		perror("FBIOGET_FSCREENINFO");
	}

	stride = fix.line_length;
}

int CFrameBuffer::setMode()
{
	if (!available && !active)
		return -1;
	
	dprintf(DEBUG_NORMAL, "CFrameBuffer::setMode: FB: %dx%dx%d\n", DEFAULT_XRES, DEFAULT_YRES, DEFAULT_BPP);

#if defined __sh__ || defined USE_OPENGL
	xRes = DEFAULT_XRES;
	yRes = DEFAULT_YRES;
	bpp = DEFAULT_BPP;
	stride = xRes * 4;
#else
	setFrameBufferMode(DEFAULT_XRES, DEFAULT_YRES, DEFAULT_BPP);
#endif	

	// clear frameBuffer
	paintBackground();
	
#if !defined USE_OPENGL
	blit();
#endif

	return 0;
}

// blend mode: 0=non-premultiplied alpha | 1=premultiplied alpha
void CFrameBuffer::setBlendMode(uint8_t mode)
{
#ifdef __sh__	
	struct stmfbio_var_screeninfo_ex varEx = {0};
	memset(&varEx, 0, sizeof(varEx));

	varEx.layerid  = 0;
	varEx.activate = STMFBIO_ACTIVATE_IMMEDIATE;
	varEx.caps |= STMFBIO_VAR_CAPS_PREMULTIPLIED;
	varEx.premultiplied_alpha = mode;

	if(ioctl(fd, STMFBIO_SET_VAR_SCREENINFO_EX, &varEx) < 0)
		perror("STMFBIO_SET_VAR_SCREENINFO_EX");
#endif
}

// blendlevel (e.g transparency)
void CFrameBuffer::setBlendLevel(int level)
{
#ifdef __sh__
	struct stmfbio_var_screeninfo_ex varEx = {0};
	memset(&varEx, 0, sizeof(varEx));

	varEx.layerid  = 0;
	varEx.activate = STMFBIO_ACTIVATE_IMMEDIATE;
	varEx.caps = STMFBIO_VAR_CAPS_OPACITY;
	varEx.opacity = level;


	if(ioctl(fd, STMFBIO_SET_VAR_SCREENINFO_EX, &varEx) < 0)
		perror("STMFBIO_SET_VAR_SCREENINFO_EX");
#endif	
}

void CFrameBuffer::paletteFade(int i, __u32 rgb1, __u32 rgb2, int level)
{
	__u16 *r = cmap.red+i;
	__u16 *g = cmap.green+i;
	__u16 *b = cmap.blue+i;
	*r= ((rgb2&0xFF0000)>>16)*level;
	*g= ((rgb2&0x00FF00)>>8 )*level;
	*b= ((rgb2&0x0000FF)    )*level;
	*r+=((rgb1&0xFF0000)>>16)*(255-level);
	*g+=((rgb1&0x00FF00)>>8 )*(255-level);
	*b+=((rgb1&0x0000FF))*(255-level);
}

void CFrameBuffer::paletteGenFade(int in, __u32 rgb1, __u32 rgb2, int num, int tr)
{
	for (int i = 0; i < num; i++) 
	{
		paletteFade(in + i, rgb1, rgb2, i*(255/(num-1)));
		cmap.transp[in+i] = tr;
		tr--; //FIXME
	}
}

void CFrameBuffer::paletteSetColor(int i, __u32 rgb, int tr)
{
	cmap.red[i] =(rgb&0xFF0000)>>8;
	cmap.green[i] =(rgb&0x00FF00)   ;
	cmap.blue[i] =(rgb&0x0000FF)<<8;
	cmap.transp[i] = tr;
}

void CFrameBuffer::paletteSet(struct fb_cmap *map)
{
	if (!active)
		return;
	
	if(map == NULL)
		map = &cmap;

	// 32 bit palette
	uint32_t  rl, ro, gl, go, bl, bo, tl, to;
	
        rl = screeninfo.red.length;
        ro = screeninfo.red.offset;
        gl = screeninfo.green.length;
        go = screeninfo.green.offset;
        bl = screeninfo.blue.length;
        bo = screeninfo.blue.offset;
        tl = screeninfo.transp.length;
        to = screeninfo.transp.offset;

	// 256 real color
	for (int i = 0; i < 256; i++) 
	{
                realcolor[i] = make16color(cmap.red[i], cmap.green[i], cmap.blue[i], cmap.transp[i], rl, ro, gl, go, bl, bo, tl, to);
	}
}

void CFrameBuffer::paintBoxRel(const int x, const int y, const int dx, const int dy, const fb_pixel_t col, int radius, int type)
{
    	if (!getActive())
        	return;

    	int F, R = radius, sx, sy, dxx = dx, dyy = dy, rx, ry, wx, wy;

    	uint8_t *pos = ((uint8_t *)getFrameBufferPointer()) + x*sizeof(fb_pixel_t) + stride*y;
	
    	uint8_t *pos0 = 0, *pos1 = 0, *pos2 = 0, *pos3 = 0;

    	fb_pixel_t *dest0, *dest1;

    	if(R) 
	{
        	if(--dyy<=0) 
		{
            		dyy=1;
        	}

        	if(R==1 || R>(dxx/2) || R>(dyy/2)) 
		{
            		R=dxx/10;
            		F=dyy/10;

            		if(R>F) 
			{
                		if(R>(dyy/3)) 
				{
                    			R=dyy/3;
                		}
            		} 
			else 
			{
                		R=F;
                		if(R>(dxx/3)) 
				{
                    			R=dxx/3;
                		}
            		}
        	}
        	
        	sx = 0;
        	sy = R;
        	F = 1 - R;

        	rx = R - sx;
        	ry = R - sy;

		if(type & 1) 
		{
        		pos1=pos+(ry*stride); // top 1
        		pos2=pos+(rx*stride); // top 2
		}

		if(type & 2) 
		{
        		pos0 = pos + ((dyy - ry)*stride); // bottom 1
        		pos3 = pos + ((dyy - rx)*stride); // bottom 2
		}

        	while (sx <= sy) 
		{
            		rx=R-sx;
            		ry=R-sy;
            		wx=rx<<1;
            		wy=ry<<1;
            		dest0=(fb_pixel_t *)(pos0+rx*sizeof(fb_pixel_t));
            		dest1=(fb_pixel_t *)(pos1+rx*sizeof(fb_pixel_t));

            		for (int i=0; i<(dxx-wx); i++) 
			{
				if(type & 2)
                			*(dest0++)=col;	//bottom 1
				if(type & 1)
                			*(dest1++)=col;	// top 1
            		}

            		dest0=(fb_pixel_t *)(pos2+ry*sizeof(fb_pixel_t));
            		dest1=(fb_pixel_t *)(pos3+ry*sizeof(fb_pixel_t));

            		for (int i=0; i<(dxx-wy); i++) 
			{
				if(type & 1)
                			*(dest0++)=col;	// top 2
				if(type & 2)
                			*(dest1++)=col;	//bottom 2
            		}
            		sx++;
            		pos2 -= stride;
            		pos3 += stride;
            		if (F<0) 
			{
                		F += (sx<<1)-1;
            		} 
			else 
			{
                		F += ((sx-sy)<<1);
                		sy--;
                		pos0 -= stride;
                		pos1 += stride;
            		}
        	}
        	
		if(type & 1)
        		pos+=R*stride;
    	}

    	int start = R;
    	int end = dyy - R;

    	if(!(type & 1))
		start = 0;
	
    	if(!(type & 2))
		end = dyy+ (R ? 1 : 0);

    	for (int count = start; count < end; count++) 
	{
        	dest0 = (fb_pixel_t *)pos;
        	for (int i = 0; i<dxx; i++)
            		*(dest0++)=col;
        	pos += stride;
    	}	
}

void CFrameBuffer::paintVLine(int x, int ya, int yb, const fb_pixel_t col)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * ya;

	int dy = yb-ya;
	
	for (int count = 0; count < dy; count++) 
	{
		*(fb_pixel_t *)pos = col;
		pos += stride;
	}	
}

void CFrameBuffer::paintVLineRel(int x, int y, int dy, const fb_pixel_t col)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;

	for(int count=0;count<dy;count++) {
		*(fb_pixel_t *)pos = col;
		pos += stride;
	}	
}

void CFrameBuffer::paintHLine(int xa, int xb, int y, const fb_pixel_t col)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)getFrameBufferPointer()) + xa * sizeof(fb_pixel_t) + stride * y;

	int dx = xb -xa;
	fb_pixel_t * dest = (fb_pixel_t *)pos;
	for (int i = 0; i < dx; i++)
		*(dest++) = col;	
}

void CFrameBuffer::paintHLineRel(int x, int dx, int y, const fb_pixel_t col)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;

	fb_pixel_t * dest = (fb_pixel_t *)pos;
	for (int i = 0; i < dx; i++)
		*(dest++) = col;	
}

void CFrameBuffer::setIconBasePath(const std::string & iconPath)
{
	iconBasePath = iconPath;
}

// get icon size
void CFrameBuffer::getIconSize(const char * const filename, int * width, int * height)
{

	if(filename == NULL)
		return;
	
	int icon_fd;
	
	// png
	std::string iconfile = iconBasePath + filename + ".png";

	icon_fd = open(iconfile.c_str(), O_RDONLY);

	if (icon_fd == -1)
	{
		std::string iconfile1 = iconBasePath + filename + ".raw";
		
		icon_fd = open(iconfile1.c_str(), O_RDONLY);
		
		if (icon_fd == -1)
		{
			//printf("Framebuffer getIconSize: error while loading icon: %s\n", iconfile.c_str());
			return;
		}
		else
		{
			// raw
			struct rawHeader header;
			uint16_t x;
			uint16_t y;
			
			read(icon_fd, &header, sizeof(struct rawHeader));
		
			x = (header.width_hi << 8) | header.width_lo;
			y = (header.height_hi << 8) | header.height_lo;
			
			*width = x;
			*height = y;
		}
		close(icon_fd);
	}
	else
	{
		CFormathandler * fh;
		int x, y;
		
		fh = fh_getsize(iconfile.c_str(), &x, &y, INT_MAX, INT_MAX);
		
		if (fh == NULL) 
		{
			*width = 0;
			*height = 0;
		}
		else
		{
			*width = x;
			*height = y;
		}
	}

	close(icon_fd);
}

bool CFrameBuffer::paintIcon8(const std::string & filename, const int x, const int y, const unsigned char offset)
{
	if (!getActive())
		return false;

	struct rawHeader header;
	uint16_t         width, height;
	int              _fd;

	_fd = open((iconBasePath + filename).c_str(), O_RDONLY);

	if (_fd == -1) 
	{
		printf("CFrameBuffer::paintIcon8: error while loading icon: %s%s\n", iconBasePath.c_str(), filename.c_str());
		return false;
	}

	read(_fd, &header, sizeof(struct rawHeader));

	width  = (header.width_hi  << 8) | header.width_lo;
	height = (header.height_hi << 8) | header.height_lo;

	unsigned char pixbuf[768];

	uint8_t * d = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;
	
	fb_pixel_t * d2;

	for (int count = 0; count < height; count ++ ) 
	{
		read(_fd, &pixbuf[0], width );
		unsigned char *pixpos = &pixbuf[0];
		d2 = (fb_pixel_t *) d;
		for (int count2 = 0; count2 < width; count2 ++ ) 
		{
			unsigned char color = *pixpos;
			
			if (color != header.transp) 
			{
				//printf("icon8: col %d transp %d real %08X\n", color+offset, header.transp, realcolor[color+offset]);
				paintPixel(d2, color + offset);
			}
			d2++;
			pixpos++;
		}
		d += stride;
	}
	close(_fd);

	return true;
}

// paint icon raw
bool CFrameBuffer::paintIconRaw(const std::string & filename, const int x, const int y, const int h, const unsigned char offset, bool paint)
{
	if (!getActive())
		return false;
	
	struct rawHeader header;
	
	int width, height;
	int lfd;
	
	fb_pixel_t * data;
	struct Icon tmpIcon;
	std::map<std::string, Icon>::iterator it;
	int dsize;
	int  yy = y;

	/* we cache and check original name */
	it = icon_cache.find(filename);
	if(it == icon_cache.end()) 
	{
		std::string newname = iconBasePath + filename.c_str() + ".raw";

		lfd = open(newname.c_str(), O_RDONLY);

		if (lfd == -1) 
		{
			dprintf(DEBUG_NORMAL, "paintIcon: error while loading icon: %s\n", newname.c_str());
			return false;
		}
		
		read(lfd, &header, sizeof(struct rawHeader));

		tmpIcon.width = width  = (header.width_hi  << 8) | header.width_lo;
		tmpIcon.height = height = (header.height_hi << 8) | header.height_lo;
		
		dsize = width*height*sizeof(fb_pixel_t);
		tmpIcon.data = (fb_pixel_t*)malloc(dsize);
		data = tmpIcon.data;
		
		unsigned char pixbuf[768];
		for (int count = 0; count < height; count ++ ) 
		{
			read(lfd, &pixbuf[0], width >> 1 );
			unsigned char *pixpos = &pixbuf[0];
			for (int count2 = 0; count2 < width >> 1; count2 ++ ) 
			{
				unsigned char compressed = *pixpos;
				unsigned char pix1 = (compressed & 0xf0) >> 4;
				unsigned char pix2 = (compressed & 0x0f);
				if (pix1 != header.transp)
					*data++ = realcolor[pix1+offset];
				else
					*data++ = 0;
				if (pix2 != header.transp)
					*data++ = realcolor[pix2+offset];
				else
					*data++ = 0;
				pixpos++;
			}
		}
		
		close(lfd);
		
		/* cache it */
		data = tmpIcon.data;

		if(cache_size+dsize < ICON_CACHE_SIZE) 
		{
			cache_size += dsize;
			icon_cache.insert(std::pair <std::string, Icon> (filename, tmpIcon));
			
			//printf("Cached %s, cache size %d\n", newname.c_str(), cache_size);
		}
	} 
	else 
	{
		data = it->second.data;
		width = it->second.width;
		height = it->second.height;
	}
	
	if(!paint)
		return true;

	if (h != 0)
		yy += (h - height) / 2;	

	blit2FB(data, width, height, x, yy);

	return true;
}
//

/* 
* paint icon at position x/y,
* if height h is given, center vertically between y and y+h
* offset is a color offset (probably only useful with palette) 
*/
bool CFrameBuffer::paintIcon(const std::string & filename, const int x, const int y, const int h, const unsigned char /*offset*/, bool paint)
{
	if (!getActive())
		return false;
	
	int width, height;
	
	fb_pixel_t * data;
	struct Icon tmpIcon;
	std::map<std::string, Icon>::iterator it;
	int dsize;
	int  yy = y;

	/* we cache and check original name */
	it = icon_cache.find(filename);
	if(it == icon_cache.end()) 
	{
		std::string newname = iconBasePath + filename.c_str() + ".png";
		
		data = getIcon(newname, &width, &height);
		
		if(!data) 
			data = getIcon(filename, &width, &height);

		if(data) 
		{
			// cache it
			dsize = width*height*sizeof(fb_pixel_t);
			
			if(cache_size + dsize < ICON_CACHE_SIZE) 
			{
				cache_size += dsize;
				tmpIcon.width = width;
				tmpIcon.height = height;
				tmpIcon.data = data;
				icon_cache.insert(std::pair <std::string, Icon> (filename, tmpIcon));
			}
			
			// display icon
			goto _display;
		}
		else
		{
			dprintf(DEBUG_NORMAL, "paintIcon: error while loading icon: %s\n", newname.c_str());
			return false;
		}
	} 
	else 
	{
		data = it->second.data;
		width = it->second.width;
		height = it->second.height;
	}
	
_display:
	if(!paint)
		return true;

	if (h != 0)
		yy += (h - height) / 2;	

	blit2FB(data, width, height, x, yy, 0, 0, true);

	return true;
}

void CFrameBuffer::loadPal(const std::string & filename, const unsigned char offset, const unsigned char endidx)
{
	if (!getActive())
		return;

	struct rgbData rgbdata;
	int            _fd;

	_fd = open((iconBasePath + filename).c_str(), O_RDONLY);

	if (_fd == -1) 
	{
		printf("CFrameBuffer::loadPal: error while loading palette: %s%s\n", iconBasePath.c_str(), filename.c_str());
		return;
	}

	int pos = 0;
	int readb = read(_fd, &rgbdata,  sizeof(rgbdata) );
	
	while(readb) 
	{
		__u32 rgb = (rgbdata.r<<16) | (rgbdata.g<<8) | (rgbdata.b);
		int colpos = offset+pos;
		if( colpos > endidx)
			break;

		paletteSetColor(colpos, rgb, 0xFF);
		readb = read(_fd, &rgbdata,  sizeof(rgbdata) );
		pos++;
	}
	paletteSet(&cmap);
	close(_fd);
}

void CFrameBuffer::paintPixel(const int x, const int y, const fb_pixel_t col)
{
	if (!getActive())
		return;

	fb_pixel_t * pos = getFrameBufferPointer();

	pos += (stride / sizeof(fb_pixel_t)) * y;
	pos += x;

	*pos = col;
}

void CFrameBuffer::paintLine(int xa, int ya, int xb, int yb, const fb_pixel_t col)
{
	if (!getActive())
		return;
	
	int dx = abs (xa - xb);
	int dy = abs (ya - yb);
	int x;
	int y;
	int End;
	int step;

	if ( dx > dy )
	{
		int	p = 2 * dy - dx;
		int	twoDy = 2 * dy;
		int	twoDyDx = 2 * (dy-dx);

		if ( xa > xb )
		{
			x = xb;
			y = yb;
			End = xa;
			step = ya < yb ? -1 : 1;
		}
		else
		{
			x = xa;
			y = ya;
			End = xb;
			step = yb < ya ? -1 : 1;
		}

		paintPixel (x, y, col);

		while( x < End )
		{
			x++;
			if ( p < 0 )
				p += twoDy;
			else
			{
				y += step;
				p += twoDyDx;
			}
			paintPixel (x, y, col);
		}
	}
	else
	{
		int	p = 2 * dx - dy;
		int	twoDx = 2 * dx;
		int	twoDxDy = 2 * (dx-dy);

		if ( ya > yb )
		{
			x = xb;
			y = yb;
			End = ya;
			step = xa < xb ? -1 : 1;
		}
		else
		{
			x = xa;
			y = ya;
			End = yb;
			step = xb < xa ? -1 : 1;
		}

		paintPixel (x, y, col);

		while( y < End )
		{
			y++;
			if ( p < 0 )
				p += twoDx;
			else
			{
				x += step;
				p += twoDxDy;
			}
			paintPixel (x, y, col);
		}
	}
}

void CFrameBuffer::setBackgroundColor(const fb_pixel_t color)
{
	backgroundColor = color;
}

// raw as background image
bool CFrameBuffer::loadPictureToMem(const std::string & filename, const uint16_t width, const uint16_t height, const uint16_t _stride, fb_pixel_t * memp)
{
	struct rawHeader header;
	int              _fd;	

	_fd = open((iconBasePath + filename).c_str(), O_RDONLY );

	if (_fd == -1)
	{
		printf("CFrameBuffer::loadPictureToMem: error while loading icon: %s%s\n", iconBasePath.c_str(), filename.c_str());
		return false;
	}

	read(_fd, &header, sizeof(struct rawHeader));

	if ((width  != ((header.width_hi  << 8) | header.width_lo)) || (height != ((header.height_hi << 8) | header.height_lo)))
	{
		printf("CFrameBuffer::loadPictureToMem: error while loading icon: %s - invalid resolution = %hux%hu\n", filename.c_str(), width, height);
		return false;
	}

	if ((_stride == 0) || (_stride == width * sizeof(fb_pixel_t)))
		read(_fd, memp, height * width * sizeof(fb_pixel_t));
	else
	{
		for (int i = 0; i < height; i++)
			read(_fd, ((uint8_t *)memp) + i * _stride, width * sizeof(fb_pixel_t));
	}

	close(_fd);
	
	return true;
}

bool CFrameBuffer::loadPicture2Mem(const std::string & filename, fb_pixel_t * memp)
{
	return loadPictureToMem(filename, BACKGROUNDIMAGEWIDTH, BACKGROUNDIMAGEHEIGHT, 0, memp);
}

bool CFrameBuffer::loadPicture2FrameBuffer(const std::string & filename)
{
	if (!getActive())
		return false;

	return loadPictureToMem(filename, BACKGROUNDIMAGEWIDTH, BACKGROUNDIMAGEHEIGHT, getStride(), getFrameBufferPointer());
}

bool CFrameBuffer::savePictureFromMem(const std::string & filename, const fb_pixel_t * const memp)
{
	struct rawHeader header;
	uint16_t         width, height;
	int              _fd;
	
	width = BACKGROUNDIMAGEWIDTH;
	height = BACKGROUNDIMAGEHEIGHT;

	header.width_lo  = width  &  0xFF;
	header.width_hi  = width  >>    8;
	header.height_lo = height &  0xFF;
	header.height_hi = height >>    8;
	header.transp    =              0;

	_fd = open((iconBasePath + filename).c_str(), O_WRONLY | O_CREAT);

	if (_fd==-1)
	{
		printf("CFrameBuffer::savePictureFromMem: error while saving icon: %s%s", iconBasePath.c_str(), filename.c_str() );
		return false;
	}

	write(_fd, &header, sizeof(struct rawHeader));

	write(_fd, memp, width * height * sizeof(fb_pixel_t));

	close(_fd);
	
	return true;
}

bool CFrameBuffer::loadBackground(const std::string & filename, const unsigned char offset)
{
	if ((backgroundFilename == filename) && (background))
		return true;

	if (background)
		delete[] background;

	background = new fb_pixel_t[BACKGROUNDIMAGEWIDTH * BACKGROUNDIMAGEHEIGHT];

	if (!loadPictureToMem(filename, BACKGROUNDIMAGEWIDTH, BACKGROUNDIMAGEHEIGHT, 0, background))
	{
		delete[] background;
		background=0;
		return false;
	}

	if (offset != 0)//pic-offset
	{
		fb_pixel_t * bpos = background;

		int pos = BACKGROUNDIMAGEWIDTH * BACKGROUNDIMAGEHEIGHT;
		
		while (pos > 0)
		{
			*bpos += offset;
			bpos++;
			pos--;
		}
	}

	fb_pixel_t * dest = background + BACKGROUNDIMAGEWIDTH * BACKGROUNDIMAGEHEIGHT;
	uint8_t    * src  = ((uint8_t * )background)+ BACKGROUNDIMAGEWIDTH * BACKGROUNDIMAGEHEIGHT;

	for (int i = BACKGROUNDIMAGEHEIGHT - 1; i >= 0; i--)
	{
		for (int j = BACKGROUNDIMAGEWIDTH - 1; j >= 0; j--)
		{
			dest--;
			src--;
			paintPixel(dest, *src);
		}
	}

	backgroundFilename = filename;

	return true;
}
// end raw bg

bool CFrameBuffer::loadBackgroundPic(const std::string & filename, bool show)
{
	if ((backgroundFilename == filename) && (background))
		return true;
	
	dprintf(DEBUG_INFO, "CFrameBuffer::loadBackgroundPic: %s\n", filename.c_str());	

	if (background)
		delete[] background;
	
	// get bg image
	background = getImage(iconBasePath + filename, BACKGROUNDIMAGEWIDTH, BACKGROUNDIMAGEHEIGHT);

	// if not found
	if (background == NULL) 
	{
		background = 0;
		return false;
	}

	backgroundFilename = filename;
	
	if(show) 
	{
		useBackgroundPaint = true;
		paintBackground();
	}
	
	return true;
}

void CFrameBuffer::useBackground(bool ub)
{
	useBackgroundPaint = ub;
	
	if(!useBackgroundPaint) 
	{
		delete[] background;
		background = 0;
	}
}

bool CFrameBuffer::getuseBackground(void)
{
	return useBackgroundPaint;
}

void CFrameBuffer::saveBackgroundImage(void)
{
	if (backupBackground != NULL)
		delete[] backupBackground;

	backupBackground = background;
	//useBackground(false); 		// <- necessary since no background is available
	useBackgroundPaint = false;
	background = NULL;
}

void CFrameBuffer::restoreBackgroundImage(void)
{
	fb_pixel_t * tmp = background;

	if (backupBackground != NULL)
	{
		background = backupBackground;
		backupBackground = NULL;
	}
	else
		useBackground(false); 		// <- necessary since no background is available

	if (tmp != NULL)
		delete[] tmp;
}

void CFrameBuffer::paintBackgroundBoxRel(int x, int y, int dx, int dy)
{
	if (!getActive())
		return;

	if(!useBackgroundPaint)
	{		
		paintBoxRel(x, y, dx, dy, backgroundColor);
	}
	else
	{
		uint8_t * fbpos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;

		fb_pixel_t * bkpos = background + x + BACKGROUNDIMAGEWIDTH * y;

		for(int count = 0;count < dy; count++)
		{
			memcpy(fbpos, bkpos, dx * sizeof(fb_pixel_t));
			fbpos += stride;
			bkpos += BACKGROUNDIMAGEWIDTH;
		}
	}
}

void CFrameBuffer::paintBackground()
{
	if (!getActive())
		return;

	if (useBackgroundPaint && (background != NULL))
	{
		for (int i = 0; i < BACKGROUNDIMAGEHEIGHT; i++)
			memcpy(((uint8_t *)getFrameBufferPointer()) + i * stride, (background + i * BACKGROUNDIMAGEWIDTH), BACKGROUNDIMAGEWIDTH * sizeof(fb_pixel_t));
	}
	else
	{
		paintBoxRel(0, 0, xRes, yRes, backgroundColor);
	}	
}

void CFrameBuffer::SaveScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp)
{
	dprintf(DEBUG_DEBUG, "CFrameBuffer::SaveScreen\n");
	
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;

	fb_pixel_t * bkpos = memp;

	for (int count = 0; count < dy; count++) 
	{
		fb_pixel_t * dest = (fb_pixel_t *)pos;
		for (int i = 0; i < dx; i++)
			//*(dest++) = col;
			*(bkpos++) = *(dest++);
		pos += stride;
	}
}

void CFrameBuffer::RestoreScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp)
{
	dprintf(DEBUG_DEBUG, "CFrameBuffer::RestoreScreen\n");
	
	if (!getActive())
		return;

	uint8_t * fbpos = ((uint8_t *)getFrameBufferPointer()) + x * sizeof(fb_pixel_t) + stride * y;
	
	fb_pixel_t * bkpos = memp;

	for (int count = 0; count < dy; count++)
	{
		memcpy(fbpos, bkpos, dx * sizeof(fb_pixel_t));
		fbpos += stride;
		bkpos += dx;
	}
}

void CFrameBuffer::ClearFrameBuffer()
{
	paintBackground();
}

void * CFrameBuffer::convertRGB2FB(unsigned char * rgbbuff, unsigned long x, unsigned long y, int transp, int m_transparent, bool alpha)
{
	unsigned long i;
	unsigned int * fbbuff;
	unsigned long count = x*y;

	fbbuff = (unsigned int *) malloc(count * sizeof(unsigned int));
	
	if(fbbuff == NULL)
	{
		printf("CFrameBuffer::convertRGB2FB: Error: malloc\n");
		return NULL;
	}
	
	if(alpha)
	{
		for(i = 0; i < count ; i++)
			fbbuff[i] = ((rgbbuff[i*4+3] << 24) & 0xFF000000) | 
			            ((rgbbuff[i*4]   << 16) & 0x00FF0000) | 
		        	    ((rgbbuff[i*4+1] <<  8) & 0x0000FF00) | 
			            ((rgbbuff[i*4+2])       & 0x000000FF);
	}
	else
	{
		switch (m_transparent) 
		{
			case CFrameBuffer::TM_BLACK:
				for(i = 0; i < count ; i++) 
				{
					transp = 0;
					if(rgbbuff[i*3] || rgbbuff[i*3+1] || rgbbuff[i*3+2])
						transp = 0xFF;
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3+1] << 8) & 0xFF00) | (rgbbuff[i*3+2] & 0xFF);
				}
				break;
							
			case CFrameBuffer::TM_INI:
				for(i = 0; i < count ; i++)
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3+1] << 8) & 0xFF00) | (rgbbuff[i*3+2] & 0xFF);
				break;
							
			case CFrameBuffer::TM_NONE:
			default:
				for(i = 0; i < count ; i++)
					fbbuff[i] = 0xFF000000 | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3+1] << 8) & 0xFF00) | (rgbbuff[i*3+2] & 0xFF);
				break;
		}
	}

	return (void *) fbbuff;
}

// blit2fb
void CFrameBuffer::blit2FB(void * fbbuff, uint32_t width, uint32_t height, uint32_t xoff, uint32_t yoff, uint32_t xp, uint32_t yp, bool transp )
{ 
	int xc = (width > xRes) ? xRes : width;
	int yc = (height > yRes) ? yRes : height;

	fb_pixel_t * data = (fb_pixel_t *) fbbuff;

	uint8_t * d = ((uint8_t *)getFrameBufferPointer()) + xoff * sizeof(fb_pixel_t) + stride * yoff;
	fb_pixel_t * d2;

	for (int count = 0; count < yc; count++ ) 
	{
		fb_pixel_t * pixpos = &data[(count + yp) * width];
		d2 = (fb_pixel_t *) d;
		for (int count2 = 0; count2 < xc; count2++ ) 
		{
			fb_pixel_t pix = *(pixpos + xp);
			
			if (!transp || (pix & 0xff000000) == 0xff000000)
				*d2 = pix;
			else //alpha blending
			{
				uint8_t *in = (uint8_t *)(pixpos + xp);
				uint8_t *out = (uint8_t *)d2;
				int a = in[3];	//TODO: big/little endian
				*out = (*out + ((*in - *out) * a) / 256);
				in++; out++;
				*out = (*out + ((*in - *out) * a) / 256);
				in++; out++;
				*out = (*out + ((*in - *out) * a) / 256);
			}
			
			d2++;
			pixpos++;
		}
		d += stride;
	}
}

// display RGB (used by pictureviewer)
void CFrameBuffer::displayRGB(unsigned char * rgbbuff, int x_size, int y_size, int x_pan, int y_pan, int x_offs, int y_offs, bool clearfb, int transp)
{
        void * fbbuff = NULL;

        if(rgbbuff == NULL)
                return;

        /* correct panning */
        if(x_pan > x_size - (int)xRes) 
		x_pan = 0;
        if(y_pan > y_size - (int)yRes) 
		y_pan = 0;

        /* correct offset */
        if(x_offs + x_size > (int)xRes) 
		x_offs = 0;
        if(y_offs + y_size > (int)yRes) 
		y_offs = 0;

        /* convert */
        fbbuff = convertRGB2FB(rgbbuff, x_size, y_size, transp);
        if(fbbuff == NULL)
                return;

        /* ClearFB if image is smaller */
        if(clearfb)
                ClearFrameBuffer();

	// blit2fb
        blit2FB(fbbuff, x_size, y_size, x_offs, y_offs, x_pan, y_pan);
	
        free(fbbuff);
}

// resize.cpp
unsigned char * CFrameBuffer::Resize(unsigned char * origin, int ox, int oy, int dx, int dy, ScalingMode type, unsigned char * dst, bool alpha)
{
	unsigned char * cr;
	
	if(dst == NULL) 
	{
		cr = (unsigned char*) malloc(dx*dy*(alpha? 4 : 3));

		if(cr == NULL)
		{
			printf("Error: malloc\n");
			return(origin);
		}
	} 
	else
		cr = dst;

	if(type == SIMPLE) 
	{
		unsigned char *p, *l;
		int i, j, k, ip;
		l = cr;

		for(j = 0; j < dy; j++, l += dx*3)
		{
			p = origin + (j*oy/dy*ox*3);
			for(i = 0, k = 0; i < dx; i++, k += 3)
			{
				ip = i*ox/dx*3;
				memmove(l+k, p+ip, 3);
			}
		}
	} 
	else 
	{
		unsigned char *p, *q;
		int i, j, k, l, ya, yb;
		int sq, r, g, b, a;

		p = cr;

		int xa_v[dx];
		for(i = 0; i < dx; i++)
			xa_v[i] = i*ox/dx;
		int xb_v[dx+1];
		
		for(i = 0; i < dx; i++)
		{
			xb_v[i] = (i+1)*ox/dx;
			if(xb_v[i] >= ox)
				xb_v[i] = ox - 1;
		}
		
		if (alpha)
		{
			for(j = 0;j < dy;j++)
			{
				ya = j*oy/dy;
				yb = (j + 1)*oy/dy; 
				if(yb >= oy) 
					yb = oy - 1;
				
				for(i = 0; i < dx; i++, p += 4)
				{
					for(l = ya, r = 0, g = 0, b = 0, a = 0, sq = 0; l <= yb; l++)
					{
						q = origin + ((l*ox + xa_v[i])*4);
						for(k = xa_v[i]; k <= xb_v[i]; k++, q += 4, sq++)
						{
							r += q[0]; g += q[1]; b += q[2]; a += q[3];
						}
					}
					p[0] = r/sq; 
					p[1] = g/sq; 
					p[2] = b/sq; 
					p[3] = a/sq;
				}
			}
		}
		else
		{
			for(j = 0; j < dy; j++)
			{
				ya = j*oy/dy;
				yb = (j + 1)*oy/dy; 
				if(yb >= oy) 
					yb = oy - 1;
					
				for(i = 0; i < dx; i++, p += 3)
				{
					for(l = ya, r = 0, g = 0, b = 0, sq = 0; l <= yb; l++)
					{
						q = origin + ((l*ox+xa_v[i])*3);
							
						for(k = xa_v[i]; k <= xb_v[i]; k++, q += 3, sq++)
						{
							r += q[0]; g += q[1]; b += q[2];
						}
					}
					p[0] = r/sq; p[1] = g/sq; p[2] = b/sq;
				}
			}
		}
	}
	
	free(origin);
	
	return(cr);
}

// PNG
extern int fh_png_getsize(const char *name, int *x, int *y, int wanted_width, int wanted_height);
extern int fh_png_load(const char *name, unsigned char **buffer, int* xp, int* yp);
extern int fh_png_id(const char *name);
extern int png_load_ext(const char * name, unsigned char ** buffer, int * xp, int * yp, int * bpp);

// JPG
extern int fh_jpeg_getsize (const char *, int *, int *, int, int);
extern int fh_jpeg_load (const char *, unsigned char **, int *, int *);
extern int fh_jpeg_id (const char *);

// GIF
extern int fh_gif_getsize (const char *, int *, int *, int, int);
extern int fh_gif_load (const char *, unsigned char **, int *, int *);
extern int fh_gif_id (const char *);

// BMP
extern int fh_bmp_getsize (const char *, int *, int *, int, int);
extern int fh_bmp_load (const char *, unsigned char **, int *, int *);
extern int fh_bmp_id (const char *);

// CRW
extern int fh_crw_getsize (const char *, int *, int *, int, int);
extern int fh_crw_load (const char *, unsigned char **, int *, int *);
extern int fh_crw_id (const char *);

void add_format (int (*picsize) (const char *, int *, int *, int, int), int (*picread) (const char *, unsigned char **, int *, int *), int (*id) (const char *))
{
	CFormathandler * fhn;
	fhn = (CFormathandler *) malloc(sizeof (CFormathandler));
	fhn->get_size = picsize;
	fhn->get_pic = picread;
	fhn->id_pic = id;
	fhn->next = fh_root;
	fh_root = fhn;
}

void init_handlers (void)
{
	// add png format
  	add_format (fh_png_getsize, fh_png_load, fh_png_id);
	
	// add jpg format
	add_format (fh_jpeg_getsize, fh_jpeg_load, fh_jpeg_id);
	
	// add gif
	add_format (fh_gif_getsize, fh_gif_load, fh_gif_id);
	
	// add bmp
	add_format (fh_bmp_getsize, fh_bmp_load, fh_bmp_id);
	
	// add crw
	add_format (fh_crw_getsize, fh_crw_load, fh_crw_id);
}

CFormathandler * fh_getsize(const char *name, int *x, int *y, int width_wanted, int height_wanted)
{
	CFormathandler * fh;
	
	for (fh = fh_root; fh != NULL; fh = fh->next) 
	{
		if (fh->id_pic (name))
			if (fh->get_size (name, x, y, width_wanted, height_wanted) == FH_ERROR_OK)
				return (fh);
	}

	return (NULL);
}

fb_pixel_t * CFrameBuffer::getImage(const std::string & name, int width, int height)
{
	int x, y;
	CFormathandler * fh;
	unsigned char * buffer;
	fb_pixel_t * ret = NULL;
	int load_ret;
	int _bpp = 0;

  	fh = fh_getsize(name.c_str(), &x, &y, INT_MAX, INT_MAX);
	
  	if (fh) 
	{
		buffer = (unsigned char *) malloc (x*y*4);
		
		if (buffer == NULL) 
		{
		  	printf ("CFrameBuffer::getImage: Error: malloc\n");
		  	return false;
		}
		
		if ((name.find(".png") == (name.length() - 4)) && (fh_png_id(name.c_str())))
			load_ret = png_load_ext(name.c_str(), &buffer, &x, &y, &_bpp);
		else
			load_ret = fh->get_pic(name.c_str(), &buffer, &x, &y);

		if (load_ret == FH_ERROR_OK) 
		{
			// resize
			if(x != width || y != height)
			{
				// alpha
				if(_bpp == 4)
					buffer = Resize(buffer, x, y, width, height, COLOR, NULL, true);
				else
					buffer = Resize(buffer, x, y, width, height, COLOR);
				
				x = width ;
				y = height;
			} 
			
			// convert RGB2FB
			if( name.find(".png") == (name.length() - 4) )
			{
				// alpha
				if (_bpp == 4)
					ret = (fb_pixel_t *) convertRGB2FB(buffer, x, y, 0, TM_INI, true);
				else
					ret = (fb_pixel_t *)convertRGB2FB(buffer, x, y, convertSetupAlpha2Alpha(g_settings.infobar_alpha));
			}
			else
				ret = (fb_pixel_t *)convertRGB2FB(buffer, x, y, convertSetupAlpha2Alpha(g_settings.infobar_alpha), TM_NONE);
			
			free(buffer);
		} 
		else 
		{
	  		printf ("CFrameBuffer::getImage: Error decoding file %s\n", name.c_str ());
	  		free (buffer);
	  		buffer = NULL;
		}
  	} 
	else
		printf("CFrameBuffer::getImage: Error open file %s\n", name.c_str ());

	return ret;
}

fb_pixel_t * CFrameBuffer::getIcon(const std::string & name, int * width, int * height)
{
	int x, y;
	CFormathandler * fh;
	unsigned char * rgbbuff;
	fb_pixel_t * fbbuff = NULL;
	int _bpp = 0;

  	fh = fh_getsize(name.c_str(), &x, &y, INT_MAX, INT_MAX);
	
  	if (!fh) 
	{
		return NULL;
	}
	
	rgbbuff = (unsigned char *) malloc (x*y*4);
	
	if (rgbbuff == NULL) 
	{
		printf ("CFrameBuffer::getIcon: Error: malloc\n");
		return NULL;
	}
	
	int load_ret = png_load_ext(name.c_str(), &rgbbuff, &x, &y, &_bpp);
	
	if(load_ret == FH_ERROR_OK)
	{
		// convert RGB2FB
		// with alpha
		if (_bpp == 4)
			fbbuff = (fb_pixel_t *) convertRGB2FB(rgbbuff, x, y, 0, TM_INI, true);
		else
			fbbuff = (fb_pixel_t *) convertRGB2FB(rgbbuff, x, y, convertSetupAlpha2Alpha(g_settings.infobar_alpha));

		// size
		*width = x;
		*height = y;
	} 
	else 
		printf ("Error decoding file %s\n", name.c_str ());

	free (rgbbuff);

	return fbbuff;
}

#if !defined USE_OPENGL

#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, __u32)
#endif

#ifndef FBIO_BLIT
#define FBIO_SET_MANUAL_BLIT _IOW('F', 0x21, __u8)
#define FBIO_BLIT 0x22
#endif

void CFrameBuffer::enableManualBlit()
{
	unsigned char tmp = 1;
	
	if (ioctl(fd,FBIO_SET_MANUAL_BLIT, &tmp) < 0) 
	{
		perror("FB: FBIO_SET_MANUAL_BLIT");
		printf("FB: failed\n");
	}
	else 
	{
		m_manual_blit = 1;
	}
}

void CFrameBuffer::disableManualBlit()
{
	unsigned char tmp = 0;
	
	if (ioctl(fd,FBIO_SET_MANUAL_BLIT, &tmp) < 0) 
	{
		perror("FB: FBIO_SET_MANUAL_BLIT");
		printf("FB: failed\n");
	}
	else 
	{
		m_manual_blit = 0;
	}
}

void CFrameBuffer::blit()
{
#ifdef __sh__  
	STMFBIO_BLT_DATA  bltData; 
	memset(&bltData, 0, sizeof(STMFBIO_BLT_DATA)); 

	bltData.operation  = BLT_OP_COPY;
	bltData.ulFlags |= BLT_OP_FLAGS_BLEND_SRC_ALPHA | BLT_OP_FLAGS_BLEND_DST_COLOR;

	// src
	bltData.srcOffset  = 1920 *1080 * 4;
	bltData.srcPitch   = xRes * 4; // stride

	bltData.src_left   = 0; 
	bltData.src_top    = 0; 
	bltData.src_right  = xRes; 
	bltData.src_bottom = yRes;

		
	bltData.srcFormat = SURF_ARGB8888;
	bltData.srcMemBase = STMFBGP_FRAMEBUFFER;
	
	// get variable screeninfo
	if (ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo) == -1)
	{
		perror("frameBuffer <FBIOGET_VSCREENINFO>");
	}

	/* dst */
	bltData.dstOffset  = 0; 
	bltData.dstPitch   = screeninfo.xres * 4;

	bltData.dst_left   = 0; 
	bltData.dst_top    = 0; 
	bltData.dst_right  = screeninfo.xres; 
	bltData.dst_bottom = screeninfo.yres;

	bltData.dstFormat = SURF_ARGB8888;		
	bltData.dstMemBase = STMFBGP_FRAMEBUFFER;

	if ( (bltData.dst_right > screeninfo.xres) || (bltData.dst_bottom > screeninfo.yres) )
	{
		printf("CFrameBuffer::blit: values out of range desXb:%d desYb:%d\n", bltData.dst_right, bltData.dst_bottom);
	}

	if (ioctl(fd, STMFBIO_BLT, &bltData ) < 0) 
		perror("STMFBIO_BLIT");
	
	// sync bliter
	if(ioctl(fd, STMFBIO_SYNC_BLITTER) < 0)
		perror("ioctl STMFBIO_SYNC_BLITTER");
#else
	// blit
	if (m_manual_blit == 1) 
	{
		if (ioctl(fd, FBIO_BLIT) < 0)
			perror("FBIO_BLIT");	
	}
#endif
}
#endif





