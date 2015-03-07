/*
    Gccg - Generic collectible card game.
    Copyright (C) 2001,2002,2003,2004 Tommi Ronkainen

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program, in the file license.txt. If not, write
  to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
*/

#include <ncurses.h>
#include <string>
#include <vector>
#include <map>
#include "driver.h"
#include "error.h"
#include "carddata.h"
#include "tools.h"

using namespace Database;

// LOWLEVEL SUPPORT FUNCTIONS
// ==========================

enum ColorPair
{
	PairBlackBlack=1, PairRedBlack, PairGreenBlack, PairYellowBlack, PairBlueBlack, PairMagentaBlack,	PairCyanBlack, PairWhiteBlack};

void set_color(Driver::Color c)
{
	if(c==Driver::BLACK)
		color_set(PairBlackBlack,0);
	else if(c==Driver::YELLOW)
		color_set(PairYellowBlack,0);
	else if(c==Driver::RED)
		color_set(PairRedBlack,0);
	else if(c==Driver::BLUE)
		color_set(PairBlueBlack,0);
	else if(c==Driver::GREEN)
		color_set(PairGreenBlack,0);
	else if(c==Driver::MAGENTA)
		color_set(PairMagentaBlack,0);
	else
		color_set(PairWhiteBlack,0);
}

void draw_box(int x0,int y0,int w,int h,Driver::Color c)
{
	set_color(c);
	
	if(w > 1)
	{
		mvhline(y0,x0,ACS_HLINE,w);
		mvhline(y0+h-1,x0,ACS_HLINE,w);
	}
	
	if(h > 1)
	{
		mvvline(y0,x0,ACS_VLINE,h);
		mvvline(y0,x0+w-1,ACS_VLINE,h);
	}

	if(h > 1 && w > 1)
	{
		mvhline(y0,x0,ACS_ULCORNER,1);
		mvhline(y0,x0+w-1,ACS_URCORNER,1);
		mvhline(y0+h-1,x0,ACS_LLCORNER,1);
		mvhline(y0+h-1,x0+w-1,ACS_LRCORNER,1);
	}
}

void clear_box(int x0,int y0,int w,int h)
{
	for(int i=0; i<h; i++)
		mvhline(y0+i,x0,' ',w);
}

namespace Driver
{
	/// Curses window.
	WINDOW *window=0;
	/// Number of images loaded.
	static int image_count=0;

	// Local support functions used by game-draw.cpp
	
//
//  CURSES DRIVER
//  =============
//

	Driver::Driver(int screenwidth,int screenheight,bool fullscreen,int physwidth,int physheight)
	{
		nographics=false;
	
		scrw=screenwidth;
		scrh=screenheight;
		needscale=true;
		cards=Database::cards.Cards();
		
		window = initscr();
		start_color();
		cbreak();
		raw();
		noecho();
		nonl();
		curs_set(0);
		intrflush(stdscr, FALSE);
		keypad(stdscr, TRUE);
		halfdelay(1);
		nodelay(window,true);
		
		physw=COLS;
		physh=LINES;
		
		init_pair(PairBlackBlack,COLOR_BLACK,COLOR_BLACK);
		init_pair(PairRedBlack,COLOR_RED,COLOR_BLACK);
		init_pair(PairGreenBlack,COLOR_GREEN,COLOR_BLACK);
		init_pair(PairYellowBlack,COLOR_YELLOW,COLOR_BLACK);
		init_pair(PairBlueBlack,COLOR_BLUE,COLOR_BLACK);
		init_pair(PairMagentaBlack,COLOR_MAGENTA,COLOR_BLACK);
		init_pair(PairCyanBlack,COLOR_CYAN,COLOR_BLACK);
		init_pair(PairWhiteBlack,COLOR_WHITE,COLOR_BLACK);
	}

	Driver::~Driver()
	{
		endwin();
	}

	void Driver::Beep()
	{
		static char* beep_buffer="\007";
		cout << beep_buffer << flush;
	}
	
	void Driver::DrawCardImage(int imagenumber,int x,int y,int size,int angle)
	{
	}

	void Driver::UpdateScreen(int x0,int y0,int w,int h)
	{
		wrefresh(window);
	}

	void Driver::UpdateScreen()
	{
		wrefresh(window);
	}

	int Driver::CardWidth(int imagenumber,int size,int angle)
	{
		return 1;
	}

	int Driver::CardHeight(int imagenumber,int size,int angle)
	{
		return 1;
	}

	void Driver::WaitKeyPress()
	{
	}

	void Driver::SetClipping(int x0,int y0,int w,int h)
	{
	}

	void Driver::ClearArea(int x0,int y0,int w,int h)
	{
	}

	void Driver::DrawFilledBox(int x0,int y0,int w,int h,Color c)
	{
	}

	void Driver::DrawTriangleUp(int x0,int y0,int h,Color c)
	{
	}

	void Driver::DrawTriangleDown(int x0,int y0,int h,Color c)
	{
	}
	
	void Driver::DrawBox(int x0,int y0,int w,int h,Color c)
	{
	}

	void Driver::ClippingOff()
	{
	}

	void Driver::ScrollDown(int ystep,int xoff)
	{
	}

	void Driver::ScrollUp(int ystep,int xoff)
	{
	}

	void Driver::ScrollLeft(int xstep,int yoff)
	{
	}

	void Driver::ScrollRight(int xstep,int yoff)
	{
	}

	void Driver::SaveScreen()
	{
	}

	void Driver::RestoreScreen()
	{
	}

	void Driver::RestoreScreen(int x,int y,int w,int h)
	{
	}

	void Driver::SaveRectangle(int x,int y,int w,int h)
	{
	}

	void Driver::RestoreRectangle(int x,int y)
	{
	}

	void Driver::HideMouse()
	{
	}

	void Driver::ShowMouse()
	{
	}

	Command Driver::WaitCommand(int delay)
	{
		Command ret;
		int c=getch();

		switch(c)
		{
		  case -1:
			  return ret;
			  
		  case 13: // Return
			  ret.command="key";
			  ret.argument="return";
			  break;

		  case 3: // Ctrl+C
		  case 17: // Ctrl+Q
			  ret.command="quit";
			  break;

		  case 263: // Backspace
			  ret.command="key";
			  ret.argument="backspace";
			  break;
			  
		  default:
			  ret.command="key";
			  ret.argument=(char)c;
		}

		return ret;
	}

	
	int Driver::LoadImage(const string& filename,Color c)
	{
		image_count++;
		return image_count-1;
	}

	void Driver::DrawImage(int image_number,int x,int y)
	{
	}

	int Driver::ImageWidth(int image_number)
	{
		return 1;
	}

	int Driver::ImageHeight(int image_number)
	{
		return 1;
	}

	int Driver::ValidImage(int number)
	{
		if(number < 0 || number >= image_count)
			return 0;

		return 1;
	}

	// TEXT RENDERING
	// ==============

/// Render text to surface.
	int Driver::LoadFont(const string& filename)
	{
		return 0;
	}

	int Driver::TextLineSkip(int fontnumber,int pointsize) const
	{
		return 0;
	}

	int Driver::TextHeight(int fontnumber,int pointsize,const string& text) const
	{
		return 1;
	}

	int Driver::TextWidth(int fontnumber,int pointsize,const string& text) const
	{
		return text.length();
	}

	void Driver::DrawText(int fontnumber,int pointsize,int x0,int y0,const string& text,Color color,Color bgcolor)
	{
		set_color(color);
		scale(x0,y0);
		move(y0,x0);
		printw(text.c_str());
	}

	void Driver::DrawTextShadow(int fontnumber,int pointsize,int x0,int y0,const string& text,Color color,Color bgcolor)
	{
	}

	int Driver::GetTextWidth(string text,TextStyle style)
	{
		return text.length();
	}

	int Driver::GetTextHeight(string text,TextStyle style)
	{
		return 1;
	}

	int Driver::GetTextHeight(string text,TextStyle style,int width)
	{
		return 1;
	}
	
	void Driver::RenderTextLine(const string& text,TextStyle style,int& x0,int& y0,int width,bool move_upward)
	{
	}
	
} // namespace Driver

// MAIN PROGRAM

int gccg_main(int argc,const char** argv);

int main(int argc,const char** argv)
{
	return gccg_main(argc,argv);
}
