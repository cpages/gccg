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

#if !defined(WIN32) && !defined(__WIN32__)
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <sys/stat.h>

#if defined(_MSC_VER)
# include "compat.h"
# include <direct.h>
#endif

// #include <dirent.h>
#include <SDL.h>
#include <SDL_main.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_loadso.h>
#include <SDL_mixer.h>
#include "SDL_rotozoom.h"

#include <string>
#include <vector>
#include <map>
#include "driver.h"
#include "error.h"
#include "carddata.h"
#include "tools.h"
#include "version.h"
#include "localization.h"

using namespace Database;

#define AlphaToSDL(c) (int)((c.a)*(SDL_ALPHA_OPAQUE-SDL_ALPHA_TRANSPARENT)/100)+SDL_ALPHA_TRANSPARENT

namespace Driver
{
/// Do we want free fonts even if other fonts available.
    bool use_free_fonts=true;
/// Pointer to the physical screen.
	static SDL_Surface* screen;
/// Pointer to current output surface.
	static SDL_Surface* output=0;
/// Number of the current surface.
	static int output_surface=0;
/// Current set of allocated surfaces. NULL denotes free postion. Surface 0 is the screen.
	static vector<SDL_Surface*> surface;
/// Structure of loaded cardimages: cardimage[card number][size][angle] is 0 if not loaded.
	static map<int,map<int,map<int,SDL_Surface*> > > cardimage;
/// Vector for image store in display format.
	static vector<SDL_Surface*> image_collection;
/// Vector for image store in original format.
	static vector<SDL_Surface*> image_collection_original;
/// Pointer to the main window icon
	SDL_Surface* main_icon=0;

/// Structure of loaded card sounds: cardsound[card number] is 0 if not loaded.
	static map<int,int> cardsound;
/// Vector for sounds.
	static vector<Mix_Chunk*> sound_collection;
/// enable/disable sounds.
    bool nosounds=false;

/// Font files.
	static vector<string> fontfile;
/// Fonts.
	static vector<map<int,TTF_Font*> > font;
/// Left mouse button down.
	static bool mouse1;
/// Right mouse button down.
	static bool mouse2;
/// Middle mouse button down.
	static bool mouse3;
/// Do we have a control pressed?
	static bool control;
/// Do we have a shift pressed?
	static bool shift;
/// Do we have a alt pressed?
	static bool alt;

/// Create own card using xml-description.
	static SDL_Surface* CreateOwnCard(int imagenumber);

/// lock dynamic libraries
#ifdef WIN32
	static void *jpeglib, *pnglib;
#endif
	
// STATIC SUPPORT FUNCTIONS
// ========================

	static void SetPixel(SDL_Surface* surface,int x,int y,Uint32 pixel)
	{
		Uint8   *bits;

		bits = ((Uint8 *)surface->pixels)+y*surface->pitch+x*(surface->format->BytesPerPixel);

		switch(surface->format->BytesPerPixel)
		{
		  case 2:
			  *((Uint16 *)(bits)) = (Uint16)pixel;
			  break;

		  case 3:
			  Uint8 r, g, b;

			  r = (pixel>>surface->format->Rshift)&0xFF;
			  g = (pixel>>surface->format->Gshift)&0xFF;
			  b = (pixel>>surface->format->Bshift)&0xFF;
			  *((bits)+surface->format->Rshift/8) = r; 
			  *((bits)+surface->format->Gshift/8) = g;
			  *((bits)+surface->format->Bshift/8) = b;
			  break;

		  case 4:
			  *((Uint32 *)(bits)) = pixel;
			  break;

		  default:
			  throw Error::IO("SetPixel(SDL_Surface*,int,int,Uint32)","Unsupported color depth "+ToString(surface->format->BytesPerPixel));
		}
	}

	static Uint32 GetPixel(SDL_Surface* surface,int x,int y)
	{
		Uint8   *bits;

		SDL_PixelFormat *fmt=surface->format;
		
        bits = ((Uint8 *)surface->pixels)+y*surface->pitch+x*(fmt->BytesPerPixel);

        switch(surface->format->BytesPerPixel)
        {
          case 2:
			  return *((Uint16 *)(bits));

          case 3:

			  if (SDL_BYTEORDER == SDL_BIG_ENDIAN)			  
				  return ((*bits)<<16) | ((*(bits+1))<<8) | (*(bits+2));
			  else
				  return ((*bits)) | ((*(bits+1))<<8) | (*(bits+2)<<16);

          case 4:
			  return *((Uint32 *)(bits));

          default:
			  throw Error::IO("GetPixel(SDL_Surface*,int,int)","Unsupported color depth "+ToString(surface->format->BytesPerPixel));
        }
		
        return 0;
	}

	static int GetEvent(SDL_Event& event)
	{
		if(SDL_PollEvent(&event)==0)
			return 0;

		if(event.type==SDL_KEYUP)
		{
                    if(event.key.keysym.sym == SDLK_RCTRL || event.key.keysym.sym == SDLK_LCTRL)
                        control=false;
                    else if(event.key.keysym.sym == SDLK_RSHIFT || event.key.keysym.sym == SDLK_LSHIFT)
                        shift=false;
                    else if(event.key.keysym.sym == SDLK_RALT || event.key.keysym.sym == SDLK_LALT)
                        alt=false;
		}
		else if(event.type==SDL_KEYDOWN)
		{
                    if(event.key.keysym.sym == SDLK_RCTRL || event.key.keysym.sym == SDLK_LCTRL)
                        control=true;
                    else if(event.key.keysym.sym == SDLK_RSHIFT || event.key.keysym.sym == SDLK_LSHIFT)
                        shift=true;
                    else if(event.key.keysym.sym == SDLK_RALT || event.key.keysym.sym == SDLK_LALT)
                        alt=true;
		}
		else if (event.type==SDL_MOUSEBUTTONDOWN)
		{
                    if(event.button.button==SDL_BUTTON_LEFT)
                        mouse1=true;
                    else if(event.button.button==SDL_BUTTON_RIGHT)
                        mouse2=true;
                    else if(event.button.button==SDL_BUTTON_MIDDLE)
                        mouse3=true;
		}
		else if (event.type==SDL_MOUSEBUTTONUP)
		{
                    if(event.button.button==SDL_BUTTON_LEFT)
                        mouse1=false;
                    else if(event.button.button==SDL_BUTTON_RIGHT)
                        mouse2=false;
                    else if(event.button.button==SDL_BUTTON_MIDDLE)
                        mouse3=false;
		}
		else if(event.type==SDL_MOUSEMOTION)
		{
                    SDL_Event peep;
                    int count=0;
                    while(count < 50 && SDL_PeepEvents(&peep,1,SDL_GETEVENT,SDL_EVENTMASK(SDL_MOUSEMOTION))==1)
                    {
                        event.motion.x=peep.motion.x;
                        event.motion.y=peep.motion.y;
                        event.motion.xrel+=peep.motion.xrel;
                        event.motion.yrel+=peep.motion.yrel;
                        count++;
                    }
		}
		
		return 1;
	}

    static string CommandModifier()
    {
	if(shift && control && alt)
	    return "shift control alt ";
	else if(shift && control)
	    return "shift control ";
	else if(control && alt)
	    return "control alt ";
	else if(shift && alt)
	    return "shift alt ";
	else if(shift)
	    return "shift ";
	else if(alt)
	    return "alt ";
	else if(control)
	    return "control ";
	else
	    return "";
    }

	static unsigned char fix_SDL_Keypad(SDLKey key)
	{
		// Fixed keypad for 0..9, / and .
		if( key >= SDLK_KP0 && key <= SDLK_KP9 ) return '0' + key - SDLK_KP0;
		if( key == SDLK_KP_PERIOD ) return '.';
		if( key == SDLK_KP_DIVIDE ) return '/';
	    return 0;
	}

//
//  SDL DRIVER CORE
//  ===============
//

	Driver::Driver(int screenwidth,int screenheight,bool _fullscreen,int physwidth,int physheight)
	{
		nographics=false;
		fullscreen=_fullscreen;
		
		scrw=screenwidth;
		scrh=screenheight;
		physw=physwidth ? physwidth : scrw;
		physh=physheight ? physheight : scrh;
		needscale=(physw != scrw || physh != scrh);

		cards=Database::cards.Cards();

        if(nosounds)
        {
            if(SDL_InitSubSystem(SDL_INIT_VIDEO))
				throw Error::IO("SDL_InitSubSystem failed",SDL_GetError());
        }
        else
        {
            if(SDL_InitSubSystem(SDL_INIT_VIDEO|SDL_INIT_AUDIO))
				throw Error::IO("SDL_InitSubSystem failed",SDL_GetError());
            
            if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,4096)<0)
			{
				nosounds=true;
				cerr << "Mix_OpenAudio failed: " << SDL_GetError();
			}
        }

		Uint32 flags=(fullscreen ? SDL_FULLSCREEN : 0);
//		flags|=SDL_HWSURFACE;
		
		Uint32 bpp;

		bpp=SDL_VideoModeOK(physw,physh,32,flags);
                if(bpp < 32)
                {               
                    bpp=SDL_VideoModeOK(physw,physh,16,flags);

                    if(bpp < 16)
			throw Error::IO("Driver::Driver(bool)","No suitable video mode found, needs 16bpp.");
                }

		string caption="Gccg v"VERSION" ";
		caption+=Database::game.Gamedir();
		SDL_WM_SetCaption(caption.c_str(),"Gccg");

#ifdef WIN32
		main_icon=IMG_Load(CCG_DATADIR"/graphics/icon32.jpg");
#else
		main_icon=IMG_Load(CCG_DATADIR"/graphics/icon32.jpg");
#endif
		SDL_WM_SetIcon(main_icon,NULL);

		screen = SDL_SetVideoMode(physw, physh, bpp, flags);
		output = screen;
		surface.resize(100);
		surface[0]=screen;
		output_surface=0;
		
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);

		if(!screen)
			throw Error::IO("Driver::Driver(bool)",SDL_GetError());

		TTF_Init();

		SDL_EnableUNICODE(1);
	
		control=false;
		shift=false;
		alt=false;
		mouse1=false;
		mouse2=false;
		mouse3=false;

		char drivername[128];
		SDL_VideoDriverName(drivername, sizeof(drivername));
		const SDL_VideoInfo* info = SDL_GetVideoInfo();

		cout << Localization::Message("Graphics initialized:") << endl;
		cout << Localization::Message("  Driver = %s", drivername) << endl;
		cout << Localization::Message("  Fullscreen = %s", screen->flags&SDL_FULLSCREEN ? "Yes" : "No") << endl;
		cout << Localization::Message("  Physical resolution = %s", ToString(info->current_w)+"x"+ToString(info->current_h)) << endl;
		cout << Localization::Message("  HW Available = %s", info->hw_available ? "Yes" : "No") << endl;
		cout << Localization::Message("  HW Video memory = %sMB", ToString(info->video_mem/1024)) << endl;
		cout << Localization::Message("  HW Surface = %s", screen->flags&SDL_HWSURFACE ? "Yes" : "No") << endl;
		cout << Localization::Message("  BPP = %s", ToString((int)screen->format->BitsPerPixel)) << endl;

#ifdef WIN32
		// PERF_IMPROVEMENT: Avoid SDL_Image to load/unload shared libs on every image
		// This also seems to fix a double load of the same file.
		void* jpeglib = SDL_LoadObject("jpeg.dll");	
		void* pnglib = SDL_LoadObject("libpng12-0.dll");
#endif
	}

	Driver::~Driver()
	{
#ifdef WIN32
		SDL_UnloadObject(pnglib);
		SDL_UnloadObject(jpeglib);
#endif

		SDL_FreeSurface(main_icon);

		map<int,map<int,map<int,SDL_Surface*> > >::iterator i;
		map<int,map<int,SDL_Surface*> >::iterator j;
		map<int,SDL_Surface*>::iterator k;
		
		for(i=cardimage.begin(); i!=cardimage.end(); i++)
			for(j=(*i).second.begin(); j!=(*i).second.end(); j++)
				for(k=(*j).second.begin(); k!=(*j).second.end(); k++)
					SDL_FreeSurface((*k).second);

		for(size_t n=0; n<surface.size(); n++)
			if(surface[n])
				SDL_FreeSurface(surface[n]);
		
		TTF_Quit();

		// TODO: free all loaded sounds
		// Mix_FreeChunk(chunk);
 
		Mix_CloseAudio();
	}

	int Driver::AllocateSurface(int w,int h)
	{
		if(w < 0 || h < 0)
			throw Error::Invalid("Driver::AllocateSurface","invalid size "+ToString(w)+"x"+ToString(h));
		
		int i=1;
		while((size_t)i<surface.size() && surface[i])
			i++;
		if((size_t)i==surface.size())
			surface.resize(surface.size()+100);

		SDL_Surface* s;

		s=SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA|SDL_RLEACCEL,w,h,screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,screen->format->Amask);

		if(s==0)
			throw Error::Memory("Driver::AllocateSurface(int,int)");
		
		surface[i]=SDL_DisplayFormatAlpha(s);
		SDL_FreeSurface(s);
		
		if(surface[i]==0)
			throw Error::Memory("Driver::AllocateSurface(int,int)");
		
		return i;
	}

	int Driver::SelectSurface(int num)
	{
		if(num==-1)
			throw Error::Invalid("Driver::SelectSurface","Surface not yet allocated");
		
		if(num < 0 || (size_t)num >= surface.size())
			throw Error::Invalid("Driver::SelectSurface","Invalid surface number "+ToString(num));

		if(surface[num]==0)
			throw Error::Invalid("Driver::SelectSurface","No such surface "+ToString(num));

		output=surface[num];

		int ret=output_surface;
		output_surface=num;

		return ret;
	}

	void Driver::FreeSurface(int num)
	{
	    if(num < 1 || (size_t)num >= surface.size())
		throw Error::Invalid("Driver::FreeSurface","Invalid surface number "+ToString(num));

	    if(surface[num]==0)
		throw Error::Invalid("Driver::FreeSurface","No such surface "+ToString(num));

	    SDL_FreeSurface(surface[num]);
	    surface[num]=0;
	}

    void Driver::DrawSurface(int x,int y,int num)
    {
	if(num < 0 || (size_t)num >= surface.size())
	    throw Error::Invalid("Driver::DrawSurface","Invalid surface number "+ToString(num));
	if(surface[num]==0)
	    throw Error::Invalid("Driver::DrawSurface","No such surface "+ToString(num));
		
	SDL_Rect dst;
		
	dst.x=x;
	dst.y=y;
	dst.w=surface[num]->w;
	dst.h=surface[num]->h;

	SDL_BlitSurface(surface[num],0,screen,&dst);
    }

    int Driver::SurfaceWidth(int num) const
    {
	if(num < 0 || (size_t)num >= surface.size())
	    throw Error::Invalid("Driver::SurfaceWidth","Invalid surface number "+ToString(num));
	if(surface[num]==0)
	    throw Error::Invalid("Driver::SurfaceWidth","No such surface "+ToString(num));

	return surface[num]->w;
    }
	
    int Driver::SurfaceHeight(int num) const
    {
	if(num < 0 || (size_t)num >= surface.size())
	    throw Error::Invalid("Driver::SurfaceHeight","Invalid surface number "+ToString(num));
	if(surface[num]==0)
	    throw Error::Invalid("Driver::SurfaceHeight","No such surface "+ToString(num));

	return surface[num]->h;
    }
	
    void Driver::Fullscreen(bool mode)
    {
	if(mode!=fullscreen)
	{
	    fullscreen=mode;
	    SDL_WM_ToggleFullScreen(screen);
	}
    }
	
    void Driver::Beep()
    {
	string beep_buffer="\007";
	cout << beep_buffer << flush;
    }

	void Driver::Blink(int enabled)
	{
		// A very lame attempt to drag the user attention, since SDL doesn't provide real features for this.
		// Just blink the main window icon and title (and in app. toolbar)
		// Might be able to get something nicer using system dependant functions, but so far it seems ok
		// and not too much intrusive.
		if(enabled)
		{
			size_t masksize=((main_icon->w + 7) >> 3) * main_icon->h;
			Uint8 *mask = (Uint8 *)SDL_malloc(masksize);
			SDL_memset(mask, 0, masksize);
			SDL_WM_SetIcon(main_icon, mask);
			SDL_free(mask);

			SDL_WM_SetCaption("","");
		}
		else
		{
			SDL_WM_SetIcon(main_icon, NULL);

			string caption="Gccg v"VERSION" ";
			caption+=Database::game.Gamedir();
			SDL_WM_SetCaption(caption.c_str(),"Gccg");
		}
	}
	
    void Driver::UpdateScreen(int x0,int y0,int w,int h)
    {
	if(x0 < 0)
	{
	    w+=x0;
	    if(w <= 0)
		return;
	    x0=0;
	}
	if(x0 + w >= physw + 1)
	{
	    w = physw - x0;
	    if(w <= 0)
		return;
	}
	if(y0 + h >= physh + 1)
	{
	    h = physh - y0;
	    if(h <= 0)
		return;
	}
	if(y0 < 0)
	{
	    h+=y0;
	    if(h <= 0)
		return;
	    y0=0;
	}
	SDL_UpdateRect(screen,x0,y0,w,h);
    }

    void Driver::UpdateScreen()
    {
	SDL_UpdateRect(screen,0,0,physw,physh);
    }

    void Driver::DrawCardImage(int imagenumber,int x,int y,int size,int angle,int alpha)
    {
	LoadIfUnloaded(imagenumber,size,angle);

	SDL_Rect dest;

	dest.x=x;
	dest.y=y;

	// we keep the mode as SDL_ALPHA_OPAQUE always, unless a different alpha is specified
	if (alpha != 255)
	    SDL_SetAlpha(cardimage[imagenumber][size][angle], SDL_SRCALPHA, alpha);

	// blit graphic across.  <srcrect> is NULL: blit entire object
	SDL_BlitSurface(cardimage[imagenumber][size][angle], NULL, output, &dest);

	if (alpha != 255)
	    // use 0 to turn off alpha-blending
	    SDL_SetAlpha(cardimage[imagenumber][size][angle], 0, SDL_ALPHA_OPAQUE);
    }
	
    int Driver::CardWidth(int imagenumber,int size,int angle)
    {
	LoadIfUnloaded(imagenumber,size,angle);

	return cardimage[imagenumber][size][angle]->w;
    }

    int Driver::CardHeight(int imagenumber,int size,int angle)
    {
	LoadIfUnloaded(imagenumber,size,angle);

	return cardimage[imagenumber][size][angle]->h;
    }

    void Driver::WaitKeyPress()
    {
	SDL_Event event;

	while(1)
	{
	    SDL_PollEvent(&event);
	    if(event.type==SDL_KEYDOWN)
		break;
	}

	while(1)
	{
	    SDL_PollEvent(&event);
	    if(event.type==SDL_KEYUP)
		break;
	}
    }

    void Driver::ClearArea(int x0,int y0,int w,int h)
    {
	SDL_Rect dst;
	if(x0 < 0)
	{
	    dst.x=0;
	    dst.w=w+x0;
	}
	else
	{
	    dst.x=x0;
	    dst.w=w;
	}
	if(y0 < 0)
	{
	    dst.y=0;
	    dst.h=h+y0;
	}
	else
	{
	    dst.y=y0;
	    dst.h=h;
	}

	if(output_surface > 0)
	    SDL_FillRect(output,&dst,SDL_MapRGBA(output->format,0,0,0,SDL_ALPHA_TRANSPARENT));
	else
	    SDL_FillRect(output,&dst,SDL_MapRGBA(output->format,0,0,0,SDL_ALPHA_OPAQUE));
    }

    void Driver::SetClipping(int surf,int x0,int y0,int w,int h)
    {
	SDL_Rect clip;
		
	clip.x=x0;
	clip.y=y0;
	clip.w=w;
	clip.h=h;
	
	SDL_SetClipRect(surface[surf],&clip);
    }
	
    void Driver::ClippingOff(int surf)
    {

	SDL_SetClipRect(surface[surf],0);
    }
	
    void Driver::DrawFilledBox(int x0,int y0,int w,int h,Color c)
    {
	if(w==0 || h==0)
	    return;

	if(w < 1)
	    w=1;
	if(h < 1)
	    h=1;

	if(!c.invisible)
	{
	    SDL_Rect dst;

	    dst.x=x0;
	    dst.y=y0;
	    dst.w=w;
	    dst.h=h;

	    SDL_FillRect(output,&dst,SDL_MapRGBA(output->format,c.r,c.g,c.b,AlphaToSDL(c)));
	}
    }

    void Driver::DrawTriangleUp(int x0,int y0,int h,Color c)
    {
	if(!c.invisible)
	{
	    y0+=h-1;
	    for(int i=0; i<h; i++)
	    {
		DrawFilledBox(x0,y0,(h-i)*2,1,c);
		x0++;
		y0--;
	    }
	}
    }

    void Driver::DrawTriangleDown(int x0,int y0,int h,Color c)
    {
	if(!c.invisible)
	{
	    for(int i=0; i<h; i++)
	    {
		DrawFilledBox(x0,y0,(h-i)*2,1,c);
		x0++;
		y0++;
	    }
	}
    }
	
    void Driver::DrawBox(int x0,int y0,int w,int h,Color c)
    {
	if(!c.invisible)
	{
	    DrawFilledBox(x0,y0,w,1,c);
	    DrawFilledBox(x0,y0+h-1,w,1,c);
	    DrawFilledBox(x0,y0,1,h,c);
	    DrawFilledBox(x0+w-1,y0,1,h,c);
	}
    }

    void Driver::HideMouse()
    {
	SDL_ShowCursor(0);
    }

    void Driver::ShowMouse()
    {
	SDL_ShowCursor(1);
    }

    Command Driver::WaitCommand(int delay)
    {
	static int state=0; // 0 - normal, 1 - dragging, 2 - mouse clicked (waiting drag or release)
	static string dragtype; // Description of the dragging mode (ctrl,left etc.).
	static int dragx,dragy; // Mouseposition when dragging begun
	static Uint32 clicktime_l=0; // Time when mouse pressed button down.
	static Uint32 clicktime_m=0;
	static Uint32 clicktime_r=0;

        // Get the event if any.
	Command ret;
	SDL_Event event;

	SDL_Delay(delay);

        event.type=SDL_NOEVENT;

        if(!GetEvent(event) && state!=2)
            return ret;

	// Handle application input focus
	if(event.type==SDL_ACTIVEEVENT && (event.active.state & SDL_APPINPUTFOCUS))
	{
		ret.command= event.active.gain ? "input gained" : "input lost";
		return ret;
	}

        // Handle screen refresh.
	if(event.type==SDL_VIDEOEXPOSE || (event.type==SDL_ACTIVEEVENT && event.active.gain==1 && (event.active.state & SDL_APPACTIVE)))
	{
	    ret.command="redraw";
	    return ret;
	}
        
        // Handle quit event.
	if(event.type==SDL_QUIT)
	{
	    ret.command="quit";
	    return ret;
	}

	// Handle key press
	if(state==0 && event.type==SDL_KEYDOWN)
	{
	    SDL_GetMouseState(&ret.x,&ret.y);

	    ret.command=CommandModifier()+"key";
	    ret.argument=SDL_GetKeyName(event.key.keysym.sym);
	    ret.key=char(event.key.keysym.unicode & 0xff);
		if( ret.key == 0 ) ret.key = fix_SDL_Keypad(event.key.keysym.sym);

	    return ret;
	}

        // Handle key release
	if(state==0 && event.type==SDL_KEYUP)
	{
	    SDL_GetMouseState(&ret.x,&ret.y);
						
	    ret.command=CommandModifier()+"key up";
	    ret.argument=SDL_GetKeyName(event.key.keysym.sym);
	    ret.key=char(event.key.keysym.unicode & 0xff);
		if( ret.key == 0 ) ret.key = fix_SDL_Keypad(event.key.keysym.sym);

	    return ret;
	}

        // Handle mouse wheel
        if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP)
        {
            ret.x=event.button.x;
            ret.y=event.button.y;
            ret.command="wheel up";
            return ret;
        }
        else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN)
        {
            ret.x=event.button.x;
            ret.y=event.button.y;
            ret.command="wheel down";
            return ret;
        }

	// Mouse state 1: dragging
	if(state==1)
	{
	    if(event.type==SDL_MOUSEBUTTONUP)
	    {
		ret.x=event.button.x;
		ret.y=event.button.y;
		state=0;
		ret.command=dragtype+" drag end";

		return ret;
	    }
	    else if(event.type==SDL_MOUSEMOTION)
	    {
		ret.x=event.motion.x;
		ret.y=event.motion.y;
		ret.command=dragtype+" drag";
			
		return ret;
	    }

            ret.command="";
            return ret;
	}

	// Mouse state 2: button is just pressed down
        static string click_command;
	if(state==2)
	{
	    bool motion = false;
	    int press_time=SDL_GetTicks();

	    if(clicktime_l)
		press_time-=clicktime_l;
	    else if(clicktime_r)
		press_time-=clicktime_r;
	    else if(clicktime_m)
		press_time-=clicktime_m;

	    if(event.type==SDL_MOUSEMOTION)
	    {
                static int radius=0;

                if(radius==0)
                {
                    radius=physw/140;
                    radius*=physw/140;                            
                }

		int x,y;
		x=event.motion.x;
		y=event.motion.y;

		motion=(x-dragx)*(x-dragx) + (y-dragy)*(y-dragy) > radius;
	    }

	    if(event.type==SDL_MOUSEBUTTONUP)
	    {
		ret.x=event.button.x;
		ret.y=event.button.y;
		ret.command=click_command+" click";
		state=0;
		return ret;
	    }

	    if(press_time > 400 || motion)
	    {
		dragtype=click_command;
		ret.x=dragx;
		ret.y=dragy;
		ret.command=click_command+" drag begin";
		state=1;
		return ret;
	    }

            ret.command="";
            return ret;
	}

	// Mouse state 0: buttons are not down.
	if (event.type==SDL_MOUSEBUTTONDOWN)
	{
            if(mouse1)
		clicktime_l=SDL_GetTicks();
            else
                clicktime_l=0;
            if(mouse2)
		clicktime_r=SDL_GetTicks();
            else
                clicktime_r=0;
            if(mouse3)
		clicktime_m=SDL_GetTicks();
            else
                clicktime_m=0;

	    if(event.button.button==SDL_BUTTON_LEFT)
		click_command=CommandModifier()+"left";
	    else if(event.button.button==SDL_BUTTON_RIGHT)
		click_command=CommandModifier()+"right";
	    else if(event.button.button==SDL_BUTTON_MIDDLE)
		click_command=CommandModifier()+"middle";

	    state=2;
	    dragx=event.button.x;
	    dragy=event.button.y;
	}
	
	// Mouse state 0: moving
	if(state==0 && event.type==SDL_MOUSEMOTION)
	{
		ret.command=CommandModifier()+"move";
		ret.x=event.motion.x;
		ret.y=event.motion.y;
		return ret;
	}

	ret.command="";
	return ret;
    }

    // Load cached image surface if available.
    static SDL_Surface* CacheLoad(const string& original_file,int imagenumber,int size,int angle)
    {
	if(nocache)
	    return 0;

#ifdef WIN32
	string cachefile=getenv("TEMP");
	cachefile+="/gccg";
#else
	string cachefile="/tmp/gccg.";
	cachefile+=getenv("USER");
#endif
	cachefile+="/";
	cachefile+=Database::game.Gamedir();
	cachefile+="/";
	cachefile+=Database::cards.SetDirectory(Database::cards.Set(imagenumber));
	cachefile+="/";
	if(Localization::GetLanguage()!="en")
	{
	    cachefile+=Localization::GetLanguage();
	    cachefile+="/";
	}
	cachefile+=Database::cards.ImageFile(imagenumber);
	cachefile+="_";
	cachefile+=ToString(size);
	cachefile+="_";
	cachefile+=ToString(angle);
	cachefile+=".bmp";

	struct stat original,cached;
	if(stat(cachefile.c_str(),&cached))
	    return 0;
	if(stat(original_file.c_str(),&original))
	    return 0;
	if(cached.st_mtime < original.st_mtime)
	{
	    unlink(cachefile.c_str());
	    return 0;
	}
		
	return SDL_LoadBMP(cachefile.c_str());
    }

    // Create directory safely.
    static bool CreateDir(const string& dir)
    {
	struct stat dirfile;

#ifdef WIN32
	// PERF_IMPROVEMENT/TODO: use directory exists instead ? 
	if( stat(dir.c_str(), &dirfile)==0 )
	{
		if( dirfile.st_mode & S_IFDIR ) return true;
	    if(unlink(dir.c_str())!=0) return false;
	}

	_mkdir(dir.c_str());
#else
	if(lstat(dir.c_str(),&dirfile)==0)
	{
	    if(S_ISDIR(dirfile.st_mode))
		return true;
	    if(unlink(dir.c_str())!=0)
		return false;
	}
		
	mkdir(dir.c_str(),0755);
#endif

	return true;
    }

    // Try to save cached image.
    static void CacheSave(SDL_Surface* surface,int imagenumber,int size,int angle)
    {
	if(nocache)
	    return;
		
#ifdef WIN32
	string cachefile=getenv("TEMP");
	cachefile+="/gccg";
#else
	string cachefile="/tmp/gccg.";
	cachefile+=getenv("USER");
#endif
	// PERF_IMPROVEMENT/TODO: avoid attempting creation of existing directories
	if(!CreateDir(cachefile))
	    return;
	cachefile+="/";
	cachefile+=Database::game.Gamedir();
	if(!CreateDir(cachefile))
	    return;
	cachefile+="/";
	cachefile+=Database::cards.SetDirectory(Database::cards.Set(imagenumber));
	if(!CreateDir(cachefile))
	    return;
	cachefile+="/";
	if(Localization::GetLanguage()!="en")
	{
	    cachefile+=Localization::GetLanguage();
	    if(!CreateDir(cachefile))
		return;
	    cachefile+="/";
	}
	cachefile+=Database::cards.ImageFile(imagenumber);
	cachefile+="_";
	cachefile+=ToString(size);
	cachefile+="_";
	cachefile+=ToString(angle);
	cachefile+=".bmp";

#ifndef WIN32
	struct stat file;
	if(lstat(cachefile.c_str(),&file)==0)
	{
	    if(unlink(cachefile.c_str())!=0)
		return;
	}
#endif
			
	if(SDL_SaveBMP(surface,cachefile.c_str())!=0)
	    unlink(cachefile.c_str());
    }

    int Driver::LoadCardSound(int imagenumber)
    {
		if(nosounds) return 0;

		// Check arguments and if card is loaded already.		
		if(imagenumber < 0 || imagenumber >= Database::cards.Cards())
			return 0;

		if(cardsound[imagenumber] != 0)
			return cardsound[imagenumber];

		int snd=-1;
		int pos;

		string file=CCG_DATADIR;
		file+="/sounds/";
		file+=Database::game.Gamedir();
		file+="/";
		file+=Database::cards.SetDirectory(Database::cards.Set(imagenumber));
		file+="/";
		file+=Database::cards.ImageFile(imagenumber);
		file=Localization::File(file);
		if((pos=file.rfind(".")) >= 0)
		{
			file.replace(pos, file.length()-pos, ".ogg");
			snd=LoadSound(file.c_str());
		}

		if(snd < 0 )
		{
			string file=CCG_DATADIR;
            file+="/../";
            file+=ToLower(Database::game.Gamedir());
			file+="/sounds/";
			file+=Database::game.Gamedir();
			file+="/";
			file+=Database::cards.SetDirectory(Database::cards.Set(imagenumber));
			file+="/";
			file+=Database::cards.ImageFile(imagenumber);
			file=Localization::File(file);
			if((pos=file.rfind(".")) >= 0)
			{
				file.replace(pos, file.length()-pos, ".ogg");
				snd=LoadSound(file.c_str());
			}
		}

		cardsound[imagenumber]=snd;
		return snd;
	}

    void Driver::LoadIfUnloaded(int imagenumber,int size,int angle)
    {
	bool self_generated=false;
	static list<int> image_load_order;

	// Check arguments and if card is loaded already.
		
	if(imagenumber < 0 || imagenumber >= Database::cards.Cards())
	    throw Error::Range("Driver::LoadIfUnloaded()","Invalid imagenumber "+ToString(imagenumber));
	if(size < 1)
	    throw Error::Range("Driver::LoadIfUnloaded()","Invalid size");
	if(angle < 0 || angle >= 360)
	    throw Error::Range("Driver::LoadIfUnloaded()","Invalid angle");

	if(cardimage[imagenumber][size][angle])
	    return;

	// Main loop of image loader.
		
    try_again:
		
	SDL_Surface* newcard=0;

	string file=CCG_DATADIR;
	file+="/graphics/";
	file+=Database::game.Gamedir();
	file+="/";
	file+=Database::cards.SetDirectory(Database::cards.Set(imagenumber));
	file+="/";
	file+=Database::cards.ImageFile(imagenumber);
	file=Localization::File(file);

	// Load from cache or from original image file or generate it.
	if(!nographics || imagenumber==0)
	{
	    newcard=CacheLoad(file,imagenumber,size,angle);
	    if(newcard)
		goto scale_and_return;
	    newcard=IMG_Load(file.c_str());
	}

	if(!nographics && !newcard)
        {
            string file=CCG_DATADIR;
            file+="/../";
            file+=ToLower(Database::game.Gamedir());
            file+="/graphics/";
            file+=Database::game.Gamedir();
            file+="/";
            file+=Database::cards.SetDirectory(Database::cards.Set(imagenumber));
            file+="/";
            file+=Database::cards.ImageFile(imagenumber);
            file=Localization::File(file);
	    newcard=IMG_Load(file.c_str());
        }

	if(!newcard && imagenumber==0)
        {
            cerr << "Warning: cannot load " << file << endl;
            file=CCG_DATADIR;
            file+="/graphics/unknown_card.png";
	    newcard=IMG_Load(file.c_str());
        }

	if(!newcard)
	{
	    if(imagenumber==0)
		throw Error::IO("Driver::LoadIfUnloaded(int,int,int)","Unable to load 0-image file '"+file+"'");

	    newcard=CreateOwnCard(imagenumber);
				
	    self_generated=true;
	}

	if(!newcard)
	    goto out_of_memory;

	// Now stretch it to the correct size and angle.
		
	SDL_Surface* tmp;
		
	if(angle==0 && size!=100)
	{
	    // Circumvent buggy SDL_rotozoom and check if there is memory.
	    tmp=SDL_CreateRGBSurface(SDL_SWSURFACE,scrw,scrh,screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,screen->format->Amask);
	    if(!tmp)
		goto out_of_memory;
	    SDL_FreeSurface(tmp);
					
	    tmp=zoomSurface(newcard,double(size)/100.0,double(size)/100.0,SMOOTHING_OFF);
	    SDL_FreeSurface(newcard);
	    newcard=tmp;

	    if(!newcard)
		goto out_of_memory;
	}
	else if(angle)
	{
	    // Circumvent buggy SDL_rotozoom and check if there is memory.
	    tmp=SDL_CreateRGBSurface(SDL_SWSURFACE,scrw,scrh,screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,screen->format->Amask);
	    if(!tmp)
		goto out_of_memory;
	    SDL_FreeSurface(tmp);
			
	    int a=angle;
	    if(a)
		a=360-angle;
	    if(a%90)
		tmp=rotozoomSurface(newcard,double(a),double(size)/100.0,SMOOTHING_ON);
            else
		//multiples of 90 degrees allow straightforward translation
		//so no smoothing necessary
		tmp=rotozoomSurface(newcard,double(a),double(size)/100.0,SMOOTHING_OFF);
	    SDL_FreeSurface(newcard);
	    newcard=tmp;
			
	    if(!newcard)
		goto out_of_memory;
	}
	// otherwise size and angle are both unchanged, so do nothing
		
	// Save it to the cache.
			
	if(!self_generated)
	    CacheSave(newcard,imagenumber,size,angle);
		
	goto scale_and_return;

	// What to do when out of memory.

    out_of_memory:

	if(newcard)
	    SDL_FreeSurface(newcard);

	if(image_load_order.size()==0)
	    throw Error::Memory("Driver::LoadIfUnloaded(int,int,int)");
			
	for(int k=20; k; k--)
	{
	    if(image_load_order.size())
	    {
		int img=image_load_order.front();
		image_load_order.pop_front();

		if(img > 0)
		{
		    map<int,map<int,SDL_Surface*> >::iterator i;
		    for(i=cardimage[img].begin(); i!=cardimage[img].end(); i++)
		    {
			int sz=i->first;
			map<int,SDL_Surface*>::iterator j;
			for(j=(*i).second.begin(); j!=(*i).second.end();j++)
			{
			    int an=j->first;
			    SDL_FreeSurface(cardimage[img][sz][an]);
			    cardimage[img][sz][an]=0;
			}
		    }
		}
	    }
	}

	goto try_again;

    scale_and_return:
		
	// Scale if needed.
	if(needscale)
	{
	    // Circumvent buggy SDL_rotozoom and check if there is memory.
	    tmp=SDL_CreateRGBSurface(SDL_SWSURFACE,scrw,scrh,screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,screen->format->Amask);
	    if(!tmp)
		goto out_of_memory;
	    SDL_FreeSurface(tmp);
			
	    tmp=zoomSurface(newcard,double(physw)/double(scrw),double(physh)/double(scrh),SMOOTHING_ON);
	    SDL_FreeSurface(newcard);
	    newcard=tmp;
	}

	if(!newcard)
	    goto out_of_memory;
		
	cardimage[imagenumber][size][angle]=SDL_DisplayFormat(newcard);
	SDL_FreeSurface(newcard);

	newcard=0;
		
	if(!cardimage[imagenumber][size][angle])
	    goto out_of_memory;

	image_load_order.push_back(imagenumber);
    }


    int Driver::LoadImage(const string& filename,Color c)
    {
	SDL_Surface* img=IMG_Load(filename.c_str());

	SDL_Surface* imgscaled;
	if(!img)
	    throw Error::IO("Driver::LoadImage(const string&,int,int,int)",string("Unable to load file '"+filename+"': ")+SDL_GetError());
	if(!c.invisible)
	    SDL_SetColorKey(img,SDL_SRCCOLORKEY,SDL_MapRGB(img->format,c.r,c.g,c.b));

	imgscaled=img;
	if(needscale)
	{
	    imgscaled=zoomSurface(img,double(physw)/double(scrw),double(physh)/double(scrh),SMOOTHING_ON);
	    if(!imgscaled)
		throw Error::Memory("Driver::LoadImage(const string&,Color)");
	    if(!c.invisible)
		SDL_SetColorKey(imgscaled,SDL_SRCCOLORKEY,SDL_MapRGB(imgscaled->format,c.r,c.g,c.b));
	    SDL_FreeSurface(img);
	}

	image_collection_original.push_back(imgscaled);
	img=SDL_DisplayFormat(imgscaled);
	if(!img)
	    throw Error::Memory("Driver::LoadImage(const string&,Color)");
	image_collection.push_back(img);
		
	return image_collection.size()-1;
    }

    void Driver::ScaleImage(int img,int neww,int newh)
    {
	if(img < 0 || img >= int(image_collection.size()))
	    throw Error::Range("Driver::ScaleImage(int,int,int)","Invalid image number");

	if(needscale)
	{
	    neww=int(neww*(double(physw)/double(scrw)));
	    newh=int(newh*(double(physh)/double(scrh)));
	}
		
	if(ImageWidth(img)==neww && ImageHeight(img)==newh)
	    return;
		
	SDL_Surface* oldimg=image_collection[img];
		
	image_collection[img]=zoomSurface(oldimg,double(neww)/double(ImageWidth(img)),double(newh)/double(ImageHeight(img)),SMOOTHING_ON);
		
	SDL_FreeSurface(oldimg);
    }
	
    void Driver::DrawImage(int image_number,int x,int y,int scl,int alpha,Color colorkey)
    {
	if(image_number < 0 || image_number >= int(image_collection.size()))
	    throw Error::Range("Driver::DrawImage(int,int,int)","Invalid image number");

	SDL_Rect dest;

	dest.x=x;
	dest.y=y;

	SDL_Surface* img=image_collection[image_number];

	if(scl!=100)
	{
	    if(scl < 1)
		throw Error::Range("Driver::DrawImage(int,int,int)","Invalid scale");

	    img=zoomSurface(img,double(scl)/double(100),double(scl)/double(100),SMOOTHING_ON);
            if(!colorkey.invisible)
                SDL_SetColorKey(img,SDL_SRCCOLORKEY,SDL_MapRGB(img->format,colorkey.r,colorkey.g,colorkey.b));
	}

	if(alpha!=255)
	    SDL_SetAlpha(img,SDL_SRCALPHA,alpha);
	SDL_BlitSurface(img,0,output,&dest);
	if(alpha!=255)
	    SDL_SetAlpha(img,SDL_SRCALPHA,SDL_ALPHA_OPAQUE);

	if(scl!=100)
	    SDL_FreeSurface(img);
    }

    int Driver::ImageWidth(int image_number)
    {
	if(image_number < 0 || image_number >= int(image_collection.size()))
	    throw Error::Range("Driver::ImageWidth(int)","Invalid image number");

	return image_collection[image_number]->w;
    }

    int Driver::ImageHeight(int image_number)
    {
	if(image_number < 0 || image_number >= int(image_collection.size()))
	    throw Error::Range("Driver::ImageHeight(int)","Invalid image number");

	return image_collection[image_number]->h;
    }

    void Driver::GetRGB(int image_number,int x,int y,int &r,int &g,int &b)
    {
	SDL_Surface* surface=image_collection_original[image_number];
	SDL_PixelFormat *fmt=surface->format;
		
	if(SDL_MUSTLOCK(surface))
	    SDL_LockSurface(surface);
		
	Uint32 pixel=GetPixel(surface,x,y);

	if(SDL_MUSTLOCK(surface))
	    SDL_UnlockSurface(surface);

	Uint8 R,G,B;
	SDL_GetRGB(pixel,fmt,&R,&G,&B);

	r=R;
	g=G;
	b=B;
    }

    int Driver::ValidImage(int number)
    {
	if(number < 0 || number >= int(image_collection.size()))
	    return 0;
	
	return 1;
    }

    void Driver::CheckFont(int fontnumber,int pointsize) const
    {
	if(fontnumber < 0 || size_t(fontnumber) >= font.size())
	    throw Error::Range("CheckFont(int,int)","invalid font number");
	if(pointsize < 1)
	    throw Error::Range("CheckFont(int,int)","invalid point size");

	if(!font[fontnumber][pointsize])
	{
	    int realsize=pointsize;
			
	    if(needscale)
	    {
		realsize=(pointsize*physw)/scrw;
		if(realsize < 3)
		    realsize=3;
	    }
			
	    font[fontnumber][pointsize]=TTF_OpenFont(fontfile[fontnumber].c_str(),realsize);
	
	    if(!font[fontnumber][pointsize])
		throw Error::IO("CheckFont(int,int)","Unable to load font '"+fontfile[fontnumber]+"'");
	}
    }

    // TEXT RENDERING
    // ==============

    /// Render text to surface.
    void Driver::DrawTextToSurface(int fontnumber,int pointsize,int x0,int y0,const string& _text,Color color,Color bgcolor,bool addshadow)
    {
	string text=_text;

	if(text=="")
	    return;

	if(pointsize < 3)
	    pointsize=3;

	CheckFont(fontnumber,pointsize);

	SDL_Surface* surface;
	SDL_Rect dest;
	SDL_Color fg;
//	SDL_Color bg;

	dest.x=x0;
	dest.y=y0;

	fg.r=color.r;
	fg.g=color.g;
	fg.b=color.b;
//	bg.r=bgcolor.r;
//	bg.g=bgcolor.g;
//	bg.b=bgcolor.b;

	surface=TTF_RenderText_Blended(font[fontnumber][pointsize],text.c_str(),fg);

	if(!surface)
	{
	    cerr << "Warning: Unable to render '" << text << "'" << endl;
	    return;
	}

	//  Alpha blitting does not work correctly from RGBA -> RGBA.
	//	We must use RGB shadow surface if we want to see text in invisible background.
	//
	if(addshadow || bgcolor.invisible)
	{
	    SDL_Surface* shadow;
	    SDL_Color black;
	    black.r=1;
	    black.g=1;
	    black.b=1;

	    //			shadow=TTF_RenderText_Blended(font[fontnumber][pointsize],text.c_str(),black);
	    shadow=TTF_RenderText_Shaded(font[fontnumber][pointsize],text.c_str(),black,black);
	    SDL_SetColorKey(shadow,SDL_SRCCOLORKEY,SDL_MapRGB(shadow->format,0,0,0));

	    if(!shadow)
	    {
		cerr << "Warning: Unable to render '" << text << "'" << endl;
		return;
	    }

	    dest.x=x0-1;
	    dest.y=y0;
	    SDL_BlitSurface(shadow,0,output,&dest);
	    dest.x=x0+1;
	    dest.y=y0;
	    SDL_BlitSurface(shadow,0,output,&dest);
	    dest.x=x0;
	    dest.y=y0-1;
	    SDL_BlitSurface(shadow,0,output,&dest);
	    dest.x=x0;
	    dest.y=y0+1;
	    SDL_BlitSurface(shadow,0,output,&dest);
	    dest.x=x0;
	    dest.y=y0;
	    SDL_FreeSurface(shadow);
	}

	SDL_BlitSurface(surface,0,output,&dest);

	SDL_FreeSurface(surface);
    }

    int Driver::LoadFont(const string& primary,const string& secondary)
    {
	string f;
	
	f=CCG_DATADIR"/graphics/fonts/";
	f+=primary;
	if(!FileExist(f) && secondary!="")
	{
	    f=CCG_DATADIR"/graphics/fonts/";
	    f+=secondary;
	    if(!FileExist(f))
	    {
		f=CCG_DATADIR"/graphics/fonts/";
		f+=primary;
	    }
	}

	fontfile.push_back(f);
	font.push_back(map<int,TTF_Font*>());
	
	CheckFont(fontfile.size() - 1,12);

	return fontfile.size() - 1;
    }

    int Driver::TextLineSkip(int fontnumber,int pointsize) const
    {
	CheckFont(fontnumber,pointsize);

	return TTF_FontDescent(font[fontnumber][pointsize])+1;
    }

    int Driver::TextHeight(int fontnumber,int pointsize,const string& text) const
    {
	CheckFont(fontnumber,pointsize);

	int w,h;
	TTF_SizeText(font[fontnumber][pointsize],text.c_str(),&w,&h);
	return h;
    }

    int Driver::TextWidth(int fontnumber,int pointsize,const string& text) const
    {
	CheckFont(fontnumber,pointsize);

	int w,h;
	TTF_SizeText(font[fontnumber][pointsize],text.c_str(),&w,&h);
	return w;
    }

    void Driver::DrawText(int fontnumber,int pointsize,int x0,int y0,const string& text,Color color,Color bgcolor)
    {
	CheckFont(fontnumber,pointsize);

	if(!color.invisible)
	    DrawTextToSurface(fontnumber,pointsize,x0,y0,text,color,bgcolor);
    }

    void Driver::DrawTextShadow(int fontnumber,int pointsize,int x0,int y0,const string& text,Color color,Color bgcolor)
    {
	CheckFont(fontnumber,pointsize);

	if(!color.invisible)
	    DrawTextToSurface(fontnumber,pointsize,x0,y0,text,color,bgcolor,true);
    }

    /// Calculate width of the text.
    int Driver::GetTextWidth(string text,TextStyle style)
    {
	int w,h;
	GetNextTextLine(text,style,w,h,999999);
	return w;
    }

    /// Calculate height of the text.
    int Driver::GetTextHeight(string text,TextStyle style)
    {
	int w,h;
	GetNextTextLine(text,style,w,h,999999);
	return h;
    }

    int Driver::GetTextHeight(string text,TextStyle style,int width)
    {
	string last,src=text;

	int texth=0;
	int w,h;
	do
	{
	    last=GetNextTextLine(src,style,w,h,width);
	    if(texth)
		texth+=TextLineSkip(style.font,style.pointsize);

	    texth+=GetTextHeight(last,style);

	} while(src != "");

	return texth;
    }

    void Driver::RenderTextLine(const string& text, TextStyle style, int& x0, int& y0, int width, bool move_upward, bool fit_text, bool cut_text)
    {
        // Split from formatting tags.
        string split=text;
        string::size_type loc=split.find("{right}");
        if(loc > 0 && loc!=string::npos)
        {
            int x=x0,y=y0;
            RenderTextLine(split.substr(0,loc), style, x, y, width, move_upward, fit_text, cut_text);
            RenderTextLine(split.substr(loc), style, x0, y0, width, move_upward, fit_text, cut_text);
            return;
        }
        loc=split.find("{left}");
        if(loc > 0 && loc!=string::npos)
        {
            int x=x0,y=y0;
            RenderTextLine(split.substr(0,loc), style, x, y, width, move_upward, fit_text, cut_text);
            RenderTextLine(split.substr(loc), style, x0, y0, width, move_upward, fit_text, cut_text);
            return;
        }
        loc=split.find("{center}");
        if(loc > 0 && loc!=string::npos)
        {
            int x=x0,y=y0;
            RenderTextLine(split.substr(0,loc), style, x, y, width, move_upward, fit_text, cut_text);
            RenderTextLine(split.substr(loc), style, x0, y0, width, move_upward, fit_text, cut_text);
            return;
        }

        // Do it.
	int h = GetTextHeight(text,style);

	if(fit_text)
	{
	    int w = GetTextWidth(text,style);
	    while(w > width && style.pointsize > 7)
	    {
		style.pointsize--;
		w = GetTextWidth(text,style);
	    }
	}

        if(cut_text)
            SetClipping(output_surface,x0,y0-h,width,h*2);

	if(move_upward)
	{
	    y0 -= h;
	    y0++; // looks a bit better when tightening lines by one pixel
	}

	int offset=0;

	switch(style.align)
	{
	  case 0:
	      offset = style.margin; 
	      break; // left
	  case 1:
	      offset = width/2 - GetTextWidth(text,style)/2; 
	      break; // center
	  case 2:
	      offset = width - GetTextWidth(text,style) - style.margin;
	      break; // right
	}

	int w0=0, h0;
	string s = text;

	while(s != "")
	{
            string word = GetNextTextElement(s, style, w0, h0, width);

	    if(word[0]=='{')
	    {
		if(word=="{|}")
		{
		    int curh=TextHeight(style.font,style.pointsize,"|")-2;
		    int x=x0+offset+1;
		    int y=y0+2;

		    DrawBox(x-1,y,3,curh,BLACK);
		    DrawBox(x,y+1,1,curh-2,style.color);
		    DrawBox(x-1,y,3,1,style.color);
		    DrawBox(x-1,y+curh-1,3,1,style.color);
		}
		else if(word=="{hr}")
		    DrawFilledBox(x0+style.margin,y0,width-2*style.margin,1,style.color);
		else if(word=="{left}")
		    offset=offset + style.margin;
		else if(word=="{center}")
		    offset=offset + (width-offset)/2 - GetTextWidth(s,style)/2;
		else if(word=="{right}")
		    offset=width - GetTextWidth(s,style) - style.margin;
		else if(inlineimage.find(word)!=inlineimage.end())
		{
		    int ix=x0+offset,iy=y0+(h-h0)/2;

		    SDL_Rect dest;
		    dest.x=ix;
		    dest.y=iy;
		    SDL_BlitSurface(image_collection[inlineimage[word]],0,output,&dest);
		}
		else if(word.substr(0,5)=="{card" && word.substr(word.length()-1,1)=="}")
		{
		    int n=atoi(word.substr(5,word.length()-6).c_str());
		    if(n >= 0 && n < Database::cards.Cards())
		    {
			string name=Database::cards.Name(n);
			DrawTextToSurface(style.font, style.pointsize, x0+offset, y0+(h-h0), name, style.color, BLACK, style.shadow);
		    }
		}
		else if(word=="{lb}")
		    DrawTextToSurface(style.font,style.pointsize,x0+offset,y0+(h-h0),"{",style.color,BLACK,style.shadow);
		else if(word=="{rb}")
		    DrawTextToSurface(style.font,style.pointsize,x0+offset,y0+(h-h0),"}",style.color,BLACK,style.shadow);
	    }
	    else
		DrawTextToSurface(style.font, style.pointsize, x0+offset, y0+(h-h0), word, style.color, BLACK, style.shadow);

	    offset += w0;
	}

	if(!move_upward)
	{
	    y0+=h;
	    y0+=TextLineSkip(style.font,style.pointsize);
	}

        if(cut_text)
            ClippingOff(output_surface);
    }

    void Driver::RenderTextFitVertical(const string& text,TextStyle& style,int x0,int y0,int width,int height)
    {
	if(text!="")
	{
	    int h0=GetTextHeight(text,style,width);
	    int h1;
			
	    while(h0 > height && style.pointsize > 5)
	    {
		style.pointsize--;
		h1=GetTextHeight(text,style,width);
		if(h1==h0)
		    break;
		h0=h1;
	    }
			
	    RenderText(text,style,x0,y0,width,height);
	}
    }

    void Driver::RenderTextFitHorizontal(const string& text,TextStyle& style,int x0,int y0,int width,int height)
    {
	if(text!="")
	{
	    int w0=GetTextWidth(text,style);
	    int w1;
			
	    while(w0 > width && style.pointsize > 5)
	    {
		style.pointsize--;
		w1=GetTextWidth(text,style);
		if(w1==w0)
		    break;
		w0=w1;
	    }
			
	    RenderText(text,style,x0,y0,width,height);
	}
    }
	
    // CARD CREATION
    // =============
	
    static string GameOption(int image,const string& option)
    {
	static string hint_type,hint_type2;
	string ret;

	if(hint_type=="")
	    hint_type=game.Option("draw hints");
	if(hint_type2=="")
	    hint_type2=game.Option("draw hints 2");

	ret=game.Option(cards.Name(image)+" "+option);
	if(ret=="" && hint_type!="")
	    ret=game.Option(cards.AttrValue(image,hint_type)+" "+option);
	if(ret=="" && hint_type2!="")
	    ret=game.Option(cards.AttrValue(image,hint_type2)+" "+option);
	if(ret=="")
	    ret=game.Option(string("default ")+option);

	return ret;
    }

    static Color GameColor(int image,const string& option,Color def)
    {
	string col=GameOption(image,option);
	if(col=="")
	    return def;

	const char* s=col.c_str();

	int r=atoi(s);
	while(*s && *s!=',')
	    s++;
	s++;

	int g=atoi(s);
	while(*s && *s!=',')
	    s++;
	s++;

	int b=atoi(s);

	return Color(r,g,b);
    }

    static Uint32 GameSDLColor(int image,const string& option,int r_def,int g_def,int b_def)
    {
	Color col=GameColor(image,option,Color(r_def,g_def,b_def));
	
	return SDL_MapRGB(screen->format,col.r&255,col.g&255,col.b&255);
    }

    static int GameInt(int image,const string& option,int def)
    {
	string col=GameOption(image,option);
	if(col=="")
	    return def;
	
	return atoi(col.c_str());
    }

    static SDL_Rect GamePos(int image,const string& option,int x_def,int y_def,int w_def, int h_def)
    {
	string col=GameOption(image,option);

	SDL_Rect ret;

	if(col=="")
	{
	    ret.x=x_def;
	    ret.y=y_def;
	    ret.w=w_def;
	    ret.h=h_def;
	    return ret;
	}
	
	ret.w=0;
	ret.h=0;
	
	const char* s=col.c_str();
	ret.x=atoi(s);
	while(*s && *s!=',')
	    s++;
	s++;
	ret.y=atoi(s);
	if(*s)
	{
	    while(*s && *s!=',')
		s++;
	    s++;
	    ret.w=atoi(s);
	    if(*s)
	    {
		while(*s && *s!=',')
		    s++;
		s++;
		ret.h=atoi(s);
	    }
	}

	return ret;
    }

    static void GetTextStyle(int image,const string& option,TextStyle& style,int font=4,int size=12,Color color=Color(0,0,0),int align=0)
    {
	style.valign=GameInt(image,option+" valign",1);
	style.align=GameInt(image,option+" align",align);
	style.font=GameInt(image,option+" font",font);
	style.pointsize=GameInt(image,option+" font size",size);
	style.color=GameColor(image,option+" font color",color);
    }

    // Replace "[attr]" notations with the corresponding values of the attributes.
    static string GetAttrText(const string& src,int imagenumber)
    {
	bool ok=false,noattr=true;
	string attr,txt;
		
	for(size_t i=0; i<src.length(); i++)
	{
	    if(src[i]=='\\' && src[i+1]=='n')
	    {
		txt+="\n";
		i++;
	    }
	    else if(src[i]=='[')
	    {
		i++;
		attr="";
		while(i<src.length() && src[i] != ']')
		    attr+=src[i++];
				
		attr=cards.AttrValue(imagenumber,attr);
		if(attr != "")
		    ok=true;
		txt+=attr;
		noattr=false;
	    }
	    else
		txt+=src[i];
	}

	return ok || noattr ? txt : string("");
    }

    static void DrawBoxToCard(SDL_Surface* ret,int imagenumber,const char* boxname)
    {
	Color col;
	SDL_Rect box,box2;
	int n;
	
	box=GamePos(imagenumber,boxname,0,0,0,0);
	
	if(box.w)
	{
	    string req;
	    req=GameOption(imagenumber,boxname+string(" require"));
	    if(req=="" || GetAttrText(req,imagenumber)!="")
	    {
		n=GameInt(imagenumber,boxname+string(" border"),0);
		if(n)
		{
		    box2=box;
		    box2.w+=2*n;
		    box2.h+=2*n;
		    box2.x-=n;
		    box2.y-=n;
		    col=GameColor(imagenumber,boxname+string(" color"),Color(68,68,68));
		    if(col.r < 30)
			col.r=0;
		    if(col.g < 30)
			col.g=0;
		    if(col.b < 30)
			col.b=0;

		    SDL_FillRect(ret,&box2,GameSDLColor(imagenumber,boxname+string(" border color"),col.r-30,col.g-30,col.b-30));
		}
		SDL_FillRect(ret,&box,GameSDLColor(imagenumber,boxname+string(" color"),128,128,128));
	    }
	}
    }
	
    static void DrawTextToCard(SDL_Surface* ret,int imagenumber,const char* textname)
    {
	SDL_Rect box;
		
	box=GamePos(imagenumber,textname,0,0,0,0);
	if(box.w)
	{
	    string src=GameOption(imagenumber,textname+string(" content"));
	    src=GetAttrText(src,imagenumber);
	    if(src != "")
	    {
		TextStyle style;
		GetTextStyle(imagenumber,textname,style);
		driver->RenderTextFitHorizontal(src,style,box.x,box.y,box.w,box.h);
	    }
	}
    }

	static string ImageText(int imagenumber)
	{
		string ret,n=cards.Name(imagenumber);

		size_t p=n.find_first_not_of(" ,.·");
		while(p!=string::npos)
		{
			ret+=n[p];
			p=n.find_first_of(" ,.·",p+1);
			if(p!=string::npos)
				p=n.find_first_not_of(" ,.·",p+1);
		}

		return ret;
	}
	
    static SDL_Surface* CreateOwnCard(int imagenumber)
    {
	SDL_Surface* ret;
	
	driver->LoadIfUnloaded(0,100,0);
	
	int w=cardimage[0][100][0]->w * driver->ScreenWidth() / driver->PhysicalWidth();
	int h=cardimage[0][100][0]->h * driver->ScreenHeight() / driver->PhysicalHeight();

	ret=SDL_CreateRGBSurface(SDL_SWSURFACE,w,h,screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,screen->format->Amask);

	if(ret==0)
	    return 0;
		
	int n;
	SDL_Rect dst;

	// Background
		
	dst.x=0;
	dst.y=0;
	dst.w=w;
	dst.h=h;
	SDL_FillRect(ret,&dst,GameSDLColor(imagenumber,"border color",0,0,0));
	n=GameInt(imagenumber,"border width",5);
	dst.x+=n;
	dst.y+=n;
	dst.w-=2*n;
	dst.h-=2*n;
	SDL_FillRect(ret,&dst,GameSDLColor(imagenumber,"back color",255,255,255));

	// Boxes
		
	DrawBoxToCard(ret,imagenumber,"title box");
	DrawBoxToCard(ret,imagenumber,"text box");
	DrawBoxToCard(ret,imagenumber,"image");
	DrawBoxToCard(ret,imagenumber,"box 1");
	DrawBoxToCard(ret,imagenumber,"box 2");
	DrawBoxToCard(ret,imagenumber,"box 3");
	DrawBoxToCard(ret,imagenumber,"box 4");
	DrawBoxToCard(ret,imagenumber,"box 5");
	DrawBoxToCard(ret,imagenumber,"box 6");
	DrawBoxToCard(ret,imagenumber,"box 7");
	DrawBoxToCard(ret,imagenumber,"box 8");
	DrawBoxToCard(ret,imagenumber,"box 9");

	// Title
	SDL_Surface* old_output=output;
	output=ret;
		
	TextStyle titlestyle;
	GetTextStyle(imagenumber,"title",titlestyle);
	SDL_Rect titlebox=GamePos(imagenumber,"title",6,6,200,50);
		
	driver->RenderTextFitHorizontal(Database::cards.Name(imagenumber).c_str(),titlestyle,titlebox.x,titlebox.y,titlebox.w,titlebox.h);
		
	// Card text
	string text=cards.Text(imagenumber);
	if(text!="")
	{
	    SDL_Rect textbox=GamePos(imagenumber,"text box",0,0,0,0);
	    if(textbox.w)
	    {
		TextStyle style;
		GetTextStyle(imagenumber,"text box",style);
		driver->RenderTextFitVertical(GameOption(imagenumber,"text box special")+text,style,textbox.x,textbox.y,textbox.w,textbox.h);
	    }
	}

	// Other texts
	DrawTextToCard(ret,imagenumber,"text 1");
	DrawTextToCard(ret,imagenumber,"text 2");
	DrawTextToCard(ret,imagenumber,"text 3");
	DrawTextToCard(ret,imagenumber,"text 4");
	DrawTextToCard(ret,imagenumber,"text 5");
	DrawTextToCard(ret,imagenumber,"text 6");
	DrawTextToCard(ret,imagenumber,"text 7");
	DrawTextToCard(ret,imagenumber,"text 8");
	DrawTextToCard(ret,imagenumber,"text 9");

	// Image
	SDL_Rect imagebox=GamePos(imagenumber,"image",0,0,0,0);
	if(imagebox.w)
	{
	    TextStyle style;
	    GetTextStyle(imagenumber,"image",style,1,60,Color(0,0,230),1);
	    driver->RenderTextFitHorizontal(ImageText(imagenumber),style,imagebox.x,imagebox.y,imagebox.w,imagebox.h);
	}
		
	// Corners
	output=old_output;
		
	if(SDL_MUSTLOCK(ret))
	    SDL_LockSurface(ret);
		
	SetPixel(ret,0,0,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,0,1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,0,2,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,0,3,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,1,0,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,2,0,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,3,0,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,1,1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-1,0,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-1,1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-1,2,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-1,3,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-2,0,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-3,0,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-4,0,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-2,1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,0,h-1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,0,h-2,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,0,h-3,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,0,h-4,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,1,h-1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,2,h-1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,3,h-1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,1,h-2,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-1,h-1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-1,h-2,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-1,h-3,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-1,h-4,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-2,h-1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-3,h-1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-4,h-1,SDL_MapRGB(ret->format,10,11,12));
	SetPixel(ret,w-2,h-2,SDL_MapRGB(ret->format,10,11,12));
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,10,11,12));

	if(SDL_MUSTLOCK(ret))
	    SDL_UnlockSurface(ret);

	return ret;
    }

    int Driver::LoadSound(const string& filename)
    {
		if(nosounds) return 0;

		Mix_Chunk* chunk=Mix_LoadWAV(filename.c_str());
		if(!chunk)
            return -1;
		sound_collection.push_back(chunk);
		return sound_collection.size()-1;
	}

	void Driver::PlaySound(int sound_number)
	{
		if(nosounds) return;

		if(sound_number < 0 || sound_number >= int(sound_collection.size()))
			throw Error::Range("Driver::PlaySound(int)","Invalid sound number");

		Mix_Chunk* chunk=sound_collection[sound_number];
		if(Mix_PlayChannel(-1, chunk, 0) < 0)
			cerr << "Unable to play sound: " << Mix_GetError() << endl;
	}
}

// MAIN PROGRAM

int gccg_main(int argc,const char** argv);

#ifdef WIN32
extern "C" int SDL_main(int argc,char** argv)
{
    return gccg_main(argc,(const char**)argv);
}
#else
extern "C" int main(int argc,char** argv)
{
    return gccg_main(argc,(const char**)argv);
}
#endif
