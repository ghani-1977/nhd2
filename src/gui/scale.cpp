#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <global.h>
#include <neutrino.h>
//#include <gui/customcolor.h>
#include <gui/scale.h>

#include <string>

#include <time.h>
#include <sys/timeb.h>
#include <sys/param.h>

#define RED_BAR 40
#define YELLOW_BAR 70
#define GREEN_BAR 100

#define BAR_BORDER 1
#define BARW 2
#define BARWW 2

#define ITEMW 4
#define POINT 2

#define RED    0xFF0000
#define GREEN  0x00FF00
#define YELLOW 0xFFFF00

#if 1
#define WHITE  0xFFFFFF
#endif


inline unsigned int make16color(__u32 rgb)
{
        return 0xFF000000 | rgb;
}

CScale::CScale (int w, int h, int r, int g, int b, bool inv)
{
	//printf("new SCALE, w %d h %d size %d\n", w, h, sizeof(CScale)); fflush(stdout);
	
	frameBuffer = CFrameBuffer::getInstance ();
	
	double div;
	width = w;
	height = h;
	inverse = inv;
	
	div = (double) 100 / (double) width;
	red = (double) r / (double) div / (double) ITEMW;
	green = (double) g / (double) div / (double) ITEMW;
	yellow = (double) b / (double) div / (double) ITEMW;
	
	percent = 255;
}

void CScale::paint (int x, int y, int pcr)
{
	int i, j, siglen;
	int posx, posy;
	int xpos, ypos;
	//int hcnt = height / ITEMW;
	int hcnt = height;
	double div;
	uint32_t  rgb;
	
	fb_pixel_t color;
	int b = 0;
	
	i = 0;
	xpos = x;
	ypos = y;
	
	//printf("CScale::paint: old %d new %d x %d y %d\n", percent, pcr, x, y); fflush(stdout);
	
	if (pcr != percent) 
	{
		if(percent == 255) 
			percent = 0;

		div = (double) 100 / (double) width;
		siglen = (double) pcr / (double) div;
		posx = xpos;
		posy = ypos;
		int maxi = siglen / ITEMW;
		int total = width / ITEMW;
		int step = 255/total;

		if (pcr > percent) 
		{
			//red
			for (i = 0; (i < red) && (i < maxi); i++) 
			{
				step = 255/red;

				if(inverse) 
					rgb = GREEN + ((unsigned char)(step*i) << 16); // adding red
				else
					rgb = RED   + ((unsigned char)(step*i) <<  8); // adding green
				
				color = make16color(rgb);
				
				frameBuffer->paintBoxRel (posx + i*ITEMW, posy /*+ j*ITEMW*/, POINT, height/*POINT*/, color);
			}
	
			//yellow
			for (; (i < yellow) && (i < maxi); i++) 
			{
				step = 255/yellow/2;

				if(inverse) 
					rgb = YELLOW - (((unsigned char)step*(b++)) <<  8); // removing green
				else
					rgb = YELLOW - ((unsigned char)(step*(b++)) << 16); // removing red
	
				color = make16color(rgb);		    
				
				frameBuffer->paintBoxRel (posx + i*ITEMW, posy /*+ j*ITEMW*/, POINT, height/*POINT*/, color);
			}

			//green
			for (; (i < green) && (i < maxi); i++) 
			{
				step = 255/green;

				if(inverse) 
					rgb = YELLOW - ((unsigned char) (step*(b++)) <<  8); // removing green
				else
					rgb = YELLOW - ((unsigned char) (step*(b++)) << 16); // removing red
				
				color = make16color(rgb);
				
				frameBuffer->paintBoxRel (posx + i*ITEMW, posy /*+ j*ITEMW*/, POINT, height/*POINT*/, color);
			}
		}
		
		for(i = maxi; i < total; i++) 
		{
			frameBuffer->paintBoxRel (posx + i*ITEMW, posy, ITEMW, height, COL_INFOBAR_PLUS_1);	//fill passive
		}
		
		percent = pcr;
	}
}

void CScale::reset()
{
  	percent = 255;
}

void CScale::hide()
{
}
