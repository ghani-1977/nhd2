/*
*	progressbar.h
*/

#ifndef __progressbar_
#define __progressbar_

#include <driver/framebuffer.h>


class CProgressBar
{
	private:
		CFrameBuffer * frameBuffer;
		short width;
		short height;
		char percent;
		short red, green, yellow;
		bool inverse;

	public:
		CProgressBar(int w, int h, int r, int g, int b, bool inv = false);
		void paint(int x, int y, int pcr);
		void reset();
		int getPercent() { return percent; };
};


#endif
