#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <global.h>
#include <neutrino.h>

#include <progressbar.h>

#include <string>

#include <time.h>
#include <sys/timeb.h>
#include <sys/param.h>

#include <system/debug.h>


#define ITEMW 4


CProgressBar::CProgressBar(int w, int h, int r, int g, int b, bool inv)
{
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

void CProgressBar::paint (unsigned int x, unsigned int y, unsigned char pcr)
{
	int i, siglen;
	unsigned int posx;
	unsigned int posy;
	unsigned int xpos;
	unsigned int ypos;

	double div;
	uint32_t  rgb;
	
	fb_pixel_t color;
	int b = 0;
	
	i = 0;
	xpos = x;
	ypos = y;
	
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
					rgb = COL_GREEN + ((unsigned char)(step*i) << 16); // adding red
				else
					rgb = COL_RED + ((unsigned char)(step*i) <<  8); // adding green
				
				color = rgb;
				
				frameBuffer->paintBoxRel(posx + i*ITEMW, posy, ITEMW, height, color);
			}
	
			//yellow
			for (; (i < yellow) && (i < maxi); i++) 
			{
				step = 255/yellow/2;

				if(inverse) 
					rgb = COL_YELLOW - (((unsigned char)step*(b++)) <<  8); // removing green
				else
					rgb = COL_YELLOW - ((unsigned char)(step*(b++)) << 16); // removing red
	
				color = rgb;		    
				
				frameBuffer->paintBoxRel(posx + i*ITEMW, posy, ITEMW, height, color);
			}

			//green
			for (; (i < green) && (i < maxi); i++) 
			{
				step = 255/green;

				if(inverse) 
					rgb = COL_YELLOW - ((unsigned char) (step*(b++)) <<  8); // removing green
				else
					rgb = COL_YELLOW - ((unsigned char) (step*(b++)) << 16); // removing red
				
				color = rgb;
				
				frameBuffer->paintBoxRel (posx + i*ITEMW, posy, ITEMW, height, color);
			}
		}
		
		for(i = maxi; i < total; i++) 
		{
			frameBuffer->paintBoxRel(posx + i*ITEMW, posy, ITEMW, height, COL_INFOBAR_PLUS_1);	//fill passive
		}
		
		percent = pcr;
	}
}

void CProgressBar::reset()
{
  	percent = 255;
}


