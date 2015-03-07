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

using namespace CCG;
using namespace Database;
using namespace Evaluator;
using namespace std;

vector<int> CCG::marker_image;
Table* CCG::table=0;
int Table::design_width=DEFAULT_DESIGN_WIDTH;
int Table::design_height=DEFAULT_DESIGN_HEIGHT;

//
// class Object
// ============

Object::Object(int n,Object* prnt,string _name)
{
    surface=-1;
    parent=prnt; number=n; name=_name;
    for(size_t i=0; i<marker_image.size(); i++)
	markers.push_back(0);
}

Object::~Object()
{
}

Object* Object::Root()
{
    Object* obj=this;
    while(obj->parent)
    {
	obj=obj->parent;
	if(obj==this)
	    return 0;
    }

    return obj;
}

void Object::DetachAll()
{
    list<Object*>::const_iterator i;

    for(i=attachments.begin(); i!=attachments.end(); i++)
	(*i)->parent=0;

    Detach();
}

void Object::Dump() const
{
    cout << endl << "  #" << number << "  '" << Name() << "' ";
    if(parent)
	cout << " LNK #" << parent->number;

    list<Object*>::const_iterator i;
    for(i=attachments.begin(); i!=attachments.end(); i++)
	cout << " ATC #" << (*i)->Number();

    for(size_t i=0; i<marker_image.size(); i++)
	if(markers[i])
	    cout << " MRK" << i << " x " << markers[i] << " ";

    for(vector<string>::const_iterator i=texts.begin(); i!=texts.end(); i++)
	cout << " TXT '" << *i << "' ";

    cout << endl << "  ";

    if(grp.visible)
	cout << "visible ";
    if(grp.draggable)
	cout << "draggable ";
    if(grp.sticky)
	cout << "sticky ";
    if(grp.ontop)
	cout << "ontop ";
    if(grp.clickable)
	cout << "clickable ";
    if(grp.highlight)
	cout << "highlight ";
    if(grp.compact)
	cout << "compact ";
    if(grp.tiny)
	cout << "tiny ";
    if(grp.fit)
	cout << "fit ";
	
    cout << endl << "  ";

    cout << "color " << grp.text.color.r << "," << grp.text.color.g << "," << grp.text.color.b << " ";
    cout << "font " << grp.text.font << " ";
    cout << "pointsize " << grp.text.pointsize << " ";
    cout << "align " << grp.text.align << " ";
    cout << "valign " << grp.text.valign << " ";
    cout << "margin " << grp.text.margin << " ";
    cout << "shadow " << grp.text.shadow << " ";

    cout << endl << "  ";
}

bool Object::ClickableAt(int x,int y) const
{
    return (grp.visible && grp.clickable && ATBOX(x,y,grp.x,grp.y,grp.w,grp.h));
}

int Object::GroupX() const
{
    int n,ret=grp.x;
    list<Object*>::const_iterator i;

    for(i=attachments.begin(); i!=attachments.end(); i++)
    {
	n=(*i)->GroupX();
	if(n < ret)
	    ret=n;
    }

    return ret;
}

int Object::GroupX2() const
{
    int n,ret=grp.x + grp.w - 1;
    list<Object*>::const_iterator i;

    for(i=attachments.begin(); i!=attachments.end(); i++)
    {
	n=(*i)->GroupX2();
	if(n > ret)
	    ret=n;
    }

    return ret;
}

int Object::GroupY() const
{
    int n,ret=grp.y;
    list<Object*>::const_iterator i;

    for(i=attachments.begin(); i!=attachments.end(); i++)
    {
	n=(*i)->GroupY();
	if(n < ret)
	    ret=n;
    }

    return ret;
}

int Object::GroupY2() const
{
    int n,ret=grp.y + grp.h - 1;
    list<Object*>::const_iterator i;

    for(i=attachments.begin(); i!=attachments.end(); i++)
    {
	n=(*i)->GroupY2();
	if(n > ret)
	    ret=n;
    }

    return ret;
}

void Object::AddText(const string& s)
{
    texts.push_back(s);
    grp.redraw=true;
}

string Object::Text(int i) const
{
    if(i < 0 || (size_t)i >= texts.size())
	throw Error::Invalid("Object::Text(int)","Invalid text index "+ToString(i));

    return texts[i];
}

void Object::DelText(const string& s)
{
    vector<string>::iterator i;
    i=find(texts.begin(),texts.end(),s);
    if(i!=texts.end())
	texts.erase(i);
    grp.redraw=true;
}

int Object::AllocateSurface(int w,int h)
{
    if(surface!=-1)
    {
	if(Driver::driver->SurfaceWidth(surface)==w && Driver::driver->SurfaceHeight(surface)==h)
	    return surface;

	Driver::driver->FreeSurface(surface);
    }

    grp.redraw=true;
    surface=Driver::driver->AllocateSurface(w,h);

    return surface;
}

string Object::Title() const
{
    if(title!="")
	return title;

    if(name=="")
	return name;

    string t=name;
    t[0]=toupper(t[0]);
	
    return t;
}

int Object::W(int w) const
{
    return table->W(w);
}

int Object::H(int h) const
{
    return table->H(h);
}

//
// class Image
// ===========

void Image::RecalculateSize()
{
    grp.w=Driver::driver->ImageWidth(image);
    grp.h=Driver::driver->ImageHeight(image);
    AllocateSurface(grp.w,grp.h);
}

//
// class CardInPlay
// ================

void CardInPlay::Dump() const
{
    cout << "CardInPlay ";
    Object::Dump();
    cout << " angle: " << angle;
    cout << " " << Database::cards.Name(card);
}

void CardInPlay::Tap()
{
    angle=90;
    grp.redraw=true;
}

void CardInPlay::TapLeft()
{
    angle=270;
    grp.redraw=true;
}

void CardInPlay::Untap()
{
    angle=0;
    grp.redraw=true;
}

void CardInPlay::Invert()
{
    angle=180;
    grp.redraw=true;
}

void CardInPlay::Rot(int ang)
{
    while(ang < 0)
	ang+=360;
	
    angle=ang % 360;
    grp.redraw=true;
}

void CardInPlay::RecalculateSize()
{
    grp.w=Driver::driver->CardWidth(Number(),table->TableCardSize(),angle);
    grp.h=Driver::driver->CardHeight(Number(),table->TableCardSize(),angle);
    AllocateSurface(grp.w,grp.h);
}

//
// class Hand
// ==========

void Hand::Dump() const
{
    cout << "Hand ";
    Object::Dump();
    cout << " (";
    for(size_t i=0; i<cards.size(); i++)
    {
	if(i)
	    cout << " ";
	cout << cards[i];
    }
    cout << ")";
}

void Hand::Add(int card)
{
    cards.push_back(card);
    grp.redraw=true;
}

void Hand::Del(int index)
{
    if(index < 0 || index >= int(cards.size()))
	throw Error::Syntax("Hand::Del(int)","invalid index");

    cards.erase(cards.begin()+index);
    grp.redraw=true;
}

int Hand::CardAt(int x,int y) const
{
    if(Cards()==0 || grp.tiny)
	return -1;

    int dx=grp.x,dy=grp.y,ret=-1;
    int rot;
	
    for(size_t i=0; i<Cards(); i++)
    {
	rot=Driver::driver->Rotation(cards[i],size);
		
	if(ATBOX(x,y,dx,dy,Driver::driver->CardWidth(cards[i],size,rot),Driver::driver->CardHeight(cards[i],size,rot)))
	    ret=i;
	dx+=xoffset;
	dy+=yoffset;
    }

    return ret;
}

bool Hand::ClickableAt(int x,int y) const
{
    if(!Object::ClickableAt(x,y))
	return false;

    // There is only a box with a number.
    if(Cards()==0 || grp.tiny)
	return true;

    return (CardAt(x,y) > -1);
}

void Hand::RecalculateSize()
{
    int rot;
    int dy=0,dx=0;

    if(grp.compact)
    {
	size=table->HandCardSize();
	xoffset=Driver::driver->CardWidth(0,size,0);
	yoffset=0;
    }
    else
    {
	size=table->HandCardSize();
	xoffset=20;
	yoffset=20;
    }

    grp.w=17;
    grp.h=17;
    if(Cards() > 9)
	grp.w+=8;
    if(Cards() > 99)
	grp.w+=8;

    if(!grp.tiny)
	for(size_t i=0; i<Cards(); i++)
	{
	    rot=Driver::driver->Rotation(cards[i],size);
			
	    if(dx+Driver::driver->CardWidth(cards[i],size,rot) > grp.w)
		grp.w=dx+Driver::driver->CardWidth(cards[i],size,rot);
	    if(dy+Driver::driver->CardHeight(cards[i],size,rot) > grp.h)
		grp.h=dy+Driver::driver->CardHeight(cards[i],size,rot);
	    dx+=xoffset;
	    dy+=yoffset;
	}

    AllocateSurface(grp.w,grp.h);
}

//
// class CardBox
// =============

void CardBox::Dump() const
{
    cout << "CardBox ";
    Object::Dump();
    cout << " (";
    for(size_t i=0; i<cards.size(); i++)
    {
	if(i)
	    cout << " ";
	cout << cards[i];
    }
    cout << ")";
}

void CardBox::Del(int index)
{
    if(index < 0 || index >= int(cards.size()))
	throw Error::Syntax("CardBox::Del(int)","invalid index");

    cards.erase(cards.begin()+index);
    grp.redraw=true;
}

int CardBox::CardAt(int x,int y) const
{
    if(Cards()==0)
	return -1;

    int dx=grp.x,dy=grp.y,ret=-1;
    int rot=0;
	
    for(size_t i=0; i<Cards(); i++)
    {
	if(rotate)
	    rot=Driver::driver->Rotation(cards[i],size);
		
	if(ATBOX(x,y,dx,dy,Driver::driver->CardWidth(cards[i],size,rot),Driver::driver->CardHeight(cards[i],size,rot)))
	    ret=i;
	dx+=max_cardwidth;
	if(int(i) % cards_per_row == cards_per_row-1)
	{
	    dx=grp.x;
	    dy+=max_cardheight-overlap;
	}
    }

    return ret;
}

void CardBox::RecalculateSize()
{
    int rot=0;
	
    grp.w=0;
    grp.h=0;
    overlap=0;
    rows=0;
	
    if(grp.compact)
	size=table->HandCardSize();
    else if(grp.tiny)
	size=table->HandCardSize() / 4;
    else 
	size=100;

    // Check if we want rotation.
    int w,h;
    rotate=false;
    if(Cards())
    {
	w=Driver::driver->CardWidth(cards[0],size,0);
	h=Driver::driver->CardHeight(cards[0],size,0);
	
	for(size_t i=1; i<Cards(); i++)
	{		
	    if(Driver::driver->CardWidth(cards[i],size,0) != w ||
	      Driver::driver->CardHeight(cards[i],size,0) != h)
	    {
		rotate=true;
		break;
	    }
	}
    }
	
    // Compute largest card.
    max_cardheight=0;
    max_cardwidth=0;
    for(size_t i=0; i<Cards(); i++)
    {
	if(rotate)
	    rot=Driver::driver->Rotation(cards[i],size);
		
	if(Driver::driver->CardHeight(cards[i],size,rot) > max_cardheight)
	    max_cardheight=Driver::driver->CardHeight(cards[i],size,rot);
	if(Driver::driver->CardWidth(cards[i],size,rot) > max_cardwidth)
	    max_cardwidth=Driver::driver->CardWidth(cards[i],size,rot);
    }

    // Compute dimensions.
    if(Cards())
    {
	cards_per_row=(Driver::driver->PhysicalWidth()-20)/max_cardwidth;
	cards_per_col=(Driver::driver->PhysicalHeight()-20)/max_cardheight;
	rows=Cards() / cards_per_row + (Cards() % cards_per_row != 0);
	if(rows > cards_per_col)
	    overlap=max_cardheight - (cards_per_col-1) * max_cardheight / (rows-1);
	grp.h=(max_cardheight - overlap) * (rows - 1) + max_cardheight;
	if(rows==1)
	    grp.w=max_cardwidth * Cards();
	else
	    grp.w=max_cardwidth * cards_per_row;
    }

    AllocateSurface(grp.w,grp.h);
}

//
// class Deck
// ==========

void Deck::Dump() const
{
    cout << "Deck ";
    cout << Size() << " cards ";
    Object::Dump();
}

void Deck::Del(int index)
{
    if(index < 0 || index >= int(cards.size()))
	throw Error::Syntax("Deck::Del(int)","invalid index");

    cards.erase(cards.begin()+(size_t)index);
    grp.redraw=true;
}

bool Deck::ClickableAt(int x,int y) const
{
	if(!Object::ClickableAt(x,y))
		return false;
	if(Size()<11)
		return true;
	
	// Every 10 cards, the next card is drawn at an offset of (3,3).
	// Therefore, there is a triangular arrangement of 3x3 squares at
	// the top-right and bottom-left of the surface that are drawn as
	// empty space (assuming the deck contains 11 or more cards).
	unsigned int iy=(y-grp.y)/3;
	unsigned int ix=(grp.x+grp.w-1-x)/3;
	if(ix+iy <= (Size()-11)/10)
		return false;
	
	iy=(grp.y+grp.h-1-y)/3;
	ix=(x-grp.x)/3;
	if(ix+iy <= (Size()-11)/10)
		return false;
	
	return true;
}

void Deck::RecalculateSize()
{
    int rot;
    int dy=0,dx=0;

    grp.w=0;
    grp.h=0;

    if(grp.compact)
	size=table->DeckCardSize()/2;
    else if(grp.tiny)
	size=table->DeckCardSize()/4;
    else 
	size=table->DeckCardSize();

    for(size_t i=0; i<=Size(); i+=10)
    {
	if(Size() && i<Size())
	{
	    rot=Driver::driver->Rotation(cards[i],size);
	    if(dx + Driver::driver->CardWidth(cards[i],size,rot) > grp.w)
		grp.w=dx + Driver::driver->CardWidth(cards[i],size,rot);
	    if(dy + Driver::driver->CardHeight(cards[i],size,rot) > grp.h)
		grp.h=dy + Driver::driver->CardHeight(cards[i],size,rot);
	}
	else if(Size()==0)
	{
	    rot=Driver::driver->Rotation(0,size);
	    if(dx + Driver::driver->CardWidth(0,size,rot) > grp.w)
		grp.w=dx + Driver::driver->CardWidth(0,size,rot);
	    if(dy + Driver::driver->CardHeight(0,size,rot) > grp.h)
		grp.h=dy + Driver::driver->CardHeight(0,size,rot);
	}
					
	dx+=3;
	dy+=3;
    }

    AllocateSurface(grp.w,grp.h);
}

// class MessageBox
// ================

void MessageBox::ScrollUp()
{
    if((size_t) offset < content.size())
	offset++;

    grp.redraw=true;
}

void MessageBox::ScrollDown()
{
    if(offset > 0)
	offset--;

    grp.redraw=true;
}

//
// class CardBook
// ==============

CardBook::CardBook(int num,Object* parent,const string& name,int basecard,int w,int h,Evaluator::Parser<Table> *_parser) : Object(num,parent,name){
    parser=_parser;
    rows=w;
    columns=h;
    page=0;
    slot=-1;
    margin=W(10);
    titleh=H(25);
    toolh=titleh;
    toolw=grp.w-margin;
    cardw=Driver::driver->CardWidth(basecard,table->BookCardSize(),0);
    cardh=Driver::driver->CardHeight(basecard,table->BookCardSize(),0);
    tabh=1;
    tabskip=0;
    grp.text.pointsize=11;

    cards.resize(Database::cards.Cards());
    card_index.resize(Database::cards.Cards());

    for(size_t i=0; i < cards.size(); i++)
	cards[i]=BookEntry(i);
    for(size_t i=0; i < card_index.size(); i++)
	card_index[i]=-1;

    list<BookIndexEntry> default_index;
    SetIndex(default_index);
}

CardBook::~CardBook()
{
}

void CardBook::SetIndex(const list<BookIndexEntry>& entries)
{
    page=0;
    slot=-1;
    for(size_t i=0; i < card_index.size(); i++)
	card_index[i]=-1;

    index.resize(entries.size());
	
    list<BookIndexEntry>::const_iterator i;
    int j=0;
    for(i=entries.begin(); i!=entries.end(); i++)
    {
	index[j]=*i;
	if(i->card != -1)
	    card_index[i->card]=j;
	j++;
    }

    titles.clear();
    for(int i=0; i<Entries(); i++)
	if(Entry(i).title!="")
	    titles.push_back(pair<string,int>(Entry(i).title,i));

    RecalculateSize();
    grp.redraw=true;
}

int CardBook::SetSlot(const string& title)
{
    for(size_t i=0; i<index.size(); i++)
	if(index[i].card==-1 && index[i].title==title)
	{
	    slot=i;
	    grp.redraw=true;
	    return i/(rows*columns);
	}

    slot=-1;
    grp.redraw=true;
    return -1;
}

int CardBook::SetSlot(int card)
{
    if(card < 0 || card >= Database::cards.Cards())
	throw Error::Range("CardBook::SetSlot(int)","invalid card number "+ToString(card));

    int s=card_index[card];

    if(s!=-1)
    {
	slot=s;
	grp.redraw=true;
	return PageOf(card);
    }

    slot=-1;
    grp.redraw=true;
    return -1;
}

void CardBook::Dump() const
{
    cout << "Book ";
    cout << "pg." << page << " (max. " << LastPage() << ")";
	
    Object::Dump();
}

void CardBook::RecalculateSize()
{
    grp.w=cardw*columns + margin*(columns+1);
    grp.h=cardh*rows + margin*rows + titleh + toolh + margin;
    AllocateSurface(grp.w,grp.h);

    int sections=Sections();

    // Initial varibles
    tabwidths=vector<int>(sections);
    toolw=grp.w-margin;
    toolh=titleh;
    tabh=toolh-3;
    tabskip=0;

    if(sections<1)
	return;
    if(sections==1)
    {
	tabwidths[0]=grp.w-margin;
	return;
    }

    // Compute maximum text width,
    int txtw;

    int maxtabw=0;
    for(int i=0; i<sections; i++)
    {
	txtw=Driver::driver->GetTextWidth(titles[i].first,grp.text) + grp.text.pointsize;
	if(txtw > maxtabw)
	    maxtabw=txtw;
    }
    tabskip=(grp.w-margin)/sections;

    if(tabskip < W(30))
	tabskip=W(30);

    // Initialize tab width vector.
    int x0,y0,x1,y1;

    for(int i=0; i<sections; i++)
    {
	tabwidths[i]=Driver::driver->GetTextWidth(titles[i].first,grp.text) + grp.text.pointsize;

	// Restrict the size of the tab.
	if(tabwidths[i] > (grp.w-margin)/2)
	    tabwidths[i]=(grp.w-margin)/2;

	// Increase the length of otherwise hidden tabs.
	if(i)
	{
	    TabPos(i-1,x0,y0);
	    TabPos(i,x1,y1);

	    if((x1+tabwidths[i]) - (x0+tabwidths[i-1])<tabskip/2)
		tabwidths[i]=tabwidths[i-1] - tabskip/2;
	}
    }
    toolw=(sections-1)*tabskip + tabwidths[sections-1];

    // If we have less tabs than space available, make them equal length.
    if(toolw < grp.w-margin)
    {
	for(int i=0; i<sections; i++)
	    tabwidths[i]=(grp.w-margin)/sections;
	tabskip=tabwidths[0];
    }
    toolw=(sections-1)*tabskip + tabwidths[sections-1];

    // Increase short tabs to fill empty space.
    for(int i=0; i<sections; i++)
    {
	if(tabwidths[i] <= tabskip)
	    tabwidths[i]=tabskip+1;
    }
    toolw=(sections-1)*tabskip + tabwidths[sections-1];
}

void CardBook::TabPos(int section,int& x,int& y) const
{
    int sections=Sections();

    x=(grp.w-toolw)/2;
    y=titleh+toolh-tabh;

    // Single section.
    if(sections < 1)
	return;

    // All sections fit to the collection.
    if(toolw <= grp.w)
    {
	x+=tabskip*section;
	return;
    }

    // Justify the active tab to center.
    int active=CurrentSection();
    int x0;
	
    x0=grp.w/2;
    x=x0 + (section-active)*tabskip;
    x-=TabWidth(active)/2;

    // Check if we have space on the left side.
    if(x - section*tabskip > margin/2)
	x=margin/2 + section*tabskip;

    // Check if we have space on the right side.
    if(x + (sections-section-1)*tabskip + tabwidths[sections-1] < grp.w-margin/2)
	x=grp.w-margin/2 - (sections-section-1)*tabskip - tabwidths[sections-1];
}

int CardBook::TabWidth(int section) const
{
    return tabwidths[section];
}

int CardBook::TabHeight(int section) const
{
    return tabh;
}

int CardBook::TabAt(int mx,int my) const
{
    int x,y;
    int active=CurrentSection();

    mx-=grp.x;
    my-=grp.y;

    int sections=Sections();
    if(sections==0)
	return -1;

    TabPos(active,x,y);
    if(ATBOX(mx,my,x,y,TabWidth(active),TabHeight(active)))
	return active;

    for(int i=active-1; i>=0; i--)
    {
	TabPos(i,x,y);
	if(ATBOX(mx,my,x,y,TabWidth(i),TabHeight(i)))
	    return i;
    }
    for(int i=active+1; i<sections; i++)
    {
	TabPos(i,x,y);
	if(ATBOX(mx,my,x,y,TabWidth(i),TabHeight(i)))
	    return i;
    }

    return -1;
}

int CardBook::SlotAt(int mx,int my) const
{
    int x,y;

    if(grp.compact)
    {
	y=grp.y+titleh+toolh+margin;
	x=grp.x+margin;

	int barw=(columns*cardw + (columns-1)*margin);
	int step=(rows*cardh + (rows-1)*margin) / (rows*columns);
		
	for(int i=0; i<rows; i++)
	{
	    for(int j=0; j<columns; j++)
	    {
		if(ATBOX(mx,my,x,y,barw,step))
		{
		    if(page*columns*rows + i*columns + j >= (int)index.size()
		      || index[page*columns*rows + i*columns + j].card == -1)
			return -1;
		    else
			return j+i*columns;
		}
			
		y+=step;
	    }
	}
    }
    else
    {
	y=grp.y+titleh+toolh+margin;
	for(int i=0; i<rows; i++)
	{
	    x=grp.x+margin;
	    for(int j=0; j<columns; j++)
	    {
		if(ATBOX(mx,my,x,y,cardw,cardh))
		{
		    if(page*columns*rows + i*columns + j >= (int)index.size()
		      || index[page*columns*rows + i*columns + j].card == -1)
			return -1;
		    else
			return j+i*columns;
		}
			
		x+=margin+cardw;
	    }
	    y+=margin+cardh;
	}
    }
	
    return -1;
}

int CardBook::CardAt(int idx) const
{
    if(idx < 0 || idx >= columns*rows)
	throw Error::Range("CardBook::CardAt(int)","illegal index "+ToString(idx));

    int n=page*rows*columns + idx;

    if(n >= (int)index.size())
	return -1;
    if(index[n].card == -1)
	return -1;
	
    return index[n].card;
}

int CardBook::LastPage() const
{
    int sz=index.size();
    if(sz < rows*columns)
	return 0;
    if(sz % (rows*columns)==0)
	return sz/(rows*columns) - 1;
    else
	return sz/(rows*columns);
}

BookIndexEntry& CardBook::Entry(int n)
{
    if(n < 0 || (size_t)n >= index.size())
	throw Error::Range("CardBook::Entry(int)","invalid entry number "+ToString(n));

    grp.redraw=true;

    return index[n];
}

list<BookIndexEntry*> CardBook::EntriesOfPage(int n)
{
    if(n < 0 || n > LastPage())
	throw Error::Range("CardBook::EntriesOfPage(int)","invalid page number "+ToString(n));

    list<BookIndexEntry*> ret;
    int entry=rows*columns*n;
    for(int i=0; i<rows*columns; i++)
    {
	if((size_t)entry < index.size())
	{
	    ret.push_back(&index[entry]);
	    entry++;
	}
    }

    return ret;
}

const BookIndexEntry& CardBook::Entry(int n) const
{
    if(n < 0 || (size_t)n >= index.size())
	throw Error::Range("CardBook::Entry(int)","invalid entry number "+ToString(n));

    return index[n];
}

BookEntry& CardBook::EntryOf(int n)
{
    if(n < 0 || n >= (int)cards.size())
	throw Error::Range("CardBook::EntryOf(int)","invalid card number "+ToString(n));

    grp.redraw=true;
	
    return cards[n];
}

int CardBook::PageOf(int n) const
{
    if(n < 0 || n >= Database::cards.Cards())
	throw Error::Range("CardBook::PageOf(int)","invalid card number "+ToString(n));

    if(card_index[n]==-1)
	return -1;
	
    return card_index[n]/(rows*columns);
}

bool CardBook::CardVisible(int c) const
{
    if(c < 0 || c >= (int)card_index.size())
	throw Error::Range("CardBook::CardVisible(int)","invalid card number");

    return PageOf(c)==Page();
}

void CardBook::SetPage(int newpage)
{
    if(newpage < 0 || newpage > LastPage())
	throw Error::Range("CardBook::SetPage(int)","invalid page "+ToString(newpage));

    page=newpage;

    if(slot!=-1)
	if(int(slot/(rows*columns))!=Page())
	    slot=-1;
	
    grp.redraw=true;
}

string CardBook::TitleOfPage(int pg) const
{
    if(pg < 0 || pg > LastPage())
	throw Error::Invalid("CardBook::TitleOfPage(int)","Invalid page number "+ToString(pg));
	
    int n=page*rows*columns;

    while(n >= 0)
    {
	if(n<(int)index.size() && index[n].card==-1)
	    return index[n].title;
	n--;
    }

    return "";
}

int CardBook::Sections() const
{
    return titles.size();
}

const string& CardBook::Section(int section) const
{
    static const string empty;

    if(section < 0 || section >= Sections())
	return empty;

    return titles[section].first;
}

int CardBook::CurrentSection() const
{
    int page;

    if(slot!=-1)
    {
	for(int i=0; i<int(titles.size()); i++)
	{
	    if(slot==titles[i].second)
		return i;
	    else if(slot < titles[i].second)
		return i-1;
	}

	return int(titles.size())-1;
    }

    page=Page();
    
    for(int i=0; i<int(titles.size()); i++)
	if(page==titles[i].second / (Rows()*Columns()))
	    return i;
	else if(page < titles[i].second / (Rows()*Columns()))
	    return i-1;

    return int(titles.size())-1;
}

//
// class Menu
// ==========

void Menu::Dump() const
{
    cout << "Menu ";
    cout << entries.size() << " entries ";
    Object::Dump();
}

void Menu::RecalculateSize()
{
    // Calulate entry size.
    list<MenuEntry>::iterator i;

    entryw=Driver::driver->GetTextWidth(Name(),grp.text)+5;
    titleh=Driver::driver->GetTextHeight(Name(),grp.text);
    entryh=titleh;
    if(entries.size()==0)
    {
	columns=rows=0;
		
	grp.w=entryw+50;
	grp.h=titleh;

	AllocateSurface(grp.w,grp.h);
	return;
    }
	
    for(i=entries.begin(); i!=entries.end(); i++)
    {
	int w,h;
		
	w=Driver::driver->GetTextWidth((*i).text,grp.text) + Driver::driver->GetTextWidth((*i).shortcut,grp.text) + 20;
	if(w > entryw)
	    entryw=w;
		
	h=Driver::driver->GetTextHeight((*i).text,grp.text);
	if(h > entryh)
	    entryh=h;
    }

    // Calculate layout.
    int max_rows;
    max_rows=Driver::driver->ScreenHeight() / entryh - 2;

    columns=1;
    rows=entries.size();
    while(rows > max_rows)
    {
	columns++;
	rows=entries.size() / columns;
	if(entries.size() % columns)
	    rows++;
    }
	
    // Calculate object dimensions.
    grp.w=columns*entryw;
    grp.h=rows*entryh+titleh;

    if(grp.w > Driver::driver->ScreenWidth() && grp.text.pointsize > 11)
    {
	grp.text.pointsize-=2;
	RecalculateSize();
	return;
    }

    AllocateSurface(grp.w,grp.h);
}

void Menu::Add(const string& text,const string& shortcut,const string& action)
{
    if(text=="{hr}")
	separators.push_back(entries.size());
    else
	entries.push_back(MenuEntry(text,shortcut,action));

    grp.redraw=true;
}

int Menu::At(int x,int y) const
{
    if(ATBOX(x,y,grp.x,grp.y,grp.w,grp.h))
    {
	int x0,y0;
		
	list<MenuEntry>::const_iterator i;
	int j=0;
		
	i=entries.begin();
	for(int c=0; c<columns; c++)
	    for(int r=0; r<rows; r++)
	    {
		x0=grp.x+c*entryw;
		y0=grp.y+r*entryh+titleh;
		if(i!=entries.end())
		{
		    if(ATBOX(x,y,x0,y0,entryw,entryh))
			return j;
		    i++;
		    j++;
		}
	    }
    }
	
    return -1;
}

const MenuEntry& Menu::operator[](int n) const
{
    if(n < 0 || n >= (int)entries.size())
	throw Error::Range("Menu::operator[](int)","invalid entry number "+ToString(n));

    list<MenuEntry>::const_iterator i=entries.begin();
    while(n--)
	i++;
	
    return *i;
}

//
// class ListBox
// =============

ListBox::ListBox(int num,Object* parent,const string& name,int columns,int rows,int wsize,int _height) : Object(num,parent,name)
{
    first=0;
    key=0;
    height=_height * Driver::driver->PhysicalHeight() / Driver::driver->ScreenHeight();
    windowsize=wsize;
    title.resize(columns);
    width.resize(columns);
    align.resize(columns);
    row.resize(rows);
    for(size_t i=0; i<Columns(); i++)
	width[i]=50 * Driver::driver->PhysicalWidth() / Driver::driver->ScreenWidth();
}

void ListBox::SetColumn(int col,int w,const string& t)
{
    if(col < 0 || (size_t)col>=Columns())
	throw Error::Range("ListBox::SetColumn(int,int,const string&)","invalid column number "+ToString(col));

    width[col]=w * Driver::driver->PhysicalWidth() / Driver::driver->ScreenWidth();
    title[col]=t;
    grp.redraw=true;
}

void ListBox::SetAlign(int col,int algn)
{
    if(col < 0 || (size_t)col>=Columns())
	throw Error::Range("ListBox::SetAlign(int,int)","invalid column number "+ToString(col));
    if(algn < 0 || algn > 2)
	throw Error::Range("ListBox::SetAlign(int,int)","invalid alignment "+ToString(algn));

    align[col]=algn;
    grp.redraw=true;
}

void ListBox::SetEntry(int r,int col,const string& content)
{
    if(col < 0 || (size_t)col>=Columns())
	throw Error::Range("ListBox::SetEntry(int,int,const string&)","invalid column number "+ToString(col));
    if(r < 0)
	throw Error::Range("ListBox::SetEntry(int,int,const string&)","invalid row number "+ToString(r));

    if((size_t) r >= row.size())
	row.resize(r+1);

    if(row[r].size() != Columns())
	row[r]=vector<string>(Columns());
	
    row[r][col]=content;
    grp.redraw=true;
}

const string& ListBox::GetEntry(int r,int col)
{
    if(col < 0 || (size_t)col>=Columns())
	throw Error::Range("ListBox::GetEntry(int,int)","invalid column number "+ToString(col));
    if(r < 0)
	throw Error::Range("ListBox::GetEntry(int,int)","invalid row number "+ToString(r));

    if((size_t) r >= row.size())
	row.resize(r+1);

    if(row[r].size() != Columns())
	row[r]=vector<string>(Columns());

    return row[r][col];
}

bool ListBox::IsEmptyRow(int r) const
{
    if(r < 0)
	throw Error::Range("ListBox::IsEmptyRow(int,int)","invalid row number "+ToString(r));
    if((size_t) r >= row.size())
	return true;

    for(size_t c=0; c<row[r].size(); c++)
	if(row[r][c]!="")
	    return false;
	
    return true;
}

size_t ListBox::Rows() const
{
    int r=row.size();
    if(r==0)
	return 0;

    r--;
	
    while(r > 0 && IsEmptyRow(r))
	r--;
	
    return (size_t)r+1;
}

void ListBox::Clear()
{
    for(size_t r=0; r < Rows(); r++)
	for(size_t c=0; c < Columns(); c++)
	    SetEntry(r,c,"");

    grp.redraw=true;
}

void ListBox::Dump() const
{
    cout << "Listbox ";
    cout << Columns() << " columns " << Rows() << " rows ";
    Object::Dump();
}

int ListBox::At(int x,int y) const
{
    if(ATBOX(x,y,grp.x,grp.y,grp.w,grp.h))
    {
	y-=grp.y;
	if(y < height*2)
	    return -1;

	return (y/height)-2+first;
    }

    return -1;
}

void ListBox::SortRows(int r1,int r2,int c,int c2)
{
    if(r1 >= r2)
	return;
    if(c < 0 || (size_t)c  >= Columns() || r1 < 0 || r2 < 0 || (size_t) r1 >= row.size() || (size_t) r2 >= row.size())
	throw Error::Range("ListBox::SortRows(int,int,int,int)","invalid column and/or row");

    vector<string> tmp;
    if(c2 != -1)
    {
	if(c2 < 0 ||  (size_t)c2 >= Columns())
	    throw Error::Range("ListBox::SortRows(int,int,int,int)","invalid secondary key");

	for(int r=r1; r<r2; r++)
	    for(int s=r+1; s<=r2; s++)
		if(row[s][c] < row[r][c] || (row[s][c] == row[r][c] && row[s][c2] < row[r][c2]))
		{
		    tmp=row[r];
		    row[r]=row[s];
		    row[s]=tmp;
		}
    }
    else
    {
	for(int r=r1; r<r2; r++)
	    for(int s=r+1; s<=r2; s++)
		if(row[s][c] < row[r][c])
		{
		    tmp=row[r];
		    row[r]=row[s];
		    row[s]=tmp;
		}
    }

    grp.redraw=true;
}

void ListBox::RecalculateSize()
{
    grp.w=1;
    for(size_t i=0; i<Columns(); i++)
	grp.w+=width[i];
    grp.h=(windowsize+2)*height + 1;
    AllocateSurface(grp.w,grp.h);
}

//
// class Table
// ===========

Table::Table(const string& triggerfile1,bool fullscreen,bool debug,bool fulldebug,bool nographics,int scrw,int scrh) : parser(this)
{
    InitializeLibrary();

    debugmode=0;
    if(debug)
	debugmode=1;
    if(fulldebug)
	debugmode=2;
	
    refresh=true;
	
    cout << Localization::Message("Loading %s",triggerfile1) << endl;
    event_triggers.ReadFile(triggerfile1);

    if(debug)
	parser.SetVariable("options.debug",1);
    if(fulldebug)
	Evaluator::debug=true;
	
    cout << Localization::Message("Calling %s","\"init\" \"\"") << endl;
    parser(event_triggers("init",""));
	
    cout << Localization::Message("Initializing graphics driver: resolution %s",ToString(design_width)+"x"+ToString(design_height)) << endl;


    Driver::driver=new Driver::Driver(design_width,design_height,fullscreen,scrw,scrh);
    Driver::nographics=nographics;


    //  	tablex0=design_width/6;
    //  	tabley0=2*design_height/3;
    //  	table_width=design_width/2;
    //  	table_height=design_height/2;
    tablex0=0;
    tabley0=scrh/2;
    table_width=scrw;
    table_height=scrh;

    parser.SetVariable("screen.width",scrw);
    parser.SetVariable("screen.height",scrh);
    parser.SetVariable("design.width",design_width);
    parser.SetVariable("design.height",design_height);
    parser.SetVariable("table.width",table_width);
    parser.SetVariable("table.height",table_height);
	
    marker_image.push_back(Driver::driver->LoadImage(FindFile("red_marker.png","graphics"),Driver::Color(0,0,0)));
    marker_image.push_back(Driver::driver->LoadImage(FindFile("green_marker.png","graphics"),Driver::Color(0,0,0)));
    marker_image.push_back(Driver::driver->LoadImage(FindFile("blue_marker.png","graphics"),Driver::Color(0,0,0)));
    marker_image.push_back(Driver::driver->LoadImage(FindFile("orange_marker.png","graphics"),Driver::Color(0,0,0)));
    marker_image.push_back(Driver::driver->LoadImage(FindFile("yellow_marker.png","graphics"),Driver::Color(0,0,0)));
    marker_image.push_back(Driver::driver->LoadImage(FindFile("black_marker.png","graphics"),Driver::Color(0,0,0)));
    marker_image.push_back(Driver::driver->LoadImage(FindFile("white_marker.png","graphics"),Driver::Color(0,0,0)));
    marker_image.push_back(Driver::driver->LoadImage(FindFile("default_marker.png","graphics"),Driver::Color(0,0,0)));

    if(Driver::use_free_fonts)
    {
        Driver::driver->LoadFont("free/sfd/FreeSansBold.ttf","windows/cumbc.ttf");
        Driver::driver->LoadFont("free/NATIONPP.TTF","windows/NATIONPP.TTF");
        Driver::driver->LoadFont("free/sfd/FreeSans.ttf","windows/arialn.ttf");
        Driver::driver->LoadFont("free/sfd/FreeSerifBold.ttf","windows/thornbcp.ttf");
        Driver::driver->LoadFont("free/sfd/FreeSerif.ttf","windows/thorncp.ttf");
        Driver::driver->LoadFont("free/sfd/FreeMonoBold.ttf","windows/courierb.ttf");
        last_font=Driver::driver->LoadFont("free/sfd/FreeMono.ttf","windows/couriern.ttf");
    }
    else
    {
        Driver::driver->LoadFont("windows/cumbc.ttf","free/sfd/FreeSansBold.ttf");
        Driver::driver->LoadFont("windows/NATIONPP.TTF","free/NATIONPP.TTF");
        Driver::driver->LoadFont("windows/arialn.ttf","free/sfd/FreeSans.ttf");
        Driver::driver->LoadFont("windows/thornbcp.ttf","free/sfd/FreeSerifBold.ttf");
        Driver::driver->LoadFont("windows/thorncp.ttf","free/sfd/FreeSerif.ttf");
        Driver::driver->LoadFont("windows/courierb.ttf","free/sfd/FreeMonoBold.ttf");
        last_font=Driver::driver->LoadFont("windows/couriern.ttf","free/sfd/FreeMono.ttf");
    }

    ::table=this;
}

Table::~Table()
{
    list<Object*>::iterator i;
    for(i=table.begin(); i!=table.end(); i++)
	delete (*i);

    delete Driver::driver;
}

void Table::Warning(const string& message)
{
    cerr << "WARNING: " << message << endl;
}

Evaluator::Data Table::Execute(const string& code)
{
    try
    {
	return parser(code);
    }
    catch(Evaluator::LangErr e)
    {
	static int errornumber=1;
	parser.vardump(Null);
	cerr << endl << flush;
	cerr << "=================================================================" << endl;
	cerr << "ERROR #" << errornumber << endl << endl;
	cerr << endl << "Code caused exception:" << endl;
	if(code.length() > 500)
	    cerr << code.substr(0,500) << "...." << endl;
	else
	    cerr << code << endl;
			
	cerr << endl << e.Message() << endl;

	cerr << endl << "Stacktrace:" << endl;
	parser.stacktrace(Null);
		
	errornumber++;

	parser.ClearStacks();
    }

    return Evaluator::Data();
}

void Table::Dump() const
{
    cout << "TABLE:" << endl;
    list<Object*>::const_iterator i;
    for(i=table.begin(); i!=table.end(); i++)
    {
	cout << (*i)->grp.x << ',' << (*i)->grp.y << ' ' << (*i)->grp.w << 'x' << (*i)->grp.h << ": ";
	(*i)->Dump();
	cout << endl;
    }
}

void Table::GetIntersectingWidgets(int x0,int y0,int w,int h,list<Object*>& obj) const
{
    list<Object*>::const_iterator i;

    for(i=table.begin(); i!=table.end(); i++)
	if((*i)->grp.visible && INTERSECT((*i)->grp.x,(*i)->grp.y,(*i)->grp.w,(*i)->grp.h,x0,y0,w,h))
	    obj.push_back(*i);
}

void Table::Refresh(int x0,int y0,int w,int h,bool update) const
{
    list<Object*> obj;

    GetIntersectingWidgets(x0,y0,w,h,obj);

    Driver::driver->SelectSurface(0);
    Driver::driver->ClearArea(x0,y0,w,h);
    Driver::driver->SetClipping(0,x0,y0,w,h);

    list<Object*>::const_iterator i;
    for(i=obj.begin(); i!=obj.end(); i++)
	Draw(*i);

    Driver::driver->ClippingOff(0);
	
    if(update && refresh)
	Driver::driver->UpdateScreen(x0,y0,w,h);
}

void Table::Refresh(const Widget&old, Object* obj,bool update) const
{
    int x1,y1,x2,y2;
    x1=min(old.x,obj->grp.x);
    y1=min(old.y,obj->grp.y);
    x2=max(old.x+old.w,obj->grp.x + obj->grp.w);
    y2=max(old.y+old.h,obj->grp.y + obj->grp.h);

    Refresh(x1,y1,x2-x1,y2-y1,update);
}

void Table::Refresh(bool update) const
{
    Driver::driver->SelectSurface(0);
    Driver::driver->Clear();
    list<Object*>::const_iterator i;
    for(i=table.begin(); i!=table.end(); i++)
	Draw(*i);

    if(update && refresh)
	Driver::driver->UpdateScreen();
}

void Table::Draw(Object* obj) const
{
    if(!obj->grp.visible)
	return;

    int x0,y0,w,h;
	
    x0=obj->grp.x;
    y0=obj->grp.y;
    w=obj->grp.w;
    h=obj->grp.h;


    if(obj->grp.redraw)
    {
	obj->grp.redraw=false;

	Driver::driver->SelectSurface(obj->Surface());
	if(obj->grp.bg.invisible)
	    Driver::driver->ClearArea(0,0,w,h);
	else
	    Driver::driver->DrawFilledBox(0,0,w,h,obj->grp.bg);
	obj->Draw();
		
	if(obj->grp.highlight)
        {
            Driver::Color c=obj->grp.fg;

            int xx=0,yy=0,ww=w,hh=h;
            int n=obj->grp.highlight;

            while(n--)
            {
                Driver::driver->DrawBox(xx,yy,ww,hh,c);
                xx++; yy++; hh-=2; ww-=2;
                c.MoreTransparent();
            }
            if(obj->grp.highlight > 1)
            {
                Driver::driver->DrawSurface(x0,y0,obj->Surface());
                Driver::driver->DrawFilledBox(xx,yy,ww,hh,c);
            }
        }
    }

    Driver::driver->DrawSurface(x0,y0,obj->Surface());
}

void Table::Move(Object* obj,int dx,int dy)
{
    obj->grp.x += dx;
    obj->grp.y += dy;
}

void Table::MoveGroup(Object* obj,int dx,int dy)
{
    obj->grp.x += dx;
    obj->grp.y += dy;

    list<Object*> G=obj->Group();
    list<Object*>::iterator i;
    for(i=G.begin(); i!=G.end(); i++)
	MoveGroup(*i,dx,dy);
}

void Table::HideGroup(Object* obj)
{
    obj->grp.visible=false;

    list<Object*> G=obj->Group();
    list<Object*>::iterator i;
    for(i=G.begin(); i!=G.end(); i++)
	HideGroup(*i);
}

void Table::ShowGroup(Object* obj)
{
    obj->grp.visible=true;

    list<Object*> G=obj->Group();
    list<Object*>::iterator i;
    for(i=G.begin(); i!=G.end(); i++)
	ShowGroup(*i);
}

void Table::GetGroup(Object* obj,list<Object*>& group) const
{
    group.push_back(obj);
    list<Object*> G=obj->Group();
    list<Object*>::const_iterator i;
	
    for(i=G.begin(); i!=G.end(); i++)
	GetGroup(*i,group);
}

void Table::Raise(Object* obj)
{
    list<Object*>::iterator i;
	
    table.remove(obj);

    for(i=table.begin(); i!=table.end(); i++)
    {
        if(obj->grp.onbottom > (*i)->grp.onbottom)
            break;
        if(obj->grp.ontop < (*i)->grp.ontop)
            break;
    }

    table.insert(i,obj);
	
    Refresh(obj);
}

void Table::Lower(Object* obj)
{	
    list<Object*>::iterator i;

    table.remove(obj);

    if(obj->grp.onbottom)
	i=table.begin();
    else if(obj->grp.ontop)
    {
	for(i=table.begin(); i!=table.end(); i++)
	    if((*i)->grp.ontop)
		break;
    }
    else
    {
	for(i=table.begin(); i!=table.end(); i++)
	    if(!(*i)->grp.onbottom)
		break;
    }

    table.insert(i,obj);

    Refresh(obj);
}

void Table::Raise(const list<Object*>& objs)
{
    list<Object*> order;
    list<Object*>::iterator i;

    for(i=table.begin(); i!=table.end(); i++)
	if(find(objs.begin(),objs.end(),*i)!=objs.end())
	    order.push_back(*i);

    for(i=order.begin(); i!=order.end(); i++)
	Raise(*i);
}

void Table::Delete(Object* obj)
{
    int x,y,w,h;

    x=obj->grp.x;
    y=obj->grp.y;
    w=obj->grp.w;
    h=obj->grp.h;

    obj->DetachAll();
    objects.erase(obj->Number());
    table.remove(obj);

    Refresh(x,y,w,h);
}

bool Table::DragWidget(const string& button,int& scrx,int& scry,int &num)
{
    Object* obj=WidgetAt(scrx,scry);
    if(!obj)
	return false;

    while(obj->grp.dragparent)
	obj=obj->Parent();

    int num0=obj->Number();
	
    Driver::Command c;
    int dx,dy;
    int w,h;
    int upx,upy,upw,uph;

    w=obj->GroupW();
    h=obj->GroupH();

    // Move it.
    while(1)
    {
	do
	{
	    c=Driver::driver->WaitCommand(10);
	} while(c.command=="");

	if(c.command==button+" drag")
	{
	    dx=(c.x-scrx);
	    dy=(c.y-scry);
	    scrx=c.x;
	    scry=c.y;
	    MoveGroup(obj,dx,dy);
	    BOUNDINGBOX(upx,upy,upw,uph,obj->GroupX(),obj->GroupY(),w,h,obj->GroupX()-dx,obj->GroupY()-dy,w,h);
	    Refresh(upx,upy,upw,uph);
	    Driver::driver->UpdateScreen(upx,upy,upw+2,uph+2);
	}
	else if(c.command==button+" drag end")
	{
	    scrx=obj->grp.x;
	    scry=obj->grp.y;
	    num=num0;
	    return true;
	}
    }
}

Object* Table::WidgetAt(int scrx,int scry)
{
    list<Object*>::reverse_iterator i;
    for(i=table.rbegin(); i!=table.rend(); i++)
    {
	if((*i)->ClickableAt(scrx,scry))
	{
	    if((*i)->Type()==TypeCardBox)
	    {
		CardBox* cardbox=dynamic_cast<CardBox*>(*i);
		if(cardbox->CardAt(scrx,scry) < 0)
		    continue;
	    }
	    else if((*i)->Type()==TypeCardBox)
	    {
		Hand* hand=dynamic_cast<Hand*>(*i);
		if(hand->CardAt(scrx,scry) < 0)
		    continue;
	    }
	    
	    return *i;
	}
    }
    
    return 0;
}

bool Table::TryTrigger(const string& key1,const string& key2,Data& value)
{
    string code;
    code=event_triggers(key1,key2);
    if(code=="")
    {
	if(debugmode > 0 && key1!="timer" && key1!="move")
	    cout << "Trigger: \"" << key1 << "\" \"" << key2 << "\" not defined" << endl;
	return 0;
    }

    value=Execute(code);
    if(debugmode > 0 && key1!="timer" && key1!="move")
	cout << "Trigger: \"" << key1 << "\" \"" << key2 << "\" returns: " << tostr(value) << endl;
		
    return 1;
}

void Table::SetTableCardSize(int prcnt)
{
    if(prcnt < 1)
	throw Error::Invalid("Table::SetTableCardSize","Invalide percentage "+ToString(prcnt));

    table_card_size=prcnt;
	
    parser.SetVariable("card.width",(prcnt * Driver::driver->CardWidth(0,100,0))/100);
    parser.SetVariable("card.height",(prcnt * Driver::driver->CardHeight(0,100,0))/100);

    for(list<Object*>::iterator i=table.begin(); i!=table.end(); i++)
	(*i)->RecalculateSize();
	
    Refresh();
}

void Table::SetHandCardSize(int prcnt)
{
    if(prcnt < 1)
	throw Error::Invalid("Table::SetHandCardSize","Invalide percentage "+ToString(prcnt));

    hand_card_size=prcnt;

    parser.SetVariable("card.hand.width",(prcnt * Driver::driver->CardWidth(0,100,0))/100);
    parser.SetVariable("card.hand.height",(prcnt * Driver::driver->CardHeight(0,100,0))/100);

    for(list<Object*>::iterator i=table.begin(); i!=table.end(); i++)
	if((*i)->Type()==TypeHand || (*i)->Type()==TypeCardBox)
	    (*i)->RecalculateSize();
	
    Refresh();
}

void Table::SetDeckCardSize(int prcnt)
{
    if(prcnt < 1)
	throw Error::Invalid("Table::SetDeckCardSize","Invalide percentage "+ToString(prcnt));

    deck_card_size=prcnt;

    parser.SetVariable("card.deck.width",(prcnt * Driver::driver->CardWidth(0,100,0))/100);
    parser.SetVariable("card.deck.height",(prcnt * Driver::driver->CardHeight(0,100,0))/100);

    for(list<Object*>::iterator i=table.begin(); i!=table.end(); i++)
	if((*i)->Type()==TypeDeck)
	    (*i)->RecalculateSize();

    Refresh();
}

void Table::SetBookCardSize(int prcnt)
{
    if(prcnt < 1)
	throw Error::Invalid("Table::SetBookCardSize","Invalide percentage "+ToString(prcnt));

    book_card_size=prcnt;

    parser.SetVariable("card.book.width",(prcnt * Driver::driver->CardWidth(0,100,0))/100);
    parser.SetVariable("card.book.height",(prcnt * Driver::driver->CardHeight(0,100,0))/100);

    for(list<Object*>::iterator i=table.begin(); i!=table.end(); i++)
	if((*i)->Type()==TypeCardBook)
	    (*i)->RecalculateSize();

    Refresh();
}

bool Table::IsObjectNumber(int number)
{
    return objects.find(number)!=objects.end();
}

void Table::Table2Screen(int& x,int& y) const
{
    x=tablex0 + x*table_width/design_width;
    y=tabley0 + y*table_height/design_height;
}

void Table::Screen2Table(int& x,int& y) const
{
    x=((x-tablex0)*design_width)/table_width;
    y=((y-tabley0)*design_height)/table_height;
}

void Table::Width2Screen(int& w) const
{
    w=w*table_width/design_width;
}

void Table::Height2Screen(int& h) const
{
    h=h*table_height/design_height;
}
#ifndef USE_SQUIRREL

#endif
