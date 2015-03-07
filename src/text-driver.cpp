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

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <signal.h>
#include "driver.h"
#include "error.h"
#include "carddata.h"
#include "tools.h"

using namespace Database;

namespace Driver
{
    /// Do we want free fonts even if other fonts available.
    bool use_free_fonts=true;
    /// enable/disable sounds.
    bool nosounds=false;

    static int image_count=0;

    //
    //  SDL DRIVER
    //  ==========
    //

    Driver::Driver(int screenwidth,int screenheight,bool fullscreen,int physwidth,int physheight)
    {
	nographics=false;
	
	scrw=screenwidth;
	scrh=screenheight;
	physw=screenwidth;
	physh=screenheight;
	cards=Database::cards.Cards();
    }

    Driver::~Driver()
    {
    }

    void Driver::Fullscreen(bool mode)
    {
	fullscreen=mode;
    }
	
    int Driver::AllocateSurface(int w,int h)
    {
	return 0;
    }
	
    int Driver::SelectSurface(int num)
    {
	return 0;
    }

    void Driver::FreeSurface(int num)
    {
    }

    void Driver::DrawSurface(int x,int y,int num)
    {
    }

    int Driver::SurfaceWidth(int num) const
    {
	return 1;
    }
	
    int Driver::SurfaceHeight(int num) const
    {
	return 1;
    }
	
    void Driver::Beep()
    {
	string beep_buffer="\007";
	cout << beep_buffer << flush;
    }

	void Driver::Blink(int enabled)
	{
	}
	
    void Driver::DrawCardImage(int imagenumber,int x,int y,int size,int angle,int alpha)
    {
    }

    void Driver::UpdateScreen(int x0,int y0,int w,int h)
    {
    }

    void Driver::UpdateScreen()
    {
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

    void Driver::ClearArea(int x0,int y0,int w,int h)
    {
    }

    void Driver::SetClipping(int surf,int x0,int y0,int w,int h)
    {
    }
	
    void Driver::ClippingOff(int surf)
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

    void Driver::HideMouse()
    {
    }

    void Driver::ShowMouse()
    {
    }

    static pthread_mutex_t input_buffer_lock;
    static list<string> input_buffer;
    static bool quit_signal_flag=false;

    static void handle_quit_signal(int)
    {
	quit_signal_flag=true;
    }

    static void *input_thread(void* id)
    {
	string s;

	while(1)
	{
	    if(!cin || !cout || quit_signal_flag)
		return(NULL);
			
	    cout << ">" << flush;
	    s=readline(cin) + "\n";
	    pthread_mutex_lock(&input_buffer_lock);
	    input_buffer.push_back(s);
	    pthread_mutex_unlock(&input_buffer_lock);
	}
		
	return(NULL);
    }

    Command Driver::WaitCommand(int delay)
    {
	static pthread_t input;
	static bool input_thread_created=false;
	static string buffer;
	static bool evalmode=false;
	static int sleepcount=0;
		
	Command ret;

	if(sleepcount)
	{
	    sleepcount--;
	    return ret;
	}

	if(!quit_signal_flag)
	{
	    // Read tty
	    if(isatty(0))
	    {
		if(!input_thread_created)
		{
		    pthread_mutex_init(&input_buffer_lock,NULL);
		    if(pthread_create(&input,NULL,input_thread,0))
			throw Error::IO("Driver::WaitCommand","Cannot create input thread.");
		    input_thread_created=true;
		    signal(SIGPIPE,handle_quit_signal);
		    signal(SIGQUIT,handle_quit_signal);
		    signal(SIGHUP,handle_quit_signal);
		    signal(SIGINT,handle_quit_signal);
		    signal(SIGABRT,handle_quit_signal);
		    signal(SIGTERM,handle_quit_signal);
		}

		if(buffer=="")
		{
		    pthread_mutex_lock(&input_buffer_lock);
		    if(input_buffer.size()==0)
		    {
			pthread_mutex_unlock(&input_buffer_lock);
			return ret;
		    }
		    buffer=input_buffer.front();
		    input_buffer.pop_front();
		    pthread_mutex_unlock(&input_buffer_lock);
		}
	    }
	    // Read stdin
	    else
	    {
		if(buffer=="")
		{
		    while(buffer=="")
		    {
			buffer=readline(cin)+"\n";
			if(cin.eof())
			    buffer="/quit\n";
		    }
		    sleepcount=100;
		}
	    }
	}

	// Check if it is time to quit.
	if(buffer=="/quit\n" || cin.eof() || quit_signal_flag)
	{
	    quit_signal_flag=false;
	    buffer="/quit\n";
	    pthread_cancel(input);
	    pthread_join(input,0);
	}

	if(buffer=="@\n")
	{
	    evalmode=!evalmode;
	    if(evalmode)
		cout << "Eval mode ON" << endl;
	    else
		cout << "Eval mode OFF" << endl;
	    buffer="";
	    return ret;
	}
		
	if(buffer != "")
	{
	    ret.command="key";
	    if(buffer.substr(0,1)=="\n")
	    {
		if(evalmode)
		    ret.command="control key";
		ret.argument="return";
		ret.key=0;
	    }
	    else
	    {
		ret.argument=buffer.substr(0,1);
		ret.key=buffer[0];
	    }
	    buffer=buffer.substr(1,buffer.length());
	}

	return ret;
    }

	
    int Driver::LoadImage(const string& filename,Color c)
    {
	image_count++;
	return image_count-1;
    }

    void Driver::ScaleImage(int img,int neww,int newh)
    {
    }
	
    void Driver::DrawImage(int image_number,int x,int y,int scl,int alpha,Color colorkey)
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

    void Driver::GetRGB(int image_number,int x,int y,int &r,int &g,int &b)
    {
	r=0;
	g=0;
	b=0;
    }

    int Driver::ValidImage(int number)
    {
	if(number < 0 || number >= image_count)
	    return 0;

	return 1;
    }

    // TEXT RENDERING
    // ==============

    int Driver::LoadFont(const string& primary,const string& secundary)
    {
	static int current_font=0;
	current_font++;
	return current_font-1;
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
	
    void Driver::RenderTextLine(const string& text,TextStyle style,int& x0,int& y0,int width,bool move_upward,bool text_fit,bool cut_text)
    {
    }

    int Driver::LoadSound(const string& filename)
    {
        return -1;
    }

    void Driver::PlaySound(int sound_number)
    {
    }	

    int Driver::LoadCardSound(int imagenumber)
    {
        return -1;
    }

} // namespace Driver

// MAIN PROGRAM

int gccg_main(int argc,const char** argv);

int main(int argc,const char** argv)
{
    return gccg_main(argc,argv);
}
