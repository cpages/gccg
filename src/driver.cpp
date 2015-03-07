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

#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include "driver.h"
#include "error.h"
#include "carddata.h"
#include "tools.h"

using namespace Database;

namespace Driver
{
	Driver* driver;
	bool nographics=false;
	bool nocache=true;
	map<string,int> inlineimage;
	
	int Driver::ScreenWidth()
	{
		return scrw;
	}

	int Driver::ScreenHeight()
	{
		return scrh;
	}

	int Driver::PhysicalWidth()
	{
		return physw;
	}

	int Driver::PhysicalHeight()
	{
		return physh;
	}

	int Driver::Rotation(int imagenumber,int sizeprcnt)
	{
		int w,h;

		w=CardWidth(imagenumber,sizeprcnt,0);
		h=CardHeight(imagenumber,sizeprcnt,0);
		if(w==CardHeight(0,sizeprcnt,0) && h==CardWidth(0,sizeprcnt,0))
			return 270;

		return 0;
	}
	
	string Driver::GetNextTextElement(string& text,TextStyle& style,int& w,int& h,int maxwidth)
	{
		size_t n=0;
		string ret;

		if(text=="")
			return "";

		// Return blank space as is.
		while(n < text.length() && IsSpace(text[n]) && text[n]!='\n')
			n++;

		if(n)
		{
		}

		// Get a tag if found.
		else if(text[0]=='{')
		{
			while(n < text.length())
				if(text[n]=='}')
				{
					n++;
					break;
				}
				else
					n++;
		}

		// Cut from the next word break.
		else
		{
			while(n < text.length())
				if(text[n]=='.' || text[n]==',' || text[n]==';' || text[n]==':' || text[n]=='!' || text[n]=='?' || text[n]=='\n' || text[n]=='-' || text[n]=='/')
				{
					n++;
					break;
				}
				else if(text[n]=='{' || IsSpace(text[n]))
					break;
				else
					n++;
		}

		// Extract the next element from the text.
		if(n==text.length())
		{
			ret=text;
			text="";
		}
		else
		{
			ret=text.substr(0,n);
			text=text.substr(n);
		}

		w=0;
		h=0;
		// Compute the size for normal text.
		if(ret[0]!='{')
		{
			w=TextWidth(style.font,style.pointsize,ret);

			// Shorten the word if it does not fit.
			while(w > maxwidth && ret.length())
			{
				w-=TextWidth(style.font,style.pointsize,ret.substr(ret.length()-1,1));
				text=ret.substr(ret.length()-1,1)+text;
				ret=ret.substr(0,ret.length()-1);
			}

			h=TextHeight(style.font,style.pointsize,ret);
		}
		// Compute the size for tags.
		else if(ret=="{|}")
		{
			w=3;
			h=TextHeight(style.font,style.pointsize,"|");
		}
		else if(ret=="{hr}")
		{
			w=w-2*style.margin;
			h=1;
		}
		else if(ret=="{lb}")
		{
			w=TextWidth(style.font,style.pointsize,"{");
			h=TextHeight(style.font,style.pointsize,"{");
		}
		else if(ret=="{rb}")
		{
			w=TextWidth(style.font,style.pointsize,"}");
			h=TextHeight(style.font,style.pointsize,"}");
		}
		else if(inlineimage.find(ret)!=inlineimage.end())
		{
			w=ImageWidth(inlineimage[ret]);
			h=ImageHeight(inlineimage[ret]);
		}
		else if(ret.substr(0,3)=="{sz" && ret.substr(ret.length()-1,1)=="}")
		{
			int sz=atoi(ret.substr(3,ret.length()-4).c_str());
			if(sz < 6)
				sz=6;
			else if(sz > 255)
				sz=255;
			style.pointsize=sz;
		}
		else if(ret.substr(0,5)=="{font" && ret.substr(ret.length()-1,1)=="}")
		{
			int n=atoi(ret.substr(5,ret.length()-6).c_str());
			if(n < 0)
				n=0;
			else if(n > 6)
				n=6;
			style.font=n;
		}
		else if(ret.substr(0,5)=="{card" && ret.substr(ret.length()-1,1)=="}")
		{
			int n=atoi(ret.substr(5,ret.length()-6).c_str());
			string name="_unknown_";
			
			if(n >= 0 && n < Database::cards.Cards())
				name=Database::cards.Name(n);
			
			w=TextWidth(style.font,style.pointsize,name);
			h=TextHeight(style.font,style.pointsize,name);
		}
		else if(ret=="{red}")
			style.color=RED;
		else if(ret=="{green}")
			style.color=GREEN;
		else if(ret=="{black}")
			style.color=BLACK;
		else if(ret=="{blue}")
			style.color=BLUE;
		else if(ret=="{yellow}")
			style.color=YELLOW;
		else if(ret=="{white}")
			style.color=WHITE;
		else if(ret=="{brown}")
			style.color=Color(149,79,29);
		else if(ret=="{gold}")
			style.color=Color(205,173,0);
		else if(ret=="{gray}")
			style.color=Color(77,77,77);
		else if(ret=="{silver}")
			style.color=Color(154,154,154);
		else if(ret=="{magenta}")
			style.color=Color(205,0,205);
		else if(ret=="{orange}")
			style.color=Color(255,127,0);
		else if(ret=="{cyan}")
			style.color=Color(4,197,204);
		else if(ret=="{shadow}")
			style.shadow=true;
		else if(ret=="{noshadow}")
			style.shadow=false;
		else if(ret=="{reset}")
		{
			bool old=style.shadow;
			style=TextStyle();
			style.shadow=old;
		}
          else
          {
               int r=atoi(ret.substr(1).c_str());
               int g=-1;
               int b=-1;
          
               if(r>0 || ret.substr(1,2)=="0,")
               {
				for(size_t n=0; n < ret.length(); n++)
				{
					if(ret[n]==',')
					{
						n++;
						g=atoi(ret.substr(n).c_str());
						for(; n < ret.length(); n++)
							if(ret[n]==',')
							{
								b=atoi(ret.substr(n+1).c_str());
								break;
							}

						break;
					}
				}

				if(r>=0 && g>=0 && b>=0 && r<256 && g<256 && b<256)
				{
					style.color.r=r;
					style.color.g=g;
					style.color.b=b;
				}
			}
		}

		return ret;
	}

	string Driver::GetNextTextLineAndStyle(string& _text,TextStyle& style,int& w,int& h,int maxwidth)
	{
		string text,word,ret;
		int w0,h0;

		text=_text;
		w=0;h=0;
		while(text != "")
		{
// 			cout << "text=[" << text << "] -> [";
			word=GetNextTextElement(text,style,w0,h0,maxwidth);
// 			cout << word << "] " << w0 << "x" << h0 << " (maxw " << maxwidth << ")" << endl;
			if(w+w0 <= maxwidth || ret=="")
			{
				if(word.length() && word[word.length()-1]=='\n')
				{
					ret+=word.substr(0,word.length()-1);
					_text=text;
					h=(h0 > h ? h0 : h);
					w+=w0;
					break;
				}
				ret+=word;
				_text=text;
				h=(h0 > h ? h0 : h);
				w+=w0;
			}
			else
				break;
		}

		return ret;
	}

	void Driver::RenderText(const string& text,TextStyle& style,int x0,int y0,int width,int height)
	{
		string src=text;
		list<string> lines;

		int texth=0;
		int w,h;
		do
		{
			lines.push_back(GetNextTextLine(src,style,w,h,width));
			if(texth)
				texth+=TextLineSkip(style.font,style.pointsize);
			
			texth+=GetTextHeight(lines.back(),style);
			
		} while(src != "");
		
		if(style.valign==1)
			y0+=height/2-texth/2;
		else if(style.valign==2)
			y0+=height-texth;

		for(list<string>::iterator i=lines.begin(); i!=lines.end(); i++)
			RenderTextLine(*i,style,x0,y0,width);
	}

	void Driver::RenderTextShadow(const string& text,TextStyle& style,int x0,int y0,int width,int height)
	{
		style.shadow=true;
		RenderText(text,style,x0,y0,width,height);
	}

	void Driver::RegisterInlineImage(const string& filename,const string& tag)
	{
            string f=FindFile(filename,"graphics");
            if(f=="")
            {
                cerr << "Warning: Image missing: "+filename << endl;
                f=FindFile("q.png","graphics");
            }
		inlineimage[tag]=LoadImage(f,Color(0,0,0));
	}
	
} // namespace Driver
