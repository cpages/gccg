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

#include <stdio.h>
#include <ctype.h>
#include <algorithm>
#include <math.h>
#include "game.h"
#include "carddata.h"
#include "error.h"
#include "tools.h"

#define SHADING_ALPHA_LEVEL 120

using namespace CCG;
using namespace Database;
using namespace Evaluator;
using namespace Driver;
using namespace std;

#ifdef CURSES_VERSION
void draw_box(int x0,int y0,int w,int h,::Driver::Color c);
void clear_box(int x0,int y0,int w,int h);
#endif

// extern vector<int> CCG::marker_image;

void Object::Draw()
{
#ifdef SDL_VERSION

    // draw object text

    // Calculate text height
    int texth=0;

    for (vector<string>::const_iterator i = texts.begin(); i != texts.end(); i++)
	texth += driver->GetTextHeight(*i, grp.text);

    // Render text
    int x0 = 0;
    int y0 = 0;

    switch (grp.text.valign)
    {
      case 0:		y0 += grp.text.margin;		 			break;	// top
      case 1:     y0 += grp.h/2 - texth/2;	 			break;	// center
      case 2:		y0 += grp.h - grp.text.margin - texth;	break;	// bottom
	  //	case 3:		y0 -= grp.text.margin;					break;  // above top
	  //	case 4:		y0 += grp.h;						break;	// below bottom
    }

    Color old = grp.text.color;

    for (vector<string>::const_iterator i = texts.begin(); i != texts.end(); i++)
    {
	grp.text.color = old;
	driver->RenderTextLine(*i, grp.text, x0, y0, grp.w, grp.fit);
    }

    grp.text.color = old;

    // Draw markers
    list<int> marks;

    for (size_t i = 0; i < marker_image.size(); i++)
	for (int j = markers[i]; j > 0; j--)
	    marks.push_back(i);

    if(marks.size())
    {
	int markw = driver->ImageWidth(marker_image[0]);
	int markh = driver->ImageHeight(marker_image[0]);

	int max = int((grp.w - 4) / markw) * int((grp.h - 4) / markh);
	int scale = 100;

	while(max < (int)marks.size() && scale > 10)
	{
	    markw *= 3;
	    markh *= 3;
	    scale *= 3;

	    markw /= 4;
	    markh /= 4;
	    scale /= 4;

	    max = int((grp.w - 4) / markw) * int((grp.h - 4) / markh);
	}

	if(scale > 10)
	{
	    int columns = (grp.w-4)/markw;

	    if(columns <= 0 || marks.size() <= 1)
		columns = 1;
	    else if(marks.size() <= 4 && columns > 2)
		columns = 2;
	    else if(marks.size() <= 9 && columns > 3)
		columns = 3;
	    else if(marks.size() <= 16 && columns > 4)
		columns = 4;
	    else if(marks.size() <= 25 && columns > 5)
		columns = 5;
	    else if(marks.size() <= 36 && columns > 6)
		columns = 6;
	    else if(marks.size() <= 49 && columns > 7)
		columns = 7;
	    else if(marks.size() <= 64 && columns > 8)
		columns = 8;

	    int rows = marks.size() / columns;

	    if (marks.size() % columns != 0)
		rows++;

	    if (markh*rows > grp.h)
	    {
		columns=(grp.w-4)/markw;
		rows=marks.size()/columns;

		if (marks.size() % columns != 0)
		    rows++;
	    }

	    int x0 = grp.w/2 - columns*markw/2;
	    int y0 = grp.h/2 - rows*markw/2;
	    int _x0 = x0;

	    int j = 0;
	    for (list<int>::iterator i = marks.begin(); i != marks.end(); i++, j++)
	    {
		driver->DrawImage(*i, x0, y0, scale, 230,Color(0,0,0));
		x0 += markw;

		if(j % columns == columns-1)
		{
		    y0 += markh;
		    x0 = _x0;
		}
	    }
	}
    }
#endif
}

void Image::Draw()
{
#ifdef SDL_VERSION
    driver->DrawImage(image, 0, 0);
    Object::Draw();
#endif
}

void CardInPlay::Draw()
{
#ifdef SDL_VERSION
    driver->DrawCardImage(Number(), 0, 0, table->TableCardSize(), angle);
    Object::Draw();
#endif
}

void Hand::Draw()
{
#ifdef SDL_VERSION
    int x0,y0;
    x0=0;
    y0=0;

    // calculate width of text box for drawing count of cards in hand
    // starts at 17x17, expands in width with each extra digit, up to 3 digits
    int bxw = 17 + (Cards() > 99 ? 16 : (Cards() > 9 ? 8 : 0));

    if(!grp.tiny)
    {
	int dx = 0, dy = 0;

	for (size_t i = 0; i < Cards(); i++)
	{
	    int rot = driver->Rotation(cards[i], size);
	    driver->DrawCardImage(cards[i], x0+dx, y0+dy, size, rot);

	    int w = driver->CardWidth(cards[i], size, rot);
	    int h = driver->CardHeight(cards[i], size, rot);

	    // draw name of card on card
	    // 			int textX = x0+dx + (i == 0 ? bxw : 19);
	    // 			int textY = y0+dy + 2;
	    // 			int textW = w - (i == 0 ? bxw : 0);

	    // true to fit within widget
	    //  			driver->RenderTextLine("{shadow}{font4}" + Database::cards.Name(cards[i]),
	    //  					grp.text, textX, textY, textW, false, true);

	    if (i==0)
		// draw border around card
		driver->DrawBox(x0+dx, y0+dy, w, h, grp.fg);
	    else
	    {
		// draw border: width 1, from bottom edge of previous card to
		// bottom edge of current card, along current card's left edge
		driver->DrawBox(x0+dx, y0+dy + h-Hand::yoffset-1, 1, yoffset, grp.fg);

		// draw border: height 1, along bottom edge of card
		driver->DrawBox(x0+dx, y0+dy + h-1, w, 1, grp.fg);

		// draw border: height 1, from right edge of previous card to
		// right edge of current card, along current card's top edge
		driver->DrawBox(x0+dx + w-Hand::xoffset-1, y0+dy, xoffset, 1, grp.fg);

		// draw border: width 1, along right edge of card
		driver->DrawBox(x0+dx + w-1, y0+dy, 1, h, grp.fg);
	    }

	    // offset next card
	    dx += xoffset;
	    dy += yoffset;
	}
    }


    // clear out background-colored space, draw border, output text
    driver->DrawFilledBox(x0, y0, bxw, 17, grp.bg);
    driver->DrawBox(x0, y0, bxw, 17, grp.fg);
    driver->DrawText(grp.text.font, grp.text.pointsize, x0+4, y0+2, ToString(Cards()), grp.text.color, grp.bg);

    Object::Draw();
#endif
}

void CardBox::Draw()
{
#ifdef SDL_VERSION
    int x=0;
    int y=0;
    int rot=0;
	
    for(size_t i=0; i<Cards(); i++)
    {
	if(rotate)
	    rot=driver->Rotation(cards[i],size);
		
	driver->DrawCardImage(cards[i],x,y,size,rot);

	x+=max_cardwidth;
	if(int(i) % cards_per_row == cards_per_row-1)
	{
	    x=0;
	    y+=max_cardheight-overlap;
	}
    }
    Object::Draw();
#endif
}

void Deck::Draw()
{
#ifdef SDL_VERSION
    int dx = 0;
    int dy = 0;

    for(int i=0; i<=int(Size()); i+=10)
    {
	if(i+10 >= int(Size()))
	    i = int(Size())-1;

	if(i>=0)
	{
	    int rot = driver->Rotation(cards[i], size);
	    driver->DrawCardImage(cards[i], dx, dy, size, rot);
	}
	else
	{
	    int rot = driver->Rotation(0, size);
	    driver->DrawCardImage(0, dx, dy, size, rot,SHADING_ALPHA_LEVEL);
	}

	dx += 3;
	dy += 3;
    }

    string sz=ToString(Size());
    driver->DrawText(grp.text.font,grp.text.pointsize+3,grp.w - driver->TextWidth(grp.text.font,grp.text.pointsize+3,sz) - 5 ,grp.h - driver->TextHeight(grp.text.font,grp.text.pointsize+3,sz) - 5,sz,grp.text.color,grp.bg);

    Object::Draw();
#endif
}

void MessageBox::WriteMessage(const string& text)
{
#ifdef TEXT_VERSION
    if(name != "input" && name!="chat msg")
    {
	cout << "\e[1D";
	if(name != "")
	    cout << "\e[0m\e[4m" << name << "\e[24m: ";

	int len=0;
	string code,out;
	bool center=false;
	bool right=false;
		
	for(size_t i=0; i<text.length(); i++)
	    if(text[i]!='{')
	    {
		out+=text[i];
		len++;
	    }
	    else
	    {
		code="";
		while(text[i]!='}')
		    code+=text[i++];
		code+='}';

		if(code=="{red}")
		    out+="\e[31m";
		else if(code=="{orange}" || code=="{yellow}" || code=="{brown}" || code=="{gold}")
		    out+="\e[33m";
		else if(code=="{reset}" || code=="{white}" || code=="{gray}" || code=="{silver}")
		    out+="\e[37m";
		else if(code=="{cyan}" || code=="{blue}")
		    out+="\e[34m";
		else if(code=="{green}")
		    out+="\e[32m";
		else if(code=="{black}")
		    out+="\e[30m";
		else if(code=="{magenta}")
		    out+="\e[35m";
		else if(code=="{hr}")
		    out+="----------------------------------------------------------------------------";
		else if(code.substr(0,3)=="{sz")
		{
		    if(atoi(code.substr(3).c_str()) > 10)
			out+="\e[1m";
		}
		else if(code=="{center}")
		    center=true;
		else if(code=="{right}")
		    right=true;
		else if(code=="{left}")
		    center=right=false;
		else if(code=="{font0}")
		    out+="\e[22m\e[23m\e[24m\e[27m";
		else if(code=="{font0}")
		    out+="\e[22m\e[3m\e[24m\e[27m";
		else if(code=="{font1}" || code=="{font2}")
		    out+="\e[22m\e[3m\e[24m\e[27m";
		else if(code=="{font3}")
		    out+="\e[1m\e[23m\e[24m\e[27m";
		else if(code=="{lb}")
		    out+="{";
		else if(code=="{rb}")
		    out+="}";
		else if(code=="{shadow}")
		    out+="\e[9m";
		else if(code=="{noshadow}")
		    out+="\e[29m";
		else
		    out+=code;
	    }

	if(center)
	    out=string("                                                                            ").substr(0,(76-len)/2)+out;
	else if(right)
	    out=string("                                                                            ").substr(0,(76-len))+out;
				
	cout << out << endl << "\e[0m>" << flush;
    }
#else

    string line,s=text;
    int w,h;
    TextStyle style;
    if(linestyle.size())
	style=linestyle.back();
    else
	style=grp.text;
	
    do
    {
	linestyle.push_back(style);
	if(grp.fit)
	    line=driver->GetNextTextLineAndStyle(s,style,w,h,(grp.w-2)*2);
	else
	    line=driver->GetNextTextLineAndStyle(s,style,w,h,grp.w-2);
	content.push_back(line);
		
	if(content.size() > lines)
	{
	    content.pop_front();
	    linestyle.pop_front();
	}

    } while(s != "");

    grp.redraw=true;
#endif
}

void MessageBox::Draw()
{
#ifdef CURSES_VERSION
    if(name=="input")
    {
	clear_box(0,driver->PhysicalHeight()-1,driver->PhysicalWidth(),1);
	if(content.size())
	    driver->DrawText(grp.text.font,grp.text.pointsize,0,driver->ScreenHeight()-1,content.back().c_str(),grp.text.color,grp.bg);
    }
    else if(name=="msg")
    {
	draw_box(0,driver->PhysicalHeight()-2,driver->PhysicalWidth(),1,MAGENTA);
    }
#else
    int x0,y0;
    x0=0;
    y0=0;

    driver->DrawFilledBox(x0,y0,grp.w,grp.h,grp.bg);
	
    x0 += grp.text.margin;
    y0 += grp.h-grp.text.margin;

    bool first=true;
    TextStyle style;
	
    deque<string>::reverse_iterator i;
    deque<TextStyle>::reverse_iterator j;

    i=content.rbegin() + offset;
    j=linestyle.rbegin() + offset;

    while(i!=content.rend())
    {
	if(y0 < grp.text.margin)
	    break;
	style=*j;
	style.shadow=true;
	if(first && offset)
	{
	    first=false;
	    driver->RenderTextLine(*i + "{magenta} (more...)",style,x0,y0,grp.w-2*grp.text.margin,true,grp.fit);
	}
	else
	    driver->RenderTextLine(*i,style,x0,y0,grp.w-2*grp.text.margin,true,grp.fit);

	i++;
	j++;
    }
	
    Object::Draw();
#endif
}

void CardBook::DrawBackground(int x0,int y0)
{
#ifdef SDL_VERSION
    driver->DrawFilledBox(x0,y0,grp.w,grp.h,grp.bg);
    driver->DrawBox(x0,y0,grp.w,grp.h,grp.fg);

    int namew=driver->TextWidth(0,16,Name());
    driver->DrawText(0,16,x0+grp.w/2 - namew/2,y0+H(3),Name(),grp.text.color,grp.bg);
    driver->DrawText(0,16,x0+margin,y0+H(3),"Pg."+ToString(page+1)+" of "+ToString(LastPage()+1),grp.text.color,grp.bg);

    string title=TitleOfPage(page);

    TextStyle style=grp.text;
    style.align=2;
    style.valign=1;
    style.shadow=0;

    driver->RenderTextFitHorizontal(title,style,x0 + grp.w/2 + namew/2 + margin,y0,grp.w/2-namew/2-margin*2,titleh);
	
#endif
}

void CardBook::DrawTab(int section, const string& title,bool active)
{
#ifdef SDL_VERSION

    int x,y,w,h;

    TabPos(section,x,y);
    w=TabWidth(section);
    h=TabHeight(section);

    if(x + w < 0 || x >= grp.w)
	return;

    Color col=grp.bg;
    if(!active)
	col.Darken();
    
    // Background
    driver->DrawFilledBox(x+1,y+1,w-2,h-1,col);
    
    // Borders
    driver->DrawFilledBox(x+2,y,w-4,1,grp.fg);
    driver->DrawFilledBox(x+1,y+1,1,1,grp.fg);
    driver->DrawFilledBox(x+w-2,y+1,1,1,grp.fg);
    driver->DrawFilledBox(x,y+2,1,h-2,grp.fg);
    driver->DrawFilledBox(x+w-1,y+2,1,h-2,grp.fg);
    if(!active)
	driver->DrawFilledBox(x,y+h-1,w,1,grp.fg);

    // Text

    driver->SetClipping(surface,x+1,y+1,w-6,h-5);

    string txt=title;

    int txth=driver->GetTextHeight(txt,grp.text);
    driver->RenderText(txt,grp.text,x+3,y+(tabh-txth)/2,grp.w,tabh-2);

    driver->ClippingOff(surface);
#endif
}

void CardBook::DrawTabs()
{
#ifdef SDL_VERSION
    driver->DrawFilledBox(0,titleh,grp.w,1,grp.fg);
    driver->DrawFilledBox(0,titleh+toolh-1,grp.w,1,grp.fg);
#endif

    int current=CurrentSection();
    if(current!=-1)
    {
	for(int i=0; i<current; i++)
	    DrawTab(i,titles[i].first,false);
	for(int i=Sections()-1; i>current; i--)
	    DrawTab(i,titles[i].first,false);

	DrawTab(current,titles[current].first,true);
    }
}

void CardBook::DrawSlot(int x,int y,int w,int h,bool odd_even,bool selected)
{
#ifdef SDL_VERSION
    if(grp.compact)
    {
	if(odd_even)
	    driver->DrawFilledBox(x,y,w,h,grp.bg);
	else
	    driver->DrawFilledBox(x,y,w,h,grp.fg);
	if(selected)
	    driver->DrawBox(x-1,y-1,w+2,h+2,grp.fg);
    }
    else
    {
	driver->DrawFilledBox(x,y,w,h,BLACK);
	driver->DrawBox(x-1,y-1,w+2,h+2,grp.fg);
	if(selected)
	{
	    driver->DrawBox(x-3,y-3,w+6,h+6,WHITE);
	    driver->DrawBox(x-4,y-4,w+8,h+8,RED);
	    driver->DrawBox(x-5,y-5,w+10,h+10,WHITE);
	}
    }
#endif
}

void CardBook::DrawSpreadsheetCard(BookEntry& entry,int x,int y,int w,int h,int trade_limit)
{
#ifdef SDL_VERSION
    TextStyle style;

    style.color=BLACK;
    driver->RenderText(Database::cards.Name(entry.card),style,x+2,y+2,w-4,h-4);
#endif
}

void CardBook::DrawCard(BookEntry& entry,int x,int y,int w,int h,int trade_limit)
{
    if(grp.compact)
    {
	DrawSpreadsheetCard(entry,x,y,w,h,trade_limit);
	return;
    }

#ifdef SDL_VERSION
    static char format_buffer[128];
    int lineskip=driver->TextHeight(0,12,"T")+driver->TextLineSkip(0,12);

    // Image
    int rot,sz;

    sz=table->BookCardSize();
    rot=driver->Rotation(entry.card,sz);

    if(entry.count)
	driver->DrawCardImage(entry.card,x,y,sz,rot);
    else
	driver->DrawCardImage(entry.card,x,y,sz,rot,SHADING_ALPHA_LEVEL);

    // Card name
    string name=Database::cards.Name(entry.card);
    if(name.length() > 50)
        name=name.substr(0,50)+"...";
    driver->RenderTextShadow(name,grp.text,x+2,y+2,w-4,h-4);

    //	// S
    //	if(entry.forsale == 0 && entry.count > trade_limit)
    //		driver->RenderTextShadow("{sz20}{blue}S",grp.text,x+2,y+h-20,100,40);

    string sEntryCount = "{sz20}" + ToString(entry.count);

    if (entry.forsale == 0 && entry.count > trade_limit)
		sEntryCount = sEntryCount+"**";
	else if( entry.forsale > 0 && (entry.count-entry.forsale) > trade_limit )
		sEntryCount = sEntryCount+"*";

    // Have
    if(entry.count)
	driver->RenderTextShadow(sEntryCount, grp.text, x+2, y+h-24, w-4, 40);
    //		driver->RenderTextShadow(sEntryCount, grp.text, x+2, y+15, w-14, 40);

    // Rarity
    driver->RenderTextShadow("{right}{sz12}"+Database::cards.AttrValue(entry.card,"rarity"),
      grp.text, x+2, y+h-20, w-6, 40);
    //			grp.text, x+2, y+35, w-16, 40);

    // Nmb. of cards in deck
    y=y+driver->CardHeight(entry.card,table->BookCardSize(),0)/2-16;
	
    if(entry.ondeck)
	driver->RenderTextShadow("Deck: "+ToString(entry.ondeck),grp.text,x+2,y,w-4,40);

    y+=lineskip;
	
    // Nmb. of cards for sale
    if(entry.forsale)
    {
	if(entry.forsale>0)
	    driver->RenderTextShadow("Sell: "+ToString(entry.forsale),grp.text,x+2,y,w-4,40);
	else
	    driver->RenderTextShadow("Want: "+ToString(-entry.forsale),grp.text,x+2,y,w-4,40);
    }

    y+=lineskip;
	
    // My price
    if(entry.count>0 && (entry.forsale>0 || (entry.myprice!=0.0 && entry.forsale>=0)))
    {
#ifdef WIN32
	sprintf(format_buffer,"%.2f",entry.myprice);
#else
	snprintf(format_buffer,127,"%.2f",entry.myprice);
#endif
	driver->RenderTextShadow("{center}$"+string(format_buffer),grp.text,x+2,y,w-4,40);
    }

    y+=lineskip;

    // Seller and price
    if(entry.seller != "")
    {
	string emph;
						
	if(fabs(entry.price - entry.myprice) > 0.00000001 && entry.forsale > 0)
		emph="{yellow}";
	driver->RenderTextShadow("{center}"+emph+entry.seller+":",grp.text,x+2,y,w-4,40);
	y+=lineskip;
#ifdef WIN32
	sprintf(format_buffer,"%.2f",entry.price);
#else
	snprintf(format_buffer,127,"%.2f",entry.price);
#endif
	driver->RenderTextShadow("{center}"+emph+"$"+string(format_buffer),grp.text,x+2,y,w-4,40);
    }
#endif
}

void CardBook::DrawTitle(const string& title,int x,int y,int w,int h)
{
#ifdef SDL_VERSION
    string out;
					
    driver->DrawFilledBox(x,y,w,h,Color(74,145,239));
    driver->DrawBox(x,y,w,h,BLACK);

    if(grp.compact)
	out=title;
    else
    {
	for(size_t i=0; i<title.length(); i++)
	    if(title[i]=='|')
		out+="\n{hr}";
	    else
		out+=title[i];
    }
					
    TextStyle old=grp.text;
    grp.text.valign=1;
    grp.text.align=1;
    grp.text.shadow=0;
    grp.text.pointsize=14;
    driver->RenderTextFitVertical(out,grp.text,x+2,y+2,w-4,h-4);
    grp.text=old;
#endif
}

void CardBook::Draw()
{
#ifdef SDL_VERSION

    Data setting=(*parser)("options{\"trade_limit\"};");
    int trade_limit=setting.IsInteger() ? setting.Integer() : 4;

    int x0,y0;
    x0=0;
    y0=0;

    DrawBackground(x0,y0);
    DrawTabs();

    int x,y;
    int n;

    n=page*rows*columns;
    y=y0+titleh+toolh+margin;

    if(grp.compact)
    {
	int barw=(columns*cardw + (columns-1)*margin);
	int step=(rows*cardh + (rows-1)*margin) / (rows*columns);
		
	x=x0+margin;
	for(int i=0; i<rows; i++)
	{
	    for(int j=0; j<columns; j++)
	    {
		DrawSlot(x,y,barw,step,n%2==0,n==slot);

		if(n < (int)index.size())
		{
		    if(index[n].card!=-1)
			DrawCard(cards[index[n].card],x,y,barw,step,trade_limit);
		    if(index[n].title!="")
			DrawTitle(index[n].title,x,y,barw,step);
		}
			
		n++;
				
		y+=step;
	    }
	}
    }
    else
    {
	for(int i=0; i<rows; i++)
	{
	    x=x0+margin;
	    for(int j=0; j<columns; j++)
	    {
		DrawSlot(x,y,cardw,cardh,false,n==slot);

		if(n < (int)index.size())
		{
		    if(index[n].card!=-1)
			DrawCard(cards[index[n].card],x,y,cardw,cardh,trade_limit);
		    if(index[n].title!="")
			DrawTitle(index[n].title,x,y,cardw,cardh);
		}
			
		n++;

		x+=margin+cardw;
	    }
	    y+=margin+cardh;
	}
    }
#endif
}

void Menu::Draw()
{
#ifdef SDL_VERSION
    int x0,y0;
	
    x0=0;
    y0=0;

    TextStyle old;

    driver->DrawFilledBox(x0,y0,grp.w,grp.h,grp.bg);
    driver->DrawBox(x0,y0,grp.w,grp.h,grp.fg);

    // Draw title.
    int x=x0+1,y=y0+1;
	
    old=grp.text;
    grp.text.align=1;
    grp.text.font=3;
    driver->RenderTextLine(Name(),grp.text,x,y,grp.w-2);
    grp.text=old;

    if(entries.size()==0)
	return;
	
    // Draw lines.
    driver->DrawBox(x0,y0+titleh,grp.w,1,grp.fg);

    for(int i=1; i<columns; i++)
	driver->DrawBox(x0+i*entryw,y0+titleh,1,grp.h-titleh,grp.fg);

    // Draw entries.
    list<MenuEntry>::iterator i;
    list<int>::iterator j;

    grp.text.valign=1;
    int n=0;
    i=entries.begin();
    j=separators.begin();
    for(int c=0; c<columns; c++)
	for(int r=0; r<rows; r++)
	{
	    x=x0+c*entryw;
	    y=y0+r*entryh+titleh;
	    if(i!=entries.end())
	    {
		if(j!=separators.end() && n==*j)
		{
		    driver->DrawBox(x,y,entryw,1,grp.fg);
		    j++;
		}
		string s=(*i).text;
		driver->RenderTextFitHorizontal(s,grp.text,x,y,entryw,entryh);

		s=(*i).shortcut;
		if(s!="")
		    driver->RenderTextFitHorizontal(s,grp.text,x+entryw-driver->TextWidth(grp.text.font,grp.text.pointsize,s)-5,y,entryw,entryh);
		i++;
		n++;
	    }
	}
#endif
}

void ListBox::Draw()
{
#ifdef SDL_VERSION
    int x,y,x0,y0,off;
    x0=0;
    y0=0;

    driver->DrawFilledBox(x0,y0,grp.w,grp.h,grp.bg);
    driver->DrawBox(x0,y0,grp.w,grp.h,grp.fg);

    TextStyle old=grp.text;

    grp.text.align=1;
    grp.text.font=3;
    grp.text.pointsize=14;
    x=x0+1;
    y=y0-1;
    driver->RenderTextLine(Name(),grp.text,x,y,grp.w-2);
    grp.text=old;

    y0+=height;
    driver->DrawBox(x0,y0,grp.w,1,grp.fg);

    grp.text.align=1;
    grp.text.font=3;
    x=x0;
    for(size_t i=0; i<Columns(); i++)
    {
	grp.text.align=align[i];
	grp.text.valign=1;
	driver->DrawBox(x,y0,1,height*(windowsize+1),grp.fg);
	off=(grp.text.align==0 ? 5 : 2);
	driver->RenderText(title[i],grp.text,x+off,y,width[i]-off,height);
	x+=width[i];
    }
    grp.text=old;

    y0+=height;
    driver->DrawBox(x0,y0,grp.w,1,grp.fg);

    int n=first;
    for(int i=0; i<windowsize; i++)
    {
	x=x0;
		
	if(i==0 && first > 0)
	    driver->DrawTriangleUp(x0+1,y0+3,height-8,grp.fg);
	if(i==windowsize-1 && Rows() > (size_t)(first+windowsize))
	    driver->DrawTriangleDown(x0+1,y0+3,height-8,grp.fg);
		
	for(size_t j=0; j<Columns(); j++)
	{
	    grp.text=old;
	    grp.text.align=align[j];
	    grp.text.valign=1;
	    off=(grp.text.align==0 ? 5 : 1);
	    string s=GetEntry(n,j);
            int xx=x+off;
            int yy=y0+1;
	    driver->RenderTextLine(s,grp.text,xx,yy,width[j]-off,false,false,true);
	    x+=width[j];
	}
	y0+=height;
	n++;
		
	driver->DrawBox(x0,y0,grp.w,1,grp.fg);
    }

    grp.text=old;
#endif
}
