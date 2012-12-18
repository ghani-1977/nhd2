/*
	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
        baseroutines by Shadow_
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

#include <config.h>
#include "lcddisplay.h"

#include <png.h>

#include <stdint.h> /* uint8_t */
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>


void CLCDDisplay::setSize(int xres, int yres, int bpp)
{
	_buffer = new unsigned char[xres * yres * bpp/8];
	memset(_buffer, 0, xres*yres*bpp/8);
	_stride = xres*bpp/8;
	printf("[CLCDDisplay] %s lcd buffer %p %d bytes, stride %d\n", __FUNCTION__, _buffer, xres*yres*bpp/8, _stride);
}

CLCDDisplay::CLCDDisplay()
{
	paused = 0;
	available = false;
	
	//e2
	xres = 132;
	yres = 64; 
	bpp = 8;
	
	flipped = false;
	inverted = 0;
	is_oled = 0;
	
	//open device
	fd = open("/dev/dbox/oled0", O_RDWR);
	
	if (fd < 0)
	{
		if (!access("/proc/stb/lcd/oled_brightness", W_OK) || !access("/proc/stb/fp/oled_brightness", W_OK) )
			is_oled = 2;
		fd = open(LCD_DEVICE, O_RDWR);
	} 
	else
	{
		printf("found OLED display!\n");
		is_oled = 1;
	}
	
	if (fd < 0)
	{
		printf("couldn't open LCD - load lcd.ko!\n");
		return;
	}
	else
	{
		int i = LCD_MODE_BIN;
		ioctl(fd, LCD_IOCTL_ASC_MODE, &i);
		
		FILE *f = fopen("/proc/stb/lcd/xres", "r");
		if (f)
		{
			int tmp;
			if (fscanf(f, "%x", &tmp) == 1)
				xres = tmp;
			fclose(f);
			f = fopen("/proc/stb/lcd/yres", "r");
			if (f)
			{
				if (fscanf(f, "%x", &tmp) == 1)
					yres = tmp;
				fclose(f);
				f = fopen("/proc/stb/lcd/bpp", "r");
				if (f)
				{
					if (fscanf(f, "%x", &tmp) == 1)
						bpp = tmp;
					fclose(f);
				}
			}
			is_oled = 3;
		}
	}
	  
	setSize(xres, yres, bpp);
	// end-e2
	  
	#if 0
	if ((fd = open(LCD_DEVICE, O_RDWR)) < 0)
	{
		printf("[lcddisplay] LCD (" LCD_DEVICE ") failed (%m)\n");
		return;
	}
	
	//clear the display
	if( ioctl(fd, LCD_IOCTL_CLEAR) < 0 )
		printf("[lcddisplay] LCD_IOCTL_CLEAR failed (%m)\n");
	
	//graphic (binary) mode 
	int i=LCD_MODE_BIN;
	if( ioctl(fd, LCD_IOCTL_ASC_MODE, &i) < 0 )
		printf("[lcddisplay] LCD_IOCTL_ASC_MODE failed (%m)\n");
	#endif
	//

	available = true;
	iconBasePath = "";
}

//e2
void CLCDDisplay::setInverted(unsigned char inv)
{
	inverted = inv;
	update();
}

void CLCDDisplay::setFlipped(bool onoff)
{
	flipped = onoff;
	update();
}

int CLCDDisplay::setLCDContrast(int contrast)
{
	int fp;
	if((fp=open("/dev/dbox/fp0", O_RDWR))<0)
	{
		printf("[LCD] can't open /dev/dbox/fp0(%m)\n");
		return(-1);
	}

	if(ioctl(fd, LCD_IOCTL_SRV, &contrast) < 0)
	{
		printf("[LCD] can't set lcd contrast(%m)\n");
	}
	close(fp);

	return(0);
}

int CLCDDisplay::setLCDBrightness(int brightness)
{
	printf("setLCDBrightness %d\n", brightness);
	FILE *f=fopen("/proc/stb/lcd/oled_brightness", "w");
	if (!f)
		f = fopen("/proc/stb/fp/oled_brightness", "w");
	if (f)
	{
		if (fprintf(f, "%d", brightness) == 0)
			printf("write /proc/stb/lcd/oled_brightness failed!! (%m)\n");
		fclose(f);
	}
	else
	{
		int fp;
		if((fp=open("/dev/dbox/fp0", O_RDWR)) < 0)
		{
			printf("[LCD] can't open /dev/dbox/fp0\n");
			return(-1);
		}

		if(ioctl(fp, FP_IOCTL_LCD_DIMM, &brightness) < 0)
			printf("[LCD] can't set lcd brightness (%m)\n");
		close(fp);
	}

	return(0);
}
//end-e2

bool CLCDDisplay::isAvailable()
{
	return available;
}

CLCDDisplay::~CLCDDisplay()
{
	delete [] _buffer;
	
	if (fd >= 0)
	{
		close(fd);
		fd = -1;
	}
}

void CLCDDisplay::pause()
{
	paused = 1;
}

void CLCDDisplay::resume()
{
	//clear the display
	if( ioctl(fd, LCD_IOCTL_CLEAR) < 0 )
		printf("[lcddisplay] LCD_IOCTL_CLEAR failed (%m)\n");
	
	//graphic (binary) mode 
	int i=LCD_MODE_BIN;
	if( ioctl(fd, LCD_IOCTL_ASC_MODE, &i) < 0 )
		printf("[lcddisplay] LCD_IOCTL_ASC_MODE failed (%m)\n");
	//
	
	paused = 0;
}

void CLCDDisplay::convert_data ()
{
#ifndef PLATFORM_GENERIC
	unsigned int x, y, z;
	char tmp;

	for (x = 0; x < LCD_COLS; x++)
	{   
		for (y = 0; y < LCD_ROWS; y++)
		{
			tmp = 0;

			for (z = 0; z < 8; z++)
				if (raw[y * 8 + z][x] == 1)
					tmp |= (1 << z);

			lcd[y][x] = tmp;
		}
	}
#endif
}

void CLCDDisplay::update()
{
#ifndef PLATFORM_GENERIC
	#if 0
	convert_data();
	
	if(paused || !available)
		return;
	else
	{
		if ( write(fd, lcd, LCD_BUFFER_SIZE) < 0) 
			printf("lcdd: CLCDDisplay::update(): write() failed (%m)\n");
	}
	#else
	if (fd >= 0)
	{
		if (is_oled == 0 || is_oled == 2)
		{
			unsigned char raw[132*8];
			int x, y, yy;
			for (y=0; y<8; y++)
			{
				for (x=0; x<132; x++)
				{
					int pix=0;
					for (yy=0; yy<8; yy++)
					{
						pix|=(_buffer[(y*8+yy)*132+x]>=108)<<yy;
					}
					
					if (flipped)
					{
						/* 8 pixels per byte, swap bits */
#define BIT_SWAP(a) (( ((a << 7)&0x80) + ((a << 5)&0x40) + ((a << 3)&0x20) + ((a << 1)&0x10) + ((a >> 1)&0x08) + ((a >> 3)&0x04) + ((a >> 5)&0x02) + ((a >> 7)&0x01) )&0xff)
						raw[(7 - y) * 132 + (131 - x)] = BIT_SWAP(pix ^ inverted);
					}
					else
					{
						raw[y * 132 + x] = pix ^ inverted;
					}
				}
			}
			
			write(fd, raw, 132*8);
		}
		else if (is_oled == 3)
		{
			/* for now, only support flipping / inverting for 8bpp displays */
			if ((flipped || inverted) && _stride == xres)
			{
				unsigned int height = yres;
				unsigned int width = xres;
				unsigned char raw[_stride * height];
				
				for (unsigned int y = 0; y < height; y++)
				{
					for (unsigned int x = 0; x < width; x++)
					{
						if (flipped)
						{
							/* 8bpp, no bit swapping */
							raw[(height - 1 - y) * width + (width - 1 - x)] = _buffer[y * width + x] ^ inverted;
						}
						else
						{
							raw[y * width + x] = _buffer[y * width + x] ^ inverted;
						}
					}
				}
				write(fd, raw, _stride * height);
			}
			else
			{
				write(fd, _buffer, _stride * yres);
			}
		}
		else /* is_oled == 1 */
		{
			unsigned char raw[64*64];
			int x, y;
			memset(raw, 0, 64*64);
			for (y=0; y<64; y++)
			{
				int pix=0;
				for (x=0; x<128 / 2; x++)
				{
					pix = (_buffer[y*132 + x * 2 + 2] & 0xF0) |(_buffer[y*132 + x * 2 + 1 + 2] >> 4);
					if (inverted)
						pix = 0xFF - pix;
					if (flipped)
					{
						/* device seems to be 4bpp, swap nibbles */
						unsigned char byte;
						byte = (pix >> 4) & 0x0f;
						byte |= (pix << 4) & 0xf0;
						raw[(63 - y) * 64 + (63 - x)] = byte;
					}
					else
					{
						raw[y * 64 + x] = pix;
					}
				}
			}
			write(fd, raw, 64*64);
		}
	}
	#endif
#endif
}

void CLCDDisplay::draw_point(const int x, const int y, const int state)
{
	if ((x < 0) || (x >= LCD_COLS) || (y < 0) || (y >= (LCD_ROWS * 8)))
		return;

	if (state == LCD_PIXEL_INV)
		raw[y][x] ^= 1;
	else
		raw[y][x] = state;
}

/*
 * draw_line
 * 
 * args:
 * x1    StartCol
 * y1    StartRow
 * x2    EndCol
 * y1    EndRow
 * state LCD_PIXEL_OFF/LCD_PIXEL_ON/LCD_PIXEL_INV
 * 
 */
void CLCDDisplay::draw_line(const int x1, const int y1, const int x2, const int y2, const int state)  
{
	int dx = abs (x1 - x2);
	int dy = abs (y1 - y2);
	int x;
	int y;
	int End;
	int step;

	if ( dx > dy )
	{
		int	p = 2 * dy - dx;
		int	twoDy = 2 * dy;
		int	twoDyDx = 2 * (dy-dx);

		if ( x1 > x2 )
		{
			x = x2;
			y = y2;
			End = x1;
			step = y1 < y2 ? -1 : 1;
		}
		else
		{
			x = x1;
			y = y1;
			End = x2;
			step = y2 < y1 ? -1 : 1;
		}

		draw_point(x, y, state);

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
			draw_point(x, y, state);
		}
	}
	else
	{
		int	p = 2 * dx - dy;
		int	twoDx = 2 * dx;
		int	twoDxDy = 2 * (dx-dy);

		if ( y1 > y2 )
		{
			x = x2;
			y = y2;
			End = y1;
			step = x1 < x2 ? -1 : 1;
		}
		else
		{
			x = x1;
			y = y1;
			End = y2;
			step = x2 < x1 ? -1 : 1;
		}

		draw_point(x, y, state);

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
			draw_point(x, y, state);
		}
	}
}

void CLCDDisplay::draw_fill_rect (int left,int top,int right,int bottom,int state) 
{
	int x,y;
	for(x = left + 1;x < right;x++) 
	{  
		for(y = top + 1;y < bottom;y++) 
		{
			draw_point(x,y,state);
		}
	}
}

void CLCDDisplay::draw_rectangle (int left,int top, int right, int bottom, int linestate,int fillstate)
{
	// coordinate checking in draw_pixel (-> you can draw lines only
	// partly on screen)

	draw_line(left,top,right,top,linestate);
	draw_line(left,top,left,bottom,linestate);
	draw_line(right,top,right,bottom,linestate);
	draw_line(left,bottom,right,bottom,linestate);
	draw_fill_rect(left,top,right,bottom,fillstate);  
}  

void CLCDDisplay::draw_polygon(int num_vertices, int *vertices, int state) 
{

	// coordinate checking in draw_pixel (-> you can draw polygons only
	// partly on screen)

	int i;
	for(i=0;i<num_vertices-1;i++) 
	{
		draw_line(vertices[(i<<1)+0],
			vertices[(i<<1)+1],
			vertices[(i<<1)+2],
			vertices[(i<<1)+3],
			state);
	}
   
	draw_line(vertices[0],
		vertices[1],
		vertices[(num_vertices<<1)-2],
		vertices[(num_vertices<<1)-1],
		state);
}

struct rawHeader
{
	uint8_t width_lo;
	uint8_t width_hi;
	uint8_t height_lo;
	uint8_t height_hi;
	uint8_t transp;
} __attribute__ ((packed));

bool CLCDDisplay::paintIcon(std::string filename, int x, int y, bool invert)
{
	struct rawHeader header;
	uint16_t         stride;
	uint16_t         height;
	unsigned char *  pixpos;

	int _fd;
	filename = iconBasePath + filename;

	_fd = open(filename.c_str(), O_RDONLY );
	
	if (_fd==-1)
	{
		printf("\nerror while loading icon: %s\n\n", filename.c_str() );
		return false;
	}

	read(_fd, &header, sizeof(struct rawHeader));

	stride = ((header.width_hi << 8) | header.width_lo) >> 1;
	height = (header.height_hi << 8) | header.height_lo;

	unsigned char pixbuf[200];
	while (height-- > 0)
	{
		read(fd, &pixbuf, stride);
		pixpos = (unsigned char*) &pixbuf;
		for (int count2 = 0; count2 < stride; count2++)
		{
			unsigned char compressed = *pixpos;

			draw_point(x + (count2 << 1)    , y, ((((compressed & 0xf0) >> 4) != header.transp) ^ invert) ? PIXEL_ON : PIXEL_OFF);
			draw_point(x + (count2 << 1) + 1, y, (( (compressed & 0x0f)       != header.transp) ^ invert) ? PIXEL_ON : PIXEL_OFF);

			pixpos++;
		}
		y++;
	}
	
	close(_fd);
	return true;
}

void CLCDDisplay::dump_screen(raw_display_t *screen) {
	memmove(screen, raw, sizeof(raw_display_t));
}

void CLCDDisplay::load_screen(const raw_display_t * const screen) 
{
	memmove(raw, screen, sizeof(raw_display_t));
}

bool CLCDDisplay::load_png(const char * const filename)
{
	png_structp  png_ptr;
	png_infop    info_ptr;
	unsigned int i;
	unsigned int pass;
	unsigned int number_passes;
	int          bit_depth;
	int          color_type;
	int          interlace_type;
	png_uint_32  width;
	png_uint_32  height;
	png_byte *   fbptr;
	FILE *       fh;
	bool         ret_value = false;

	if ((fh = fopen(filename, "rb")))
	{
		if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
		{
			if (!(info_ptr = png_create_info_struct(png_ptr)))
				png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
			else
			{
#if (PNG_LIBPNG_VER < 10500)
				if (!(setjmp(png_ptr->jmpbuf)))
#else
				if (!setjmp(png_jmpbuf(png_ptr)))
#endif
				{
					png_init_io(png_ptr,fh);
					
					png_read_info(png_ptr, info_ptr);
					png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
					
					if (
						(color_type == PNG_COLOR_TYPE_PALETTE) &&
						(bit_depth  == 1                     ) &&
						(width      <= LCD_COLS              ) &&
						(height     == (LCD_ROWS * 8))
						)
					{
						png_set_packing(png_ptr); /* expand to 1 byte blocks */
						
						number_passes = png_set_interlace_handling(png_ptr);
						png_read_update_info(png_ptr,info_ptr);
						
						if (width == png_get_rowbytes(png_ptr, info_ptr))
						{
							ret_value = true;
							
							for (pass = 0; pass < number_passes; pass++)
							{
								fbptr = (png_byte *)raw;
								for (i = 0; i < height; i++, fbptr += LCD_COLS)
								{
									png_read_row(png_ptr, fbptr, NULL);
									/* if the PNG is smaller, than the display width... */
									if (width < LCD_COLS)	/* clear the area right of the PNG */
										memset(fbptr + width, 0, LCD_COLS - width);
								}
							}
							png_read_end(png_ptr, info_ptr);
						}
					}
				}
				png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
			}
		}
		fclose(fh);
	}
	return ret_value;
}
