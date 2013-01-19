/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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


#ifndef __framebuffer__
#define __framebuffer__

#include <config.h>

#include <linux/fb.h>
#include <linux/vt.h>

#include <stdint.h>
#include <string>
#include <map>

// stmfb
#ifdef __sh__
#include <linux/stmfb.h>
#endif

#ifdef USE_OPENGL
class GLThreadObj;
#endif

// 32 bit
#define fb_pixel_t uint32_t		// unsigned int 32 bit data

typedef struct fb_var_screeninfo t_fb_var_screeninfo;

#define CORNER_TOP		0x1
#define CORNER_BOTTOM		0x2
#define CORNER_BOTH		0x3

// resolution
#define DEFAULT_XRES		960
#define DEFAULT_YRES		720

// bitmap
#define DEFAULT_BPP		32	// 32 bit


// Ausf�hrung als Singleton
class CFrameBuffer
{
	// png/jpg/bmp/gif/crw
	struct cformathandler 
	{
		struct cformathandler * next;
		int (*get_size)(const char *,int *,int*, int, int);
		int (*get_pic)(const char *,unsigned char **,int* ,int*);
		int (*id_pic)(const char *);
	};
	
	typedef struct cformathandler CFormathandler;
	
	private:
		CFrameBuffer();

		struct rgbData
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
		} __attribute__ ((packed));

		struct rawHeader
		{
			uint8_t width_lo;
			uint8_t width_hi;
			uint8_t height_lo;
			uint8_t height_hi;
			uint8_t transp;
		} __attribute__ ((packed));
		
		// icon
		struct Icon
		{
			uint16_t width;
			uint16_t height;
			uint8_t transp;
			fb_pixel_t * data;
		};

		std::string     iconBasePath;

		int             fd;
		fb_pixel_t *    lfb;
		int		available;
		fb_pixel_t *    background;
		fb_pixel_t *    backupBackground;
		fb_pixel_t      backgroundColor;
		std::string     backgroundFilename;
		bool            useBackgroundPaint;
		unsigned int	xRes, yRes, stride, bpp;

		t_fb_var_screeninfo screeninfo, oldscreen;

		fb_cmap cmap;
		__u16 red[256], green[256], blue[256], trans[256];

		bool	active;

		// icon cache map
		std::map<std::string, Icon> icon_cache;
		int cache_size;
		
		int m_number_of_pages;
		int m_manual_blit;
		
		// png/jpg/bmp/gif/crw
		CFormathandler * fh_root;
		CFormathandler * fh_getsize(const char *name,int *x,int *y, int width_wanted, int height_wanted);
		void init_handlers(void);
		void add_format(int (*picsize)(const char *,int *,int*,int,int),int (*picread)(const char *,unsigned char **,int*,int*), int (*id)(const char*));
		
#ifdef USE_OPENGL
		GLThreadObj *mpGLThreadObj; /* the thread object */
#endif		

	public:
		// 16/32 bits
		fb_pixel_t realcolor[256];

		~CFrameBuffer();

		static CFrameBuffer* getInstance();

#if !defined USE_OPENGL
		void enableManualBlit();
		void disableManualBlit();
		void blit();
#endif

		void init(const char * const fbDevice = "/dev/fb0");		
		void setFrameBufferMode(unsigned int xRes, unsigned int yRes, unsigned int bpp);
		int setMode();

		int getFileHandle() const; //only used for plugins (games) !!
		t_fb_var_screeninfo *getScreenInfo();

		fb_pixel_t * getFrameBufferPointer() const; // pointer to framebuffer
		unsigned int getStride() const;             // size of a single line in the framebuffer (in bytes)
		unsigned int getScreenWidth(bool real = false);
		unsigned int getScreenHeight(bool real = false); 
		unsigned int getScreenX();
		unsigned int getScreenY();
		unsigned int getAvailableMem() const;             // size of a available mem occupied by the framebuffer
		
		bool getActive() const;                     // is framebuffer active?
		void setActive(bool enable);                // is framebuffer active?

		void setBlendMode(uint8_t mode);
		void setBlendLevel(int blev);
		void setColorGain( int value);
		void setBrightness( int value);
		void setSaturation( int value);
		void setContrast( int value);

		/* Palette stuff */
		void paletteFade(int i, __u32 rgb1, __u32 rgb2, int level);
		void paletteGenFade(int in, __u32 rgb1, __u32 rgb2, int num, int tr=0);
		
		void paletteSetColor(int i, __u32 rgb, int tr);
		void paletteSet(struct fb_cmap *map = NULL);

		/* paint functions */
		inline void paintPixel(fb_pixel_t * const dest, const uint8_t color) const
		{			
			/* 16/32 bit */
			*dest = realcolor[color];
		};

		void paintPixel(const int x, const int y, const fb_pixel_t col);
		
		void paintBoxRel(const int x, const int y, const int dx, const int dy, const fb_pixel_t col, int radius = 0, int type = 0);

		inline void paintBox(int xa, int ya, int xb, int yb, const fb_pixel_t col) { paintBoxRel(xa, ya, xb - xa, yb - ya, col); }
		inline void paintBox(int xa, int ya, int xb, int yb, const fb_pixel_t col, int radius, int type) { paintBoxRel(xa, ya, xb - xa, yb - ya, col, radius, type); }

		void paintLine(int xa, int ya, int xb, int yb, const fb_pixel_t col);

		void paintVLine(int x, int ya, int yb, const fb_pixel_t col);
		void paintVLineRel(int x, int y, int dy, const fb_pixel_t col);

		void paintHLine(int xa, int xb, int y, const fb_pixel_t col);
		void paintHLineRel(int x, int dx, int y, const fb_pixel_t col);

		void setIconBasePath(const std::string & iconPath);
		
		void getIconSize(const char * const filename, int* width, int *height);
		
		bool paintIcon8(const std::string & filename, const int x, const int y, const unsigned char offset = 0);

		bool paintIconRaw(const std::string & filename, const int x, const int y, const int h = 0, const unsigned char offset = 1, bool paint = true);
		
		/* h is the height of the target "window", if != 0 the icon gets centered in that window */
		bool paintIcon(const std::string & filename, const int x, const int y, const int h = 0, const unsigned char offset = 1, bool paint = true);
		
		void loadPal(const std::string & filename, const unsigned char offset = 0, const unsigned char endidx = 255);
		
		bool loadPicture2Mem        (const std::string & filename, fb_pixel_t * const memp);
		bool loadPicture2FrameBuffer(const std::string & filename);
		bool loadPictureToMem       (const std::string & filename, const uint16_t width, const uint16_t height, const uint16_t stride, fb_pixel_t * const memp);
		bool savePictureFromMem     (const std::string & filename, const fb_pixel_t * const memp);
		bool loadBackground(const std::string & filename, const unsigned char col = 0);

		int getBackgroundColor() { return backgroundColor;}
		void setBackgroundColor(const fb_pixel_t color);
		void useBackground(bool);
		bool getuseBackground(void);

		void saveBackgroundImage(void);  // <- implies useBackground(false);
		void restoreBackgroundImage(void);

		void paintBackgroundBoxRel(int x, int y, int dx, int dy);
		inline void paintBackgroundBox(int xa, int ya, int xb, int yb) { paintBackgroundBoxRel(xa, ya, xb - xa, yb - ya); }

		void paintBackground();

		void SaveScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp);
		void RestoreScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp);

		void ClearFrameBuffer();
		
		bool loadBackgroundPic(const std::string & filename, bool show = true);
		
		enum 
		{
			TM_EMPTY  = 0,
			TM_NONE   = 1,
			TM_BLACK  = 2,
			TM_INI    = 3
		};
		
		void * convertRGB2FB(unsigned char *rgbbuff, unsigned long x, unsigned long y, int transp = 0xFF, int m_transparent = TM_NONE, bool alpha = false);
		void blit2FB(void *fbbuff, uint32_t width, uint32_t height, uint32_t xoff, uint32_t yoff, uint32_t xp = 0, uint32_t yp = 0, bool transp = false);
		void displayRGB(unsigned char *rgbbuff, int x_size, int y_size, int x_pan, int y_pan, int x_offs, int y_offs, bool clearfb = true, int transp = 0xFF);
		
		enum ScalingMode
		{
			NONE = 0,
			SIMPLE = 1,
			COLOR = 2
		};
		
		unsigned char * Resize(unsigned char *orgin, int ox, int oy, int dx, int dy, ScalingMode type, unsigned char * dst = NULL);
		fb_pixel_t * getImage (const std::string & name, int width, int height, int m_transparent = TM_NONE);
		fb_pixel_t * getIcon (const std::string & name, int *width, int *height);
};

#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1		/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */
#define FH_ERROR_MALLOC 3	/* error during malloc */

#endif
