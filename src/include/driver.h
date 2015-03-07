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

#ifndef DRIVER_H
#define DRIVER_H

#include <string>

using namespace std;

/// Low level I/O-stuff.
namespace Driver
{
    class Driver;
	
    /// Do we want free fonts even if other fonts available.
    extern bool use_free_fonts;
    /// Don't load images, use generated cards.
    extern bool nographics;
    /// Don't save images to the cache.
    extern bool nocache;
    /// Don't load/play sounds.
    extern bool nosounds;
    /// Graphics and IO-driver.
    extern Driver* driver;
    /// Structure of inline images (index to image_collection).
    extern map<string,int> inlineimage;

    /// Color type.
    struct Color
    {
	bool invisible;
	int r,g,b;
        int a; // 0-100 transparent - opaque

	Color()
	{r=g=b=a=0; invisible=true;}
	Color(int _r,int _g,int _b)
	{r=_r;g=_g;b=_b;a=100;invisible=false;}
	Color(int _r,int _g,int _b,int _a)
	{r=_r;g=_g;b=_b;a=_a;invisible=(a==0);}

	bool operator==(const Color& c)
	{return (c.invisible && invisible) || (c.r==r && c.g==g && c.b==b && c.a==a);}

	void Darken()
	{
	    r-=20;
	    if(r<0) r=0;
	    g-=20;
	    if(g<0) g=0;
	    b-=20;
	    if(b<0) b=0;
	}

	void Lighten()
	{
	    r+=20;
	    if(r>255) r=255;
	    g+=20;
	    if(g>255) g=255;
	    b+=20;
	    if(b>255) b=255;
	}

        void LessTransparent()
        {
            a+=10;
            if(a>100)
                a=100;
        }

        void MoreTransparent()
        {
            a-=10;
            if(a<0)
                a=0;
        }
    };

    const Color BLACK=Color(0,0,0);
    const Color WHITE=Color(255,255,255);
    const Color RED=Color(224,17,17);
    const Color BLUE=Color(32,51,219);
    const Color GREEN=Color(4,135,30);
    const Color YELLOW=Color(244,233,9);
    const Color MAGENTA=Color(88,45,88);
    const Color INVISIBLE=Color();
	
    /// Input action.
    struct Command
    {
	/// Command.
	string command;
	/// Command arguments.
	string argument;
	/// ASCII presentation of the key.
	unsigned char key;
	/// Mouse x-coordinate when event occurred.
	int x;
	/// Mouse y-coordinate when event occurred.
	int y;
    };

    /// Structure to store text rendering information.
    struct TextStyle
    {
	/// Text color
	Color color;
	/// Text font number.
	int font;
	/// Text size in points.
	int pointsize;
	/// Aligment: 0 - left, 1 - center, 2 - right.
	int align;
	/// Vertical aligment: 0 - top, 1 - middle, 2 - bottom.
	int valign;
	/// Space between text and object border.
	int margin;
	/// Draw black shadow for text.
	bool shadow;

	TextStyle()
	{color=WHITE; font=0; pointsize=12; align=0; valign=0; margin=2;shadow=false;}
    };

    /// Driver for graphics and mouse/keyboard input.
    class Driver
    {
	/// Screen design width.
	int scrw;
	/// Screen design height.
	int scrh;
	/// Physcial screen width.
	int physw;
	/// Physcial screen height.
	int physh;
	/// Number of cards.
	int cards;
	/// True, if physical screensize differs from the design size.
	bool needscale;
	/// True, if fullscreen mode.
	bool fullscreen;

    public:
	
	/// Change to graphics mode and initialize variables.
	Driver(int screenwidth,int screenheight,bool fullscreen,int physwidth,int physheight);
	/// Leave graphics mode and clear variables.
	~Driver();

	/// Width of the screen.
	int ScreenWidth();
	/// Height of the screen.
	int ScreenHeight();
	/// Width of the physical screen.
	int PhysicalWidth();
	/// Height of the physical screen.
	int PhysicalHeight();
	/// Width of the given card.
	int CardWidth(int imagenumber,int sizeprcnt,int angle);
	/// Height of the given card.
	int CardHeight(int imagenumber,int sizeprcnt,int angle);
	/// Rotation recommended for collection etc.
	int Rotation(int imagenumber,int sizeprcnt);
	/// Check that card image is loaded and load or generate if not.
	void LoadIfUnloaded(int imagenumber,int size,int angle);
	/// Hide mouse.
	void HideMouse();
	/// Show mouse.
	void ShowMouse();
	/// Alarm sound.
	void Beep();
	/// Visual alert.
	void Blink(int enabled);
	/// Set fullscreen mode on/off.
	void Fullscreen(bool mode);
	/// Return true if in fullscreen mode.
	bool IsFullscreen()
	{return fullscreen;}		

	/// Allocate new surface and return it's number.
	int AllocateSurface(int w,int h);
	/// Select the surface where all drawing commands affect next. Return the previous surface number.
	int SelectSurface(int num);
	/// Free allocated surface.
	void FreeSurface(int num);
	/// Draw surface to the screen.
	void DrawSurface(int x,int y,int num);
	/// Return width of the surface.
	int SurfaceWidth(int num) const;
	/// Return height of the surface.
	int SurfaceHeight(int num) const;
			
	/// Make sure that all drawings on given area are shown on screen.
	void UpdateScreen(int x0,int y0,int w,int h);
	/// Make sure that all drawings shown on screen.
	void UpdateScreen();
		
	/// Draw card image. Alpha value is from 0 (transparent) to 255 (opaque)
	void DrawCardImage(int imagenumber,int x,int y,int sizeprcnt,int angle,int alpha=255);
		
	/// Wipe out area clean.
	void ClearArea(int x0,int y0,int w,int h);
	/// Wipe out whole surface.
	void Clear()
	{ClearArea(0,0,ScreenWidth(),ScreenHeight());}
	/// Set clipping area for a surface.
	void SetClipping(int surface,int x0,int y0,int w,int h);
	/// Turn clipping off for a surface.
	void ClippingOff(int surface);	
		
	/// Draw a filled box with given color.
	void DrawFilledBox(int x0,int y0,int w,int h,Color c);
	/// Draw a box with given color.
	void DrawBox(int x0,int y0,int w,int h,Color c);
	/// Draw a triangle up.
	void DrawTriangleUp(int x0,int y0,int h,Color c);
	/// Draw a triangle down.
	void DrawTriangleDown(int x0,int y0,int h,Color c);

	/// Load font file from primary file or secondary, if primary not found, and return font number.
	int LoadFont(const string& primary,const string& secondary="");
        /// Check if font is loaded with the given point size and load it if necessary.
	void CheckFont(int fontnumber,int pointsize) const;
	/// Draw text.
	void DrawText(int fontnumber,int pointsize,int x0,int y0,const string& text,Color textcolor,Color backgroundcolor);
	/// Draw text with shadow added.
	void DrawTextShadow(int fontnumber,int pointsize,int x0,int y0,const string& text,Color textcolor,Color backgroundcolor);
	/// Text height in pixels.
	int TextHeight(int fontnumber,int pointsize,const string& text) const;
	/// Text width in pixels.
	int TextWidth(int fontnumber,int pointsize,const string& text) const;
	/// Recommended space between lines in pixels.
	int TextLineSkip(int fontnumber,int pointsize) const;

	/// Load free form jpeg image to the image collection and return its number. Color c is set as invisible.
	int LoadImage(const string& filename,Color c);
	/// Draw free form jpeg image.
	void DrawImage(int image_number,int x,int y,int scale=100,int alpha=255,Color colorkey=Color());
	/// Scale image to new size.
	void ScaleImage(int image_number,int neww,int newh);
	/// Return width of the image.
	int ImageWidth(int image_number);
	/// Return height of the image.
	int ImageHeight(int image_number);
	/// Return 1 if number is valid image number.
	int ValidImage(int number);
	/// Return (r,g,b) of the pixel (x,y). Note: No validity checking for arguments.
	void GetRGB(int image_number,int x,int y,int &r,int &g,int &b);
		
	/// Halt until key is pressed.
	void WaitKeyPress();
	/// Halt for 'delay' milliseconds. Return command if user performs noticeable action. If not, return empty command "".
	Command WaitCommand(int delay);

        /// Calculate width of the text.
	int GetTextWidth(string text,TextStyle style);
        /// Calculate height of one line of text.
	int GetTextHeight(string text,TextStyle style);
        /// Calculate height of the text restricted to the given width.
	int GetTextHeight(string text,TextStyle style,int width);
        /// Remove and return the next text line at most 'maxwidth' points wide and update style according to formatting commands.		
	string GetNextTextLineAndStyle(string& _text,TextStyle& style,int& w,int& h,int maxwidth);
        /// Remove and return the next text line at most 'maxwidth' points wide.
	string GetNextTextLine(string& _text,TextStyle style,int& w,int& h,int maxwidth)
	{return GetNextTextLineAndStyle(_text,style,w,h,maxwidth);}
        /// Remove and return next text element (word, spaces, inline image or formatting command) and it's size and style.
	string GetNextTextElement(string& text,TextStyle& style,int& w,int& h,int maxwidth);
        /// Render text to surface.
	void DrawTextToSurface(int fontnumber,int pointsize,int x0,int y0,const string& text,Color color,Color bgcolor,bool addshadow=false);
		
	/// Draw a line of text using current font and interpreting all formatting commands. Update coordinates to the beginning of the next line. If move_upward set, render from the bottom to the top. Update TextStyle argument according to formatting commands.
	void RenderTextLine(const string& text,TextStyle style,int& x0,int& y0,int width,bool move_upward=false,bool fit_text=false,bool cut_text=false);
	/// Split up to lines and render text to the box. Update TextStyle argument according to formatting commands.
	void RenderText(const string& text,TextStyle& style,int x0,int y0,int width,int height);
	/// Decrease the text size until it's width is less than given width.
	void RenderTextFitHorizontal(const string& text,TextStyle& style,int x0,int y0,int width,int height);
	/// Decrease the text size until it's height is less than given height.
	void RenderTextFitVertical(const string& text,TextStyle& style,int x0,int y0,int width,int height);
	/// Same as RenderText, but use black color to render shadow text before rendering text.
	void RenderTextShadow(const string& text,TextStyle& style,int x0,int y0,int width,int height);
	/// Register inline image for text renderer.
	void RegisterInlineImage(const string& filename,const string& tag);

	/// Load an andio file and return it's handle number
	int LoadSound(const string& filename);
	/// Play a sounds using it's handle number
	void PlaySound(int sound_number);
	/// Load a card sound if needed and returns it's handle number
	int LoadCardSound(int imagenumber);
    };

} // namespace Driver

#endif
