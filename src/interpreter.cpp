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

#include <cstdlib>
#include "game.h"
#include "parser.h"
#include "carddata.h"

using namespace Evaluator;
using namespace Database;
using namespace CCG;
using namespace Driver;

/// Decalare variable 'name' of given pointer 'type' and check if 'number' represents correct object. If not, throw LangErr. Otherwise initialize variable with pointer to the object.
#define OBJECTVAR(name,type,number) \
  if(!IsObjectNumber(number)) \
    throw LangErr("OBJECTVAR()","object number "+ToString(number)+" does not exist");\
  type* name=dynamic_cast<type*>(objects[number]); \
  if(!name) \
    throw LangErr("OBJECTVAR()","object do not have a correct type");

static void ArgumentError(const char* function,const Data& args)
{
    string s="invalid arguments: ";
    s+=tostr(args).String();
	
    throw LangErr(function,s);
}

static int GetInteger(const char* function,const Data& args)
{
    if(!args.IsInteger())
	ArgumentError(function,args);

    return args.Integer();
}

static string GetString(const char* function,const Data& args)
{
    if(!args.IsString())
	ArgumentError(function,args);

    return args.String();
}

static void Get2Strings(const char* function,const Data& args,string& i1,string& i2)
{
    if(!args.IsList(2) || !args[0].IsString() || !args[1].IsString())
	ArgumentError(function,args);

    i1=args[0].String();
    i2=args[1].String();
}

static void Get2Integers(const char* function,const Data& args,int& i1,int& i2)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError(function,args);

    i1=args[0].Integer();
    i2=args[1].Integer();
}

static void Get3Integers(const char* function,const Data& args,int& i1,int& i2,int& i3)
{
    if(!args.IsList(3) || !args[0].IsInteger() || !args[1].IsInteger() || !args[2].IsInteger())
	ArgumentError(function,args);

    i1=args[0].Integer();
    i2=args[1].Integer();
    i3=args[2].Integer();
}

static void Get6Integers(const char* function,const Data& args,int& i1,int& i2,int& i3,int& i4,int& i5,int& i6)
{
    if(!args.IsList(6) || !args[0].IsInteger() || !args[1].IsInteger() || !args[2].IsInteger() || !args[3].IsInteger() || !args[4].IsInteger() || !args[5].IsInteger())
	ArgumentError(function,args);

    i1=args[0].Integer();
    i2=args[1].Integer();
    i3=args[2].Integer();
    i4=args[3].Integer();
    i5=args[4].Integer();
    i6=args[5].Integer();
}

Object* Table::GetObject(const char* function,const Data& args)
{
    if(args.IsInteger())
    {
	int num=args.Integer();
	Object* o=objects[num];
	if(o)
	    return o;
    }
	
    throw LangErr(function,"Argument "+tostr(args).String()+" is not an object");
}

CardInPlay* Table::GetCard(const char* function,const Data& args)
{
    Object* obj=GetObject(function,args);

    if(obj->Type()==TypeCardInPlay)
    {
	CardInPlay* card=dynamic_cast<CardInPlay*>(obj);
	if(card)
	    return card;
    }

    throw LangErr("Table::GetCard(const Data&,Object*&)","Argument is not a card");
}

/// put_inplay(n,x,y,o,i,p) - Put a card $i$ on the table, center of untapped card at the postition
/// $(x,y)$ and oriented in $o$ degrees clockwise. Object number is $n$
/// and owning player is $p$. Return the object number of new object.
Data Table::put_inplay(const Data& args)
{
    int num,x,y,degrees,image,owner;
	
    Get6Integers("put_inplay",args,num,x,y,degrees,image,owner);
    if(image < 0)
	throw LangErr("Table::put_inplay(const Data&)","Invalid image number");
    else if(image >= cards.Cards())
    {
	Warning("put_inplay: invalid image number "+ToString(image));
	image=0;
    }
    if(degrees < 0 || degrees >= 360)
	throw LangErr("Table::put_inplay(const Data&)","Invalid orientation");

    if(IsObjectNumber(num))
	throw LangErr("put_inplay","Object number "+ToString(num)+" already in use.");

    CardInPlay* card=new CardInPlay(num,0,degrees,image,owner);
    if(!card)
	throw Error::Memory("Table::put_inplay(const Data&)");

    table.push_back(card);
    objects[num]=card;

    Object* obj=card;

    x=x - driver->CardWidth(image,table_card_size,degrees) * driver->ScreenWidth()
	/ driver->PhysicalWidth() / 2;
    y=y - driver->CardHeight(image,table_card_size,degrees) * driver->ScreenHeight()
	/ driver->PhysicalHeight() / 2;
	
    Table2Screen(x,y);
	
    obj->grp.x=x;
    obj->grp.y=y;
    obj->grp.bg=INVISIBLE;
    obj->grp.visible=true;
    obj->grp.draggable=true;
    obj->grp.sticky=false;
    obj->grp.ontop=false;
    obj->grp.clickable=true;
    obj->grp.highlight=false;
    obj->grp.compact=false;
    obj->grp.tiny=false;
	
    table.push_back(obj);
    obj->RecalculateSize();
	
    raise(num);

    return num;
}

/// move_object(n,x,y) - Move an object $n$ to $(x,y)$.
Data Table::move_object(const Data& args)
{
    int num,oldx,oldy,newx,newy;

    Get3Integers("move_object",args,num,newx,newy);
    Object* obj=objects[num];
    if(!obj)
	throw LangErr("move_object","invalid object number");

    oldx=obj->GroupX();
    oldy=obj->GroupY();
	
    if(obj->Type()==TypeCardInPlay)
    {
	Table2Screen(newx,newy);
	newx -= obj->grp.w/2;
	newy -= obj->grp.h/2;
    }
	
    MoveGroup(obj,newx - obj->grp.x,newy - obj->grp.y);
    obj->RecalculateSize();
    Refresh(oldx,oldy,obj->GroupW(),obj->GroupH());
    Refresh(obj->GroupX(),obj->GroupY(),obj->GroupW(),obj->GroupH());

    return Null;
}

/// put_hand(h,i) - Put a card $i$ to a hand $h$. Return {\tt NULL}
/// if $i$ is not a imagenumber. Otherwise return $h$.
Data Table::put_hand(const Data& args)
{
    int hnd,image;
	
    Get2Integers("put_hand",args,hnd,image);
    if(image < 0)
	return Null;
    else if(image >= cards.Cards())
    {
	Warning("put_hand: invalid image number "+ToString(image));
	image=0;
    }

    OBJECTVAR(hand,Hand,hnd);

    hand->Add(image);
    hand->RecalculateSize();
    Refresh(hand);
	
    return hnd;
}

/// del_hand(h,i) - Remove a card with index $i$ from a hand
/// $h$. Return {\tt NULL} if $i$ is not a valid index. Otherwise
/// return card number of the removed card.
Data Table::del_hand(const Data& args)
{
    int hnd,index;
	
    Get2Integers("del_hand",args,hnd,index);

    OBJECTVAR(hand,Hand,hnd);

    if(index < 0 || index >= int(hand->Cards()))
	return Null;

    int w=hand->grp.w,h=hand->grp.h;
    int ret=(*hand)[index];
    hand->Del(index);

    hand->RecalculateSize();
    Refresh(hand->grp.x,hand->grp.y,w,h);
	
    return ret;
}

/// hand(h) - Return the list of card numbers in hand $h$.
Data Table::hand(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("hand",args);
	
    OBJECTVAR(hand,Hand,args.Integer());
	
    Data ret;

    ret.MakeList((int)hand->Cards());
    for(size_t i=0; i<hand->Cards(); i++)
	ret[i]=Data((*hand)[i]);

    return ret;
}

/// deck(d) - Return the list of card numbers in deck $d$.
Data Table::deck(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("deck",args);
	
    OBJECTVAR(deck,Deck,args.Integer());
	
    Data ret;

    ret.MakeList((int)deck->Size());
    for(size_t i=0; i<deck->Size(); i++)
	ret[i]=Data((*deck)[i]);

    return ret;
}

/// del_deck_top(d) - Remove a card from the top of the deck
/// $d$. Return card number if successful and {\tt NULL} if
/// not.
Data Table::del_deck_top(const Data& args)
{
    int dck;
	
    dck=GetInteger("del_deck_top",args);

    OBJECTVAR(deck,Deck,dck);

    if(deck->Size())
    {
	int w=deck->grp.w,h=deck->grp.h;
	int ret=deck->DelTop();

	deck->RecalculateSize();
	Refresh(deck->grp.x,deck->grp.y,w,h);

	return(ret);
    }
	
    return Null;
}

/// del_deck_bottom(d) - Remove a card from the bottom of the deck
/// $d$. Return card number if successful and {\tt NULL} if
/// not.
Data Table::del_deck_bottom(const Data& args)
{
    int dck;
	
    dck=GetInteger("del_deck_bottom",args);

    OBJECTVAR(deck,Deck,dck);

    if(deck->Size())
    {
	int w=deck->grp.w,h=deck->grp.h;
	int ret=deck->DelBottom();

	deck->RecalculateSize();
	Refresh(deck->grp.x,deck->grp.y,w,h);

	return(ret);
    }
	
    return Null;
}

/// put_deck_top(d,e) - If $e$ is an integer, put a card $e$ to
/// the top of the deck. If $e$ is a list, convert each element of the
/// list to integer and add corresponding cards to the top of the
/// deck. Return the deck size if succesful, otherwise {\tt NULL}.
Data Table::put_deck_top(const Data& args)
{
    int dck,image;
	
    if(!args.IsList(2) || !args[0].IsInteger() || (!args[1].IsInteger() && !args[1].IsList()))
	return Null;

    dck=args[0].Integer();
    OBJECTVAR(deck,Deck,dck);

    if(args[1].IsList())
    {
	for(size_t i=0; i<args[1].Size(); i++)
	    deck->AddTop(args[1][i].Integer());
    }
    else
    {
	image=args[1].Integer();
	if(image < 0)
	    return Null;
	else if(image >= cards.Cards())
	    image=0;
	deck->AddTop(image);
    }

    deck->RecalculateSize();
    Refresh(deck);
	
    return int(deck->Size());
}

/// put_deck_bottom(d,e) - If $e$ is an integer, put a card number $e$ to
/// the bottom of the deck. If $e$ is a list, convert each element of the
/// list to integer and add corresponding cards to the bottom of the
/// deck. Return deck size if succesful, otherwise {\tt NULL}.
Data Table::put_deck_bottom(const Data& args)
{
    int dck,image;
	
    if(!args.IsList(2) || !args[0].IsInteger() || (!args[1].IsInteger() && !args[1].IsList()))
	return Null;

    dck=args[0].Integer();
    OBJECTVAR(deck,Deck,dck);

    if(args[1].IsList())
    {
	for(size_t i=0; i<args[1].Size(); i++)
	    deck->AddBottom(args[1][i].Integer());
    }
    else
    {
	image=args[1].Integer();
	if(image < 0)
	    return Null;
	else if(image >= cards.Cards())
	    image=0;

	deck->AddBottom(image);
    }

    deck->RecalculateSize();
    Refresh(deck);
	
    return int(deck->Size());
}

/// clear_deck(d) - Remove all cards from the deck
/// $d$, then redraw it. Return number of cards removed.
Data Table::clear_deck(const Data& args)
{
    int dck;
	
    dck=GetInteger("clear_deck",args);

    OBJECTVAR(deck,Deck,dck);
    
    int ds = deck->Size();

    if(ds)
    {
	int w=deck->grp.w,h=deck->grp.h,x=deck->grp.x,y=deck->grp.y;
	deck->DelAll();
	deck->RecalculateSize();
	
	Refresh(x,y,w,h);
    }
	
    return ds;
}

/// create_msgbox(x_0,y_0,n,s,w,h,L) - Create a message box to the position
/// $(x_0,y_0)$. Object number is $n$, name is $s$ and the size of the box is $w\times h$.
/// Return the object number of the message box created. The maximum
/// number of lines is $L$.
Data Table::create_msgbox(const Data& args)
{
    int num,x,y,w,h,max;
    string name;
	
    if(!args.IsList(7) || !args[0].IsInteger() || !args[1].IsInteger()
      || !args[2].IsInteger() || !args[3].IsString() || !args[4].IsInteger()
      || !args[5].IsInteger()  || !args[6].IsInteger())
	ArgumentError("create_msgbox",args);

    x=args[0].Integer();
    y=args[1].Integer();
    num=args[2].Integer();
    name=args[3].String();
    w=args[4].Integer();
    h=args[5].Integer();
    max=args[6].Integer();

    if(IsObjectNumber(num))
	throw LangErr("create_msgbox","Object number "+ToString(num)+" already in use.");
	
    MessageBox* box=new MessageBox(num,0,max,name);
    if(!box)
	throw Error::Memory("Table::create_msgbox(const Data&)");

    table.push_back(box);
    objects[num]=box;

    box->grp.w=w;
    box->grp.h=h;
    box->grp.x=x;
    box->grp.y=y;
    box->grp.bg=INVISIBLE;
    box->grp.draggable=false;
    box->grp.visible=true;
    box->grp.sticky=true;
    box->grp.ontop=true;
    box->grp.clickable=false;
    box->grp.highlight=false;
    box->grp.compact=false;
    box->grp.tiny=false;
    box->RecalculateSize();
    Refresh(box);

    return num;
}

/// clear_msgbox(o) - Remove all lines from the message box $o$. Throw error if $o$ is not a message box.
Data Table::clear_msgbox(const Data& args)
{
    Object* obj=GetObject("clear_msgbox",args);
    
    if(obj->Type() != TypeMessageBox)
	throw LangErr("clear_msgbox","Object is not a message box.");
	
    MessageBox* box=dynamic_cast<MessageBox*>(obj);
    
    box->Clear();
    Refresh(box);
    
    return Null;
}

/// create_hand(x,y,n,s) - Create a hand object at $(x,y)$. It's object
/// number is $n$ and name $s$. Return $n$.
Data Table::create_hand(const Data& args)
{
    string nm;
    int num,x,y;

    if(!args.IsList(4)
      || !args[0].IsInteger()
      || !args[1].IsInteger() || !args[2].IsInteger()
      || !args[3].IsString())
	ArgumentError("create_hand",args);

    x=args[0].Integer();
    y=args[1].Integer();
    num=args[2].Integer();
    nm=args[3].String();

    if(IsObjectNumber(num))
	throw LangErr("create_hand","Object number "+ToString(num)+" already in use.");
	
    Hand* hand=new Hand(num,0,nm);
    if(!hand)
	throw Error::Memory("Table::create_hand(const Data&)");

    table.push_back(hand);
    objects[num]=hand;

    hand->grp.w=0;
    hand->grp.h=0;
    hand->grp.x=x;
    hand->grp.y=y;
    hand->grp.bg=INVISIBLE;
    hand->grp.draggable=true;
    hand->grp.visible=true;
    hand->grp.sticky=true;
    hand->grp.ontop=true;
    hand->grp.clickable=true;
    hand->grp.highlight=false;
    hand->grp.compact=false;
    hand->grp.tiny=false;
    hand->RecalculateSize();
    Table2Screen(hand->grp.x,hand->grp.y);
	
    Refresh(hand);

    return num;
}

/// create_cardbox(x,y,n,s) - Create a card box object at
/// $(x,y)$. It's object number is $n$ and name $s$. Return $n$.
Data Table::create_cardbox(const Data& args)
{
    string nm;
    int num,x,y;

    if(!args.IsList(4)
      || !args[0].IsInteger()
      || !args[1].IsInteger() || !args[2].IsInteger()
      || !args[3].IsString())
	ArgumentError("create_cardbox",args);

    x=args[0].Integer();
    y=args[1].Integer();
    num=args[2].Integer();
    nm=args[3].String();

    if(IsObjectNumber(num))
	throw LangErr("create_cardbox","Object number "+ToString(num)+" already in use.");
	
    CardBox* cardbox=new CardBox(num,0,nm);
    if(!cardbox)
	throw Error::Memory("Table::create_cardbox(const Data&)");

    table.push_back(cardbox);
    objects[num]=cardbox;

    cardbox->grp.w=0;
    cardbox->grp.h=0;
    cardbox->grp.x=x;
    cardbox->grp.y=y;
    cardbox->grp.bg=INVISIBLE;
    cardbox->grp.draggable=true;
    cardbox->grp.visible=true;
    cardbox->grp.sticky=true;
    cardbox->grp.ontop=true;
    cardbox->grp.clickable=true;
    cardbox->grp.highlight=false;
    cardbox->grp.compact=false;
    cardbox->grp.tiny=false;
    cardbox->RecalculateSize();
    Refresh(cardbox);

    return num;
}

/// put_cardbox(b,e) - Put a card $e$ or list of cards a $e$ to the card
/// box $b$. Throw an exception if $e$ is not an imagenumber or contains
/// invalid image number. Otherwise return $b$.
Data Table::put_cardbox(const Data& args)
{
    int cbx,image;

    if(!args.IsList(2) || !args[0].IsInteger() || (!args[1].IsInteger() && !args[1].IsList()))
	ArgumentError("put_cardbox",args);

    cbx=args[0].Integer();
    OBJECTVAR(cardbox,CardBox,cbx);
    Widget oldpos=cardbox->grp;

    if(args[1].IsInteger())
    {
	image=args[1].Integer();
	if(image < 0)
	    throw LangErr("put_cardbox","invalid image number");
	else if(image >= cards.Cards())
	{
	    Warning("put_cardbox: invalid image number "+ToString(image));
	    image=0;
	}

	cardbox->Add(image);
    }
    else
	for(size_t i=0; i<args[1].Size(); i++)
	{
	    if(!args[1][i].IsInteger())
		ArgumentError("put_cardbox",args);
				
	    image=args[1][i].Integer();
	    if(image < 0)
		throw LangErr("put_cardbox","invalid image number");
	    else if(image >= cards.Cards())
	    {
		Warning("put_cardbox: invalid image number "+ToString(image));
		image=0;
	    }

	    cardbox->Add(image);
	} 
	
    cardbox->RecalculateSize();
    Refresh(oldpos,cardbox);
	
    return cbx;
}

/// put_cardbox_recenter(b,e) - Same as the {\tt put_cardbox_recenter}, but move the card
/// box after addition in a such way, that the center point of the box
/// remains in place.
Data Table::put_cardbox_recenter(const Data& args)
{
    int image,cbx,dx,dy,w,h;

    if(!args.IsList(2) || !args[0].IsInteger() || (!args[1].IsInteger() && !args[1].IsList()))
	ArgumentError("put_cardbox_recenter",args);

    cbx=args[0].Integer();
    OBJECTVAR(cardbox,CardBox,cbx);
    Widget oldpos=cardbox->grp;

    w=cardbox->grp.w;
    h=cardbox->grp.h;

    if(args[1].IsInteger())
    {
	image=args[1].Integer();
	if(image < 0)
	    throw LangErr("put_cardbox_recenter","invalid image number");
	else if(image >= cards.Cards())
	{
	    Warning("put_cardbox_recenter: invalid image number "+ToString(image));
	    image=0;
	}

	cardbox->Add(image);
    }
    else
	for(size_t i=0; i<args[1].Size(); i++)
	{
	    if(!args[1][i].IsInteger())
		ArgumentError("put_cardbox_recenter",args);
				
	    image=args[1][i].Integer();
	    if(image < 0)
		throw LangErr("put_cardbox_recenter","invalid image number");
	    else if(image >= cards.Cards())
	    {
		Warning("put_cardbox_recenter: invalid image number "+ToString(image));
		image=0;
	    }
			
	    cardbox->Add(image);
	} 

    cardbox->RecalculateSize();
    dx=(w - cardbox->grp.w)/2;
    dy=(h - cardbox->grp.h)/2;
    Move(cardbox,dx,dy);
    Refresh(oldpos,cardbox);
	
    return cbx;
}

/// replace_cardbox_recenter(b,e) - Replace the content of the card
/// box $b$ by card or cards $e$. Keep the center of the box in place.
Data Table::replace_cardbox_recenter(const Data& args)
{
    int image,cbx,dx,dy,w,h;

    if(!args.IsList(2) || !args[0].IsInteger() || (!args[1].IsInteger() && !args[1].IsList()))
	ArgumentError("replace_cardbox_recenter",args);

    cbx=args[0].Integer();
    OBJECTVAR(cardbox,CardBox,cbx);

    Widget oldpos=cardbox->grp;
		
    w=cardbox->grp.w;
    h=cardbox->grp.h;
	
    while(cardbox->Cards())
	cardbox->Del(0);
	
    if(args[1].IsInteger())
    {
	image=args[1].Integer();
	if(image < 0)
	    throw LangErr("replace_cardbox_recenter","invalid image number");
	else if(image >= cards.Cards())
	{
	    Warning("put_cardbox_recenter: invalid image number "+ToString(image));
	    image=0;
	}

	cardbox->Add(image);
    }
    else
	for(size_t i=0; i<args[1].Size(); i++)
	{
	    if(!args[1][i].IsInteger())
		ArgumentError("replace_cardbox_recenter",args);
				
	    image=args[1][i].Integer();
	    if(image < 0)
		throw LangErr("replace_cardbox_recenter","invalid image number");
	    else if(image >= cards.Cards())
	    {
		Warning("put_cardbox_recenter: invalid image number "+ToString(image));
		image=0;
	    }
			    
	    cardbox->Add(image);
	} 

    cardbox->RecalculateSize();
    dx=(w - cardbox->grp.w)/2;
    dy=(h - cardbox->grp.h)/2;
    Move(cardbox,dx,dy);
    Refresh(oldpos,cardbox);
	
    return cbx;
}

/// replace_cardbox(b,e) - Replace the content of the card
/// box $b$ by card or cards $e$. Don't move the box.
Data Table::replace_cardbox(const Data& args)
{
    int image,cbx;

    if(!args.IsList(2) || !args[0].IsInteger() || (!args[1].IsInteger() && !args[1].IsList()))
	ArgumentError("replace_cardbox",args);

    cbx=args[0].Integer();
    OBJECTVAR(cardbox,CardBox,cbx);

    Widget oldpos=cardbox->grp;
			
    while(cardbox->Cards())
	cardbox->Del(0);
	
    if(args[1].IsInteger())
    {
	image=args[1].Integer();
	if(image < 0)
	    throw LangErr("replace_cardbox","invalid image number");
	else if(image >= cards.Cards())
	{
	    Warning("replace_cardbox: invalid image number "+ToString(image));
	    image=0;
	}

	cardbox->Add(image);
    }
    else
	for(size_t i=0; i<args[1].Size(); i++)
	{
	    if(!args[1][i].IsInteger())
		ArgumentError("replace_cardbox",args);
				
	    image=args[1][i].Integer();
	    if(image < 0)
		throw LangErr("replace_cardbox","invalid image number");
	    else if(image >= cards.Cards())
	    {
		Warning("replace_cardbox: invalid image number "+ToString(image));
		image=0;
	    }
			    
	    cardbox->Add(image);
	} 

    cardbox->RecalculateSize();
    Refresh(oldpos,cardbox);
	
    return cbx;
}

/// del_cardbox(b,i) - Remove a card with index $i$ from the card box
/// $b$. Return {\tt NULL} if $i$ is not a valid index. Otherwise
/// return the card number of the removed card.
Data Table::del_cardbox(const Data& args)
{
    int cbx,index;
	
    Get2Integers("del_cardbox",args,cbx,index);

    OBJECTVAR(cardbox,CardBox,cbx);
    Widget oldpos=cardbox->grp;

    if(index < 0 || index >= int(cardbox->Cards()))
	return Null;

    int ret=(*cardbox)[index];
    cardbox->Del(index);

    cardbox->RecalculateSize();
    Refresh(oldpos,cardbox);
	
    return ret;
}

/// del_cardbox_recenter(b,i) - Same as the {\tt del_cardbox}, but move the box
/// keeping the center of the box in place.
Data Table::del_cardbox_recenter(const Data& args)
{
    int cbx,index;

    Get2Integers("del_cardbox_recenter",args,cbx,index);

    OBJECTVAR(cardbox,CardBox,cbx);
    Widget oldpos=cardbox->grp;

    if(index < 0 || index >= int(cardbox->Cards()))
	return Null;

    //	oldx=cardbox->grp.x;
    //	oldy=cardbox->grp.y;
    int w = cardbox->grp.w;
    int h = cardbox->grp.h;

    int ret=(*cardbox)[index];
    cardbox->Del(index);
    cardbox->RecalculateSize();

    int dx = (w - cardbox->grp.w)/2;
    int dy = (h - cardbox->grp.h)/2;
    Move(cardbox,dx,dy);
    Refresh(oldpos,cardbox);

    return ret;
}

/// del_cardbox_all_recenter(b) - Remove all cards. Return the number
/// of cards removed.
Data Table::del_cardbox_all_recenter(const Data& args)
{
    int cbx;
	
    cbx=GetInteger("del_cardbox_all_recenter",args);

    OBJECTVAR(cardbox,CardBox,cbx);
    Widget oldpos=cardbox->grp;

    //	int oldx = cardbox->grp.x;
    //	int oldy = cardbox->grp.y;
    int w = cardbox->grp.w;
    int h = cardbox->grp.h;

    int n=0;

    while(cardbox->Cards())
    {
	n++;
	cardbox->Del(0);
    }

    cardbox->RecalculateSize();

    int dx = (w - cardbox->grp.w)/2;
    int dy = (h - cardbox->grp.h)/2;
    Move(cardbox,dx,dy);
    Refresh(oldpos,cardbox);

    return n;
}

/// create_deck(x,y,n,s) - Create a deck object to $(x,y)$. Object
/// number is $n$ and name is $s$. Return $n$.
Data Table::create_deck(const Data& args)
{
    string nm;
    int num,x,y;

    if(!args.IsList(4) || !args[0].IsInteger()
      || !args[1].IsInteger() || !args[2].IsInteger()
      || !args[3].IsString())
	ArgumentError("create_deck",args);

    x=args[0].Integer();
    y=args[1].Integer();
    num=args[2].Integer();
    nm=args[3].String();

    if(IsObjectNumber(num))
	throw LangErr("create_deck","Object number "+ToString(num)+" already in use.");
	
    Deck* deck=new Deck(num,0,nm);
    if(!deck)
	throw Error::Memory("Table::create_deck(const Data&)");

    table.push_back(deck);
    objects[num]=deck;

    deck->grp.w=0;
    deck->grp.h=0;
    deck->grp.x=x;
    deck->grp.y=y;
    deck->grp.bg=INVISIBLE;
    deck->grp.draggable=true;
    deck->grp.visible=true;
    deck->grp.sticky=true;
    deck->grp.ontop=true;
    deck->grp.clickable=true;
    deck->grp.highlight=false;
    deck->grp.compact=false;
    deck->grp.tiny=false;
    deck->RecalculateSize();
    Table2Screen(deck->grp.x,deck->grp.y);
    Refresh(deck);

    return num;
}

/// rot(\alpha,c) - Rotate a card $c$ by $\alpha$ degrees.
Data Table::rot(const Data& args)
{
    VECTORIZE_SECOND(rot);
	
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("rot",args);
	
    CardInPlay* card=GetCard("rot",args[1]);

    int oldx=card->grp.x,oldy=card->grp.y,oldw=card->grp.w,oldh=card->grp.h;
	
    card->Rot(args[0].Integer());
    card->RecalculateSize();

    Move(card,(oldw - card->grp.w)/2,(oldh - card->grp.h)/2);
	
    int sz=max(oldw,max(oldh,max(card->grp.h,card->grp.w)));
    Refresh(min(card->grp.x,oldx),min(card->grp.y,oldy),sz,sz);

    return card->Number();
}

/// tap(o) - If an object $o$ is not a card in play, return {\tt NULL}.
/// If it as already tapped card, return 0. Otherwise tap the
/// card and return 1.
Data Table::tap(const Data& args)
{
    VECTORIZE_NONNULL(tap);

    CardInPlay* card=GetCard("tap",args);
    if(card->IsTapped())
	return Null;

    int oldx=card->grp.x,oldy=card->grp.y,oldw=card->grp.w,oldh=card->grp.h;
	
    card->Tap();
    card->RecalculateSize();
	
    Move(card,(oldw - card->grp.w)/2,(oldh - card->grp.h)/2);

    int sz=max(oldw,max(oldh,max(card->grp.h,card->grp.w)));
    Refresh(min(card->grp.x,oldx),min(card->grp.y,oldy),sz,sz);

    return card->Number();
}

/// tap_left(o) - If an object $o$ is not a card in play, return {\tt NULL}.
/// If it as already left tapped card, return 0. Otherwise tap left the
/// card and return 1.
Data Table::tap_left(const Data& args)
{
    VECTORIZE_NONNULL(tap_left);

    CardInPlay* card=GetCard("tap_left",args);
    if(card->IsLeftTapped())
	return Null;

    int oldx=card->grp.x,oldy=card->grp.y,oldw=card->grp.w,oldh=card->grp.h;
	
    card->TapLeft();
    card->RecalculateSize();
	
    Move(card,(oldw - card->grp.w)/2,(oldh - card->grp.h)/2);

    int sz=max(oldw,max(oldh,max(card->grp.h,card->grp.w)));
    Refresh(min(card->grp.x,oldx),min(card->grp.y,oldy),sz,sz);
	
    return card->Number();
}

/// invert(o) - If an object $o$ is not a card in play, return {\tt NULL}.
/// If it as already inverted card, return 0. Otherwise invert the
/// card and return 1.
Data Table::invert(const Data& args)
{
    VECTORIZE_NONNULL(invert);

    CardInPlay* card=GetCard("inver",args);

    if(card->IsUpsideDown())
	return Null;

    int oldx=card->grp.x,oldy=card->grp.y,oldw=card->grp.w,oldh=card->grp.h;

    card->Invert();
    card->RecalculateSize();

    Move(card,(oldw - card->grp.w)/2,(oldh - card->grp.h)/2);

    int sz=max(oldw,max(oldh,max(card->grp.h,card->grp.w)));
    Refresh(min(card->grp.x,oldx),min(card->grp.y,oldy),sz,sz);
	
    return card->Number();
}

/// untap(o) - If an object $o$ is not a card in play, return {\tt NULL}.
/// If it as already untapped card, return 0. Otherwise untap
/// the card and return 1.
Data Table::untap(const Data& args)
{
    VECTORIZE_NONNULL(untap);

    CardInPlay* card=GetCard("untap",args);
    if(card->IsUntapped())
	return Null;

    int oldx=card->grp.x,oldy=card->grp.y,oldw=card->grp.w,oldh=card->grp.h;
	
    card->Untap();
    card->RecalculateSize();
		
    Move(card,(oldw - card->grp.w)/2,(oldh - card->grp.h)/2);

    int sz=max(oldw,max(oldh,max(card->grp.h,card->grp.w)));
    Refresh(min(card->grp.x,oldx),min(card->grp.y,oldy),sz,sz);

    return card->Number();
}

/// tapped(o) - If $o$ is not a card in play, return {\tt NULL}.
/// Otherwise return 1 or 0 depending on tapped status.
Data Table::tapped(const Data& args)
{
    VECTORIZE(tapped);

    CardInPlay* card=GetCard("tapped",args);
    return (int)card->IsTapped();
}

/// tapped_left(o) - If $o$ is not a card in play, return {\tt NULL}.
/// Otherwise return 1 or 0 depending if card is tapped left or
/// not.
Data Table::tapped_left(const Data& args)
{
    VECTORIZE(tapped_left);

    CardInPlay* card=GetCard("tapped_left",args);
    return (int)(card->IsLeftTapped());
}

/// untapped(o) - If $o$ is not a card in play, return {\tt NULL}.
/// Otherwise return 1 or 0 depending on tapped status.
Data Table::untapped(const Data& args)
{
    VECTORIZE(untapped);

    CardInPlay* card=GetCard("untapped",args);

    return (int)(card->IsUntapped());
}

/// dump() - Print status of the all display elements on stdout.
Data Table::dump(const Data& args)
{
    Dump();

    return Null;
}

/// raise(o) - Raise an object $o$ and all related objects (child and
/// parent objects recursively) above other objects. Return the number
/// of raised objects.
Data Table::raise(const Data& args)
{
    Object* obj=GetObject("raise",args);

    bool oldref=refresh;
    refresh=false;
	
    list<Object*> all_objects;

    GetGroup(obj->Root(),all_objects);
	
    Raise(all_objects);

    refresh=oldref;
    Refresh();
	
    return int(all_objects.size());
}

/// lower(o) - Lower an object $o$ below other objects.
Data Table::lower(const Data& args)
{
    Object* obj=GetObject("raise",args);

    Lower(obj);

    return obj->Number();
}

/// mouse(m) - If $m$ is 1, show mouse pointer, otherwise hide it.
Data Table::mouse(const Data& args)
{
    int show=GetInteger("mouse",args);
    if(show)
	driver->ShowMouse();
    else
	driver->HideMouse();

    return (int)(show != 0);
}

/// message(o,m) - Print message to the message box $o$. Returns $o$ if
/// successfull, {\tt null} otherwise.
Data Table::message(const Data& args)
{
    if(!args.IsList() || args.Size() != 2)
	ArgumentError("message",args);

    Object* obj=GetObject("message",args[0]);
    string str=GetString("message",args[1]);

    if(obj->Type() != TypeMessageBox)
	return Null;
	
    MessageBox* box=dynamic_cast<MessageBox*>(obj);
    box->WriteMessage(str);
    Refresh(box);

    return box->Number();
}

/// msgbox_scroll(b,n) - Scroll the content of the message box $b$ by $n$
/// lines upward if $n\ge 1$ or $-n$ lines downward if $n \le -1$.
Data Table::msgbox_scroll(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("msgbox_scroll",args);
	
    OBJECTVAR(box,MessageBox,args[0].Integer());

    int n0,n;
	
    n0=n=args[1].Integer();
    if(n < 0)
	while(n < 0)
	{
	    box->ScrollUp();
	    n++;
	}
    else
	while(n > 0)
	{
	    box->ScrollDown();
	    n--;
	}

    if(n0)
	Refresh(box);
	
    return box->Offset();
}

/// msgbox_search(o,t) - Search the message box $o$ for lines containing the text $t$.
/// Returns list of matching lines. Returns {\tt NULL} if $o$ is not a message box.
Data Table::msgbox_search(const Data& args)
{
    if(!args.IsList() || args.Size() != 2 || !args[0].IsInteger())
	ArgumentError("msgbox_search",args);

    Object* obj=GetObject("msgbox_search",args[0]);
    string str=GetString("msgbox_search",args[1]);

    if(obj->Type() != TypeMessageBox)
	return Null;
	
    MessageBox* box=dynamic_cast<MessageBox*>(obj);
    
    Data ret;
    ret.MakeList();
    
    deque<string> text = box->Messages();
    deque<string>::iterator i;
    
    for(i = text.begin(); i < text.end(); i++)
      if ((*i).find(str)!=string::npos)
        ret.AddList(*i);

    return ret;
}

/// msgbox_tail(o,l) - Returns the last $l$ lines of the message box $o$.
/// If $o$ has fewer than $l$ lines, returns entire history of $o$.
/// Returns {\tt NULL} if $o$ is not a message box or if $l$ is zero or negative.
Data Table::msgbox_tail(const Data& args)
{
    if(!args.IsList() || args.Size() != 2 || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("msgbox_tail",args);

    Object* obj=GetObject("msgbox_tail",args[0]);
    int n=args[1].Integer();

    if(obj->Type() != TypeMessageBox || n <= 0)
	return Null;
	
    MessageBox* box=dynamic_cast<MessageBox*>(obj);
    
    Data ret;
    ret.MakeList();
    
    deque<string> text = box->Messages();
    deque<string>::iterator i;
    
    if(n >= (int)text.size())
      i = text.begin();
    else
      i = text.end() - n;
    
    while(i != text.end())
        ret.AddList(*i++);

    return ret;
}

/// get_attr(o,a) - Get the current value of the widget flag $a$ of the
/// object $o$ (See Class Widget). 
Data Table::get_attr(const Data& args)
{
    if(!args.IsList(2))
	ArgumentError("get_attr",args);

    Object* obj=GetObject("get_attr",args[0]);
    string attr=GetString("get_attr",args[1]);

    if(attr=="visible")
	return obj->grp.visible;
    else if(attr=="draggable")
	return obj->grp.draggable;
    else if(attr=="dragparent")
	return obj->grp.dragparent;
    else if(attr=="sticky")
	return obj->grp.sticky;
    else if(attr=="ontop")
	return obj->grp.ontop;
    else if(attr=="onbottom")
	return obj->grp.onbottom;
    else if(attr=="clickable")
	return obj->grp.clickable;
    else if(attr=="highlight")
	return obj->grp.highlight;
    else if(attr=="compact")
	return obj->grp.compact;
    else if(attr=="tiny")
	return obj->grp.tiny;
    else if(attr=="fit")
	return obj->grp.fit;
	
    return Null;
}

/// set_attr(o,a,b) - Set the value of the widget flag $a$ of the
/// object $o$ to $b$ (See Class Widget).
Data Table::set_attr(const Data& args)
{
    if(!args.IsList() || args.Size() != 3)
	ArgumentError("set_attr",args);

    Object* obj=GetObject("set_attr",args[0]);
    string attr=GetString("set_attr",args[1]);
    int value=GetInteger("set_attr",args[2]);
    int old;

    if(attr=="visible")
    {
	old=obj->grp.visible;
	obj->grp.visible=value;
    }
    else if(attr=="draggable")
    {
	old=obj->grp.draggable;
	obj->grp.draggable=value;
    }
    else if(attr=="dragparent")
    {
	old=obj->grp.dragparent;
	obj->grp.dragparent=value;
	obj->grp.draggable=0;
    }
    else if(attr=="sticky")
    {
	old=obj->grp.sticky;
	obj->grp.sticky=value;
    }
    else if(attr=="ontop")
    {
	old=obj->grp.ontop;
	obj->grp.ontop=value;
	obj->grp.onbottom=!value;
    }
    else if(attr=="onbottom")
    {
	old=obj->grp.onbottom;
	obj->grp.onbottom=value;
	obj->grp.ontop=!value;
    }
    else if(attr=="clickable")
    {
	old=obj->grp.clickable;
	obj->grp.clickable=value;
    }
    else if(attr=="highlight")
    {
	old=obj->grp.highlight;
	obj->grp.highlight=value;
    }
    else if(attr=="compact")
    {
	old=obj->grp.compact;
	obj->grp.tiny=0;
	obj->grp.compact=value;
    }
    else if(attr=="tiny")
    {
	old=obj->grp.tiny;
	obj->grp.compact=0;
	obj->grp.tiny=value;
    }
    else if(attr=="fit")
    {
	old=obj->grp.fit;
	obj->grp.fit=value;
    }
    else
	return Null;

    int w=obj->grp.w,h=obj->grp.h;
    obj->RecalculateSize();
    if(obj->grp.w > w)
	w=obj->grp.w;
    if(obj->grp.h > h)
	h=obj->grp.h;
	
    obj->grp.redraw=true;
	
    Refresh(obj->grp.x,obj->grp.y,w,h);
	
    return old;
}

/// card(o) - Return the card number of the object $o$ or {\tt NULL}
/// if $o$ is not a card in play.
Data Table::card(const Data& args)
{
    VECTORIZE(card);

    int crd=GetInteger("cards",args);
		
    CardInPlay* c=dynamic_cast<CardInPlay*>(objects[crd]);
    if(c)
	return c->Number();
    else
	return Null;
}

/// card_data(o) - Return a vector ($o,a,c,r$), where $o$ is owner of
/// the card, $a$ is current rotation angle, $c$ is the card number of
/// the visible side and $r$ is real identity of the card number if
/// known. Return {\tt NULL} if $o$ is not a card in play.
Data Table::card_data(const Data& args)
{
    VECTORIZE(card);

    int crd=GetInteger("cards",args);
		
    CardInPlay* c=dynamic_cast<CardInPlay*>(objects[crd]);
    if(c)
    {
	Data ret;
	ret.MakeList(4);
	ret[0]=c->Owner();
	ret[1]=c->Angle();
	ret[2]=c->Number();
	ret[3]=c->RealCard();
	return ret;
    }
    else
	return Null;
}

/// object_name(n) - Name of the object number $n$.
Data Table::object_name(const Data& args)
{
    VECTORIZE(object_name);

    int obj=GetInteger("object_name",args);

    if(IsObjectNumber(obj))
	return objects[obj]->Name();
    else
	return Null;
}

/// object_type(n) - Type of the object number $n$ as a string
/// "image", "counter", "marker", "cardinplay", "hand", "deck",
/// "messagebox", "cardbox", "cardbook", "menu", or "listbox".
Data Table::object_type(const Data& args)
{
    VECTORIZE(object_type);

    int obj=GetInteger("object_type",args);

    if(!IsObjectNumber(obj))
	return Null;

    Object* o=objects[obj];
    ObjectType t=o->Type();
	
    if(t==TypeImage)
	return string("image");
    if(t==TypeCounter)
	return string("counter");
    if(t==TypeMarker)
	return string("marker");
    if(t==TypeCardInPlay)
	return string("cardinplay");
    if(t==TypeHand)
	return string("hand");
    if(t==TypeDeck)
	return string("deck");
    if(t==TypeMessageBox)
	return string("messagebox");
    if(t==TypeCardBox)
	return string("cardbox");
    if(t==TypeCardBook)
	return string("cardbook");
    if(t==TypeMenu)
	return string("menu");
    if(t==TypeListBox)
	return string("listbox");

    return Null;
}

/// object_data(n) - Return a data vector $(x,y,w,h,p,G,M)$ for the object
/// number $n$, where $(x,y)$ is coordinates on the table, $w \times h$ is
/// the size of the object, $p$ is the object number of the parent object, $G$ is the
/// list of all attached object numbers and $M$ is list of all pairs
/// ($c$,$n$) of markers  at the object, where $n$ is number of
/// markers and $c$ is number of color.
Data Table::object_data(const Data& args)
{
    VECTORIZE(object_data);

    int obj=GetInteger("object_data",args);

    Data ret;
    ret.MakeList(7);
	
    if(IsObjectNumber(obj))
    {
	Object* o=objects[obj];
	list<Object*> att;
	list<Object*>::iterator i;
		
	ret[0]=o->grp.x;
	ret[1]=o->grp.y;
	ret[2]=o->grp.w;
	ret[3]=o->grp.h;
	if(o->Parent())
	    ret[4]=o->Parent()->Number();
	else
	    ret[4]=0;
	att=o->Group();
	ret[5].MakeList(att.size());
	int j=0;
	for(i=att.begin(); i!=att.end(); i++)
	    ret[5][j++]=(*i)->Number();

	ret[6].MakeList(marker_image.size());
	for(size_t i=0; i<marker_image.size(); i++)
	{
	    ret[6][i]=Data(int(i),o->Markers(i));
	}
	return ret;
    }
    else
	return Null;
}

/// center_of(n,[table]) - Return a vector $(x,y)$ containing the center of
/// the object or {\tt NULL} if $n$ is not an object. If optional
/// argument $table$ is given, convert coordinates to the table coordinates.
Data Table::center_of(const Data& args)
{
    bool to_table=false;
    int obj=0;
		
    if(args.IsList(2) && args[0].IsInteger() && args[1].IsInteger())
    {
	obj=args[0].Integer();
	to_table=(args[1].Integer() != 0);
    }
    else if(args.IsInteger())
    {
	obj=args.Integer();
    }
    else
	ArgumentError("center_of",args);
	
    if(IsObjectNumber(obj))
    {
	Data ret;
	ret.MakeList(2);
		
	Object* o=objects[obj];

	int x,y;
 
	x=o->grp.x + (o->grp.w/2);
	y=o->grp.y + (o->grp.h/2);
		
	if(to_table)
	    Screen2Table(x,y);
 
	ret[0]=x;
	ret[1]=y;
		
	return ret;
    }
    else
	return Null;
}

/// inplay([p]) - Return the list of objects numbers in play owned by
/// player number $p$ or all objects if $p$ is not given. The topmost
/// object is last.
Data Table::inplay(const Data& args)
{
    if(!args.IsNull() && !args.IsInteger())
	ArgumentError("inplay",args);

    int owner=-9999;
    if(args.IsInteger())
	owner=args.Integer();

    Data ret;
	
    ret.MakeList();
    list<Object*>::const_iterator i;
    for(i=table.begin(); i!=table.end(); i++)
	if((*i)->Type()==TypeCardInPlay)
	{
	    CardInPlay* c=dynamic_cast<CardInPlay*>(*i);

	    if(owner == -9999 || c->Owner()==owner)
		ret.AddList((*i)->Number());
	}

    return ret;
}

/// del_object(n) - Remove an object $n$ from the screen if it exist
/// and return 1. Otherwise NULL.
Data Table::del_object(const Data& args)
{
    VECTORIZE(del_object);

    int n=GetInteger("del_object",args);

    if(IsObjectNumber(n))
    {
	Delete(objects[n]);
	return 1;
    }

    return Null;
}

/// trigger(s_1,s_2) - Call a trigger named \{\tt "$s_1$"\}
/// \{\tt "$s_2$"\}. Return {\tt NULL} if the trigger does not
/// exists. Otherwise, return 1 and set a variable {\tt RET} to the last
/// value of evaluated expression in the trigger.
Data Table::trigger(const Data& args)
{
    string s1,s2;
    Data ret;

    Get2Strings("trigger",args,s1,s2);

    if(TryTrigger(s1,s2,ret))
    {
	parser.SetVariable("RET",ret);
	return 1;
    }

    return Null;
}

/// cardbox(n) - Return the content of the card box $n$.
Data Table::cardbox(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("cardbox",args);
	
    OBJECTVAR(cardbox,CardBox,args.Integer());
	
    Data ret;

    ret.MakeList((int)cardbox->Cards());
    for(size_t i=0; i<cardbox->Cards(); i++)
	ret[i]=Data((*cardbox)[i]);

    return ret;
}

/// del_deck(o,i) - Remove the $i$th card counting from the top of the
/// deck $o$.
Data Table::del_deck(const Data& args)
{
    int dck,index;
	
    Get2Integers("del_deck",args,dck,index);

    OBJECTVAR(deck,Deck,dck);

    if(index < 0 || index >= int(deck->Size()))
	return Null;

    int w=deck->grp.w,h=deck->grp.h;
    int ret=(*deck)[index];
    deck->Del(index);

    deck->RecalculateSize();
    Refresh(deck->grp.x,deck->grp.y,w,h);
	
    return ret;
}

/// add_marker(o,n) - Add a marker number $n$ to the object $o$. Throw
/// an exception if $n$ is not valid marker number.
Data Table::add_marker(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("add_marker",args);

    Object* o=GetObject("add_marker",args[0]);
    int type=args[1].Integer();
    if(type < 0 || type >= int(marker_image.size()))
	ArgumentError("add_marker",args);

    o->AddMarker(type);
    Refresh(o->grp.x,o->grp.y,o->grp.w,o->grp.h);
	
    return o->Markers(type);
}

/// del_marker(o,n) - Delete a marker number $n$ from the object $o$. Throw
/// an exception if $n$ is not valid marker number.
Data Table::del_marker(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("del_marker",args);

    Object* o=GetObject("del_marker",args[0]);
    int type=args[1].Integer();
    if(type < 0 || type >= int(marker_image.size()))
	ArgumentError("del_marker",args);

    o->DelMarker(type);
    Refresh(o->grp.x,o->grp.y,o->grp.w,o->grp.h);

    return o->Markers(type);
}

/// attach(o_1,o_2) - Attach an object $o_1$ to an object $o_2$. Return
/// {\tt NULL}, if the attachment would cause a loop chain, 1 otherwise.
Data Table::attach(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("attach",args);

    Object* o1=objects[args[0].Integer()];
    Object* o2=objects[args[1].Integer()];

    if(!o1 || !o2)
	throw LangErr("attach","arguments are not both objects");

    o1->Attach(o2);
    if(o1->Root()==0)
    {
	o1->Detach();
	return Null;
    }
	
    return 1;
}

/// detach(o) - Detach an object $o$ if attached to another object.
Data Table::detach(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("detach",args);

    Object* o=objects[args.Integer()];
    if(!o)
	throw LangErr("attach","argument is not an object");
	
    o->Detach();

    return Null;
}

/// change_card(n,c) - Change the image of the card with object number $n$
/// to $c$. Shift previous card number to real-card value.
Data Table::change_card(const Data& args)
{
    if(!args.IsList(2) && !args[0].IsInteger() && !args[1].IsInteger())
	ArgumentError("change_card",args);

    CardInPlay* c=GetCard("change_card",args[0]);
    int n=args[1].Integer();

    if(n < 0)
	throw LangErr("change_card","invalid card number");
    else if(n >= Database::cards.Cards())
    {
	Warning("change_card: invalid image number "+ToString(n));
	n=0;
    }

    Widget old=c->grp;
    c->ChangeCard(n);
    c->RecalculateSize();
    Refresh(old,c);

    return Null;
}

/// load_image(s_1,[s_2]) - Load an image with filename $s_1$ or $s_2$ if
/// $s_1$ not found and return it's image number. If neither one is found,
/// throw an exception.
Data Table::load_image(const Data& args)
{
    string orig,fname;

    if(args.IsString())
    {
        orig=args.String();
        fname=FindFile(orig,"graphics");
    }
    else if(args.IsList(2) && (args[0].IsString() || args[0].IsNull()) && args[1].IsString())
    {
	if(args[0].IsString())
        {
            orig=args[0].String();
            fname=FindFile(orig,"graphics/avatar","graphics");
        }

	if(args[1].IsString() && fname=="")
        {
            orig=args[1].String();
            fname=FindFile(orig,"graphics/avatar","graphics");
        }
    }
    else
	ArgumentError("load_image",args);

    if(fname=="")
	throw LangErr("load_image","image not found "+orig);

    return driver->LoadImage(fname,Color(0,0,0));
}

/// scale_image(i,w,h) - Scale an image $i$ to new size $w \times
/// h$. Return old size (w,h) of the image.
Data Table::scale_image(const Data& args)
{
    if(!args.IsList(3) || !args[0].IsInteger() || !args[1].IsInteger() || !args[2].IsInteger())
	ArgumentError("scale_image",args);

    int img=args[0].Integer();
    int w=args[1].Integer();
    int h=args[2].Integer();

    if(!driver->ValidImage(img))
	throw LangErr("scale_image","invalid image number "+ToString(img));
    if(w < 1)
	throw LangErr("scale_image","invalid image width "+ToString(w));
    if(h < 1)
	throw LangErr("scale_image","invalid image height "+ToString(h));

    int oldw=driver->ImageWidth(img);
    int oldh=driver->ImageHeight(img);
    driver->ScaleImage(img,w,h);

    list<Object*>::iterator i;
    for(i=table.begin(); i!=table.end(); i++)
	if((*i)->Type()==TypeImage)
	    (*i)->RecalculateSize();
	
    Refresh();
	
    return Data(oldw,oldh);
}

/// create_image(x,y,n,i,s,v) - Create an image at $(x,y)$. An object
/// number is $n$, an image number is $i$ and a name is $s$. Argument
/// $v$ is optional and defines the visibility of the image.
Data Table::create_image(const Data& args)
{
    string nm;
    int img,num,x,y;

    if((!args.IsList(5) && !args.IsList(6))
      || !args[0].IsInteger()
      || !args[1].IsInteger()
      || !args[2].IsInteger()
      || !args[3].IsInteger()
      || !args[4].IsString())
	ArgumentError("create_image",args);
    if(args.IsList(6) && !args[5].IsInteger())
	ArgumentError("create_image",args);

    bool visible=args.IsList(5) || (args.IsList(6) && args[5].Integer() != 0);
	
    x=args[0].Integer();
    y=args[1].Integer();
    num=args[2].Integer();
    img=args[3].Integer();
    nm=args[4].String();

    if(!driver->ValidImage(img))
	throw LangErr("create_image","invalid image number "+ToString(img));
	
    if(IsObjectNumber(num))
	throw LangErr("create_image","Object number "+ToString(num)+" already in use.");
	
    Image* image=new Image(img,num,0,nm);
    if(!image)
	throw Error::Memory("Table::create_image(const Data&)");

    table.push_back(image);
    objects[num]=image;

    image->grp.w=0;
    image->grp.h=0;
    image->grp.x=x;
    image->grp.y=y;
    image->grp.bg=INVISIBLE;
    image->grp.draggable=false;
    image->grp.visible=visible;
    image->grp.sticky=true;
    image->grp.ontop=false;
    image->grp.clickable=false;
    image->grp.highlight=false;
    image->grp.compact=false;
    image->grp.tiny=false;
    image->RecalculateSize();
    Refresh(image);

    return num;
}

/// image_width(i) - Return the width of the image $i$.
Data Table::image_width(const Data& args)
{
    if(!args.Integer())
	ArgumentError("image_width",args);
    if(!driver->ValidImage(args.Integer()))
	throw LangErr("image_width","invalid image number "+ToString(args.Integer()));

    return driver->ImageWidth(args.Integer());
}

/// image_height(i) - Return the height of the image $i$.
Data Table::image_height(const Data& args)
{
    if(!args.Integer())
	ArgumentError("image_height",args);
    if(!driver->ValidImage(args.Integer()))
	throw LangErr("image_height","invalid image number "+ToString(args.Integer()));

    return driver->ImageHeight(args.Integer());
}

/// image_pixel(i,x,y) - Return (r,g,b) value of the pixel (x,y) of the image $i$.
Data Table::image_pixel(const Data& args)
{
    if(!args.IsList(3) || !args[0].IsInteger() || !args[1].IsInteger() || !args[2].IsInteger())
	ArgumentError("image_pixel",args);

    int img=args[0].Integer();
    int x=args[1].Integer();
    int y=args[2].Integer();
	
    if(!driver->ValidImage(img))
	throw LangErr("image_pixel","invalid image number "+ToString(img));
    if(x < 0 || x >= driver->ImageWidth(img))
	throw LangErr("image_pixel","invalid x-coordinate "+ToString(x));
    if(y < 0 || y >= driver->ImageHeight(img))
	throw LangErr("image_pixel","invalid y-coordinate "+ToString(y));

    int r,g,b;
    driver->GetRGB(img,x,y,r,g,b);
	
    Data ret;
    ret.MakeList(3);
    ret[0]=r;
    ret[1]=g;
    ret[2]=b;
	
    return ret;
}

/// load_sound(s_1) - Load a sound with filename $s_1$ and return it's image number.
/// If not found, throw an exception.
Data Table::load_sound(const Data& args)
{
    list<string> files;

    if(args.IsString())
    {
        string fname=StripDirs(args.String());

	files.push_back(CCG_DATADIR"/sounds/"+Database::game.Gamedir()+"/"+fname);
	files.push_back(CCG_DATADIR"/sounds/"+fname);
    }
    else
	ArgumentError("load_sound",args);

    list<string>::iterator i;
    for(i=files.begin(); i!=files.end(); i++)
    {
	ifstream F(i->c_str());
	if(F)
	    break;
    }
    if(i==files.end())
        return Data();

    int ret=driver->LoadSound(*i);
    if(ret < 0)
        return Null;

    return ret;
}

/// play_sound(i) - Play the sound number $i$.
Data Table::play_sound(const Data& args)
{
    if(!args.IsInteger() && !args.IsNull())
		ArgumentError("play_sound",args);

    if(args.IsInteger())
        driver->PlaySound(args.Integer());

	return Null;
}

/// card_sound(i) - returns the sound handle associated to card number $i$.
Data Table::card_sound(const Data& args)
{
    if(!args.IsInteger() && !args.IsNull())
		ArgumentError("card_sound",args);

    if(!args.IsInteger())
		return Null;

	int ret=driver->LoadCardSound(args.Integer());
	if(ret < 0)
		return Null;

	return ret;
}

/// add_text(o,t) - Add a text $t$ to the object $o$.
Data Table::add_text(const Data& args)
{
    if(!args.IsList(2) || !args[1].IsString())
	ArgumentError("add_text",args);

    Object* o=GetObject("add_text",args[0]);
    o->AddText(args[1].String());
    Refresh(o);
	
    return Null;
}

/// del_text(o,t) - Delete a text $t$ from the object $o$.
Data Table::del_text(const Data& args)
{
    if(!args.IsList(2) || !args[1].IsString())
	ArgumentError("del_text",args);

    Object* o=GetObject("del_text",args[0]);
    o->DelText(args[1].String());
    Refresh(o);
	
    return Null;
}

/// del_all_texts(o) - Delete all texts from the object $o$. Return number of texts removed.
Data Table::del_all_texts(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("del_all_texts",args);

    Object* o=GetObject("del_all_texts",args.Integer());
    int count=0;
	
    while(o->Texts())
    {
	o->DelText(o->Text(0));
	count++;
    }
	
    Refresh(o);
	
    return count;
}

/// has_text(o,t) - Return 1 if an object $o$ has text $t$ on it.
Data Table::has_text(const Data& args)
{
    if(!args.IsList(2) || !args[1].IsString())
	ArgumentError("has_text",args);

    Object* o=GetObject("has_text",args[0]);
    string s=args[1].String();
	
    for(int i=0; i<o->Texts(); i++)
	if(o->Text(i)==s)
	    return 1;
	
    return 0;
}

/// get_texts(o) - Return a list of all texts added to object.
Data Table::get_texts(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("get_texts",args);

    Object* o=GetObject("get_texts",args.Integer());

    Data ret;
    ret.MakeList();
	
    for(int i=0; i<o->Texts(); i++)
	ret.AddList(o->Text(i));
	
    return ret;
}

/// objects() - Return all object numbers of the objects on the screen.
Data Table::_objects(const Data& args)
{
    if(!args.IsNull())
	ArgumentError("objects",args);

    list<Object*>::const_iterator i;
    int j=0;
    Data ret;

    ret.MakeList(table.size());
    for(i=table.begin(); i!=table.end(); i++)
	ret[j++]=(*i)->Number();

    return ret;
}

Data ColorToData(const Color& c)
{
    if(c.invisible)
	return Null;

    Data ret;
    if(c.a<100)
        ret.MakeList(4);
    else
        ret.MakeList(3);

    ret[0]=c.r;
    ret[1]=c.g;
    ret[2]=c.b;

    if(c.a<100)
        ret[3]=c.a;

    return ret;
}

Color DataToColor(const Data& arg)
{
    if(arg.IsNull())
	return Color();
    if(arg.IsList(3) && arg[0].IsInteger()
      && arg[1].IsInteger() && arg[2].IsInteger())
    {
	int r=arg[0].Integer();
        int g=arg[1].Integer();
        int b=arg[2].Integer();

        r=min(255,max(0,r));
        g=min(255,max(0,g));
        b=min(255,max(0,b));

	return Color(r,g,b);
    }
    if(arg.IsList(4) && arg[0].IsInteger()
      && arg[1].IsInteger() && arg[2].IsInteger() && arg[3].IsInteger())
    {
	int r=arg[0].Integer();
        int g=arg[1].Integer();
        int b=arg[2].Integer();
        int a=arg[3].Integer();

        r=min(255,max(0,r));
        g=min(255,max(0,g));
        b=min(255,max(0,b));
        a=min(100,max(0,a));

	return Color(r,g,b,a);
    }

    throw LangErr("DataToColor(const Data&)","invalid color "+tostr(arg).String());
}
	
/// set_fgcolor(o,c) - Set the foreground color of the object $o$. Color
/// $c$ should be {\tt NULL} if set to invisible or a list ($r,g,b$) giving
/// RGB-values between 0 and 255. The previous value is returned.
Data Table::set_fgcolor(const Data& args)
{
    if(!args.IsList(2))
	ArgumentError("set_fgcolor",args);

    Object* o=GetObject("set_fgcolor",args[0]);
    Data old=ColorToData(o->grp.fg);
    o->grp.fg=DataToColor(args[1]);
    o->grp.redraw=true;
    Refresh(o);
	
    return old;
}

/// set_bgcolor(o,c) - Set the background color for object $o$ (see {\tt set\_fgcolor}).
Data Table::set_bgcolor(const Data& args)
{
    if(!args.IsList(2))
	ArgumentError("set_bgcolor",args);

    Object* o=GetObject("set_bgcolor",args[0]);
    Data old=ColorToData(o->grp.bg);
    o->grp.bg=DataToColor(args[1]);
    o->grp.redraw=true;
    Refresh(o);
	
    return old;
}

/// set_textcolor(o,c) - Set the text color for object $o$ (see {\tt set\_fgcolor}).
Data Table::set_textcolor(const Data& args)
{
    if(!args.IsList(2))
	ArgumentError("set_textcolor",args);

    Object* o=GetObject("set_textcolor",args[0]);
    Data old=ColorToData(o->grp.text.color);
    o->grp.text.color=DataToColor(args[1]);
    o->grp.redraw=true;
    Refresh(o);
	
    return old;
}

/// set_textfont(o,f) - Set the font for object $o$.
Data Table::set_textfont(const Data& args)
{
    if(!args.IsList(2) || !args[1].IsInteger())
	ArgumentError("set_font",args);

    Object* o=GetObject("set_font",args[0]);
    int f=args[1].Integer();
    if(f < 0 || f > last_font)
	throw LangErr("set_textfont","Invalid font number "+ToString(f));
	
    Data old=o->grp.text.font;
    o->grp.text.font=f;
    o->grp.redraw=true;
    Refresh(o);
	
    return old;
}

/// set_textsize(o,s) - Set the point size of texts for the object $o$. Old
/// value is returned.
Data Table::set_textsize(const Data& args)
{
    if(!args.IsList(2) || !args[1].IsInteger())
	ArgumentError("set_textsize",args);

    Object* o=GetObject("set_textsize",args[0]);
    int old=o->grp.text.pointsize;
    o->grp.text.pointsize=args[1].Integer();
    o->grp.redraw=true;
    Refresh(o);
	
    return old;
}

/// set_textalign(o,a) - Set the text alignment $a$ (0 = left, 1 =
/// center, 2 = right) for the object $o$. Old value is returned.
Data Table::set_textalign(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("set_textalign",args);

    int align=args[1].Integer();
    if(align < 0 || align > 2)
	throw LangErr("set_textalign","invalid alignment "+tostr(args[1]).String());

    Object* o=GetObject("set_textalign",args[0]);
    Data old=o->grp.text.align;
    o->grp.text.align=align;
    o->grp.redraw=true;
    Refresh(o);
	
    return old;
}

/// set_textvalign(o,a) - Set the vertical text alignment $a$ (0 = top,
/// 1 = middle, 2 = bottom) for the object $o$. Old value is returned.
Data Table::set_textvalign(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("set_textvalign",args);

    int align=args[1].Integer();
    if(align < 0 || align > 2)
	throw LangErr("set_textvalign","invalid alignment "+tostr(args[1]).String());

    Object* o=GetObject("set_textvalign",args[0]);
    Data old=o->grp.text.valign;
    o->grp.text.valign=align;
    o->grp.redraw=true;
    Refresh(o);
	
    return old;
}

/// set_textmargin(o,m) - Set the text margin to $m$ pixels between
/// the object border and the text of the object $o$.
/// Old value is returned.
Data Table::set_textmargin(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("set_textmargin",args);

    int margin=args[1].Integer();

    Object* o=GetObject("set_textmargin",args[0]);
    Data old=o->grp.text.margin;
    o->grp.text.margin=margin;
    o->grp.redraw=true;
    Refresh(o);
	
    return old;
}

/// create_book(x,y,n,s,c,r[,bc]) - Create a card book named $s$ with
/// object number $n$ at $(x,y)$. Size of the card book is $c$ columns
/// and $r$ rows. Note that the card book is not visible by default. Sets
/// also variable {\tt book.last\_page} to denote the last page of the
/// book. If argument $bc$ is present, each space in the card book is set
/// to the image size of card number $bc$; otherwise use card number 0.
Data Table::create_book(const Data& args)
{
    string nm;
    int num,x,y,w,h,bc;

    if(!(args.IsList(6) || (args.IsList(7) && args[6].IsInteger()))
      || !args[0].IsInteger() || !args[1].IsInteger()
      || !args[2].IsInteger() || !args[3].IsString()
      || !args[4].IsInteger() || !args[5].IsInteger())
	ArgumentError("create_book",args);

    x=args[0].Integer();
    y=args[1].Integer();
    num=args[2].Integer();
    nm=args[3].String();
    w=args[4].Integer();
    h=args[5].Integer();
    if(args.IsList(7))
      bc=args[6].Integer();
    else
      bc=0;

    if(IsObjectNumber(num))
	throw LangErr("create_book","Object number "+ToString(num)+" already in use.");
	
    CardBook* book=new CardBook(num,0,nm,bc,w,h,&parser);
    if(!book)
	throw Error::Memory("Table::create_book(const Data&)");

    table.push_back(book);
    objects[num]=book;

    book->grp.w=0;
    book->grp.h=0;
    book->grp.x=x;
    book->grp.y=y;
    book->grp.draggable=true;
    book->grp.visible=false;
    book->grp.sticky=true;
    book->grp.ontop=true;
    book->grp.clickable=true;
    book->grp.highlight=false;
    book->grp.compact=false;
    book->grp.tiny=false;
    book->RecalculateSize();
	
    return num;
}

/// book_last_page(o) - Return last page number of the book.
Data Table::book_last_page(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("book_last_page",args);
	
    OBJECTVAR(book,CardBook,args.Integer());

    return book->LastPage();
}

/// book_page(o) - Return current page number of the book.
Data Table::book_page(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("book_last_page",args);
	
    OBJECTVAR(book,CardBook,args.Integer());

    return book->Page();
}

/// book_set_page(o,p) - Set the current page of the card book $o$ to
/// $p$. Returns previous current page.
Data Table::book_set_page(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger()
      || !args[1].IsInteger())
	ArgumentError("book_set_page",args);
	
    OBJECTVAR(book,CardBook,args[0].Integer());

    int pg=args[1].Integer();
    if(pg < 0 || pg > book->LastPage())
	throw LangErr("book_set_page","invalid page "+ToString(pg));

    int old=book->Page();
    book->SetPage(pg);
    Refresh(book);   

    return old;
}

/// book_set_slot(o,s) - Set the current selected slot of the card book $o$ to
/// the card $s$ (integer) or to the title $s$ (string). Returns the
/// page having the slot or NULL if not found. String "" removes the
/// slot selection.
Data Table::book_set_slot(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger()
      || (!args[1].IsInteger() && !args[1].IsString()))
	ArgumentError("book_set_slot",args);
	
    OBJECTVAR(book,CardBook,args[0].Integer());

    int pg;

    if(args[1].IsInteger())
    {
	int n=args[1].Integer();
	if(n<0 || n >= Database::cards.Cards())
	    throw LangErr("book_set_slot","invalid card number "+ToString(n));
	pg=book->SetSlot(n);
    }
    else
	pg=book->SetSlot(args[1].String());

    Refresh(book);   

    if(pg==-1)
	return Null;

    return pg;
}

/// book_set_index(o,L) - Set the current index of the card book
/// $o$. A list $L$ contains titles and card numbers to show. Each
/// string is interpreted as a title and each integer as a card
/// number.
Data Table::book_set_index(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger()
      || !args[1].IsList())
	ArgumentError("book_set_index",args);
	
    OBJECTVAR(book,CardBook,args[0].Integer());

    list<BookIndexEntry> index;
    const Data& index_list=args[1];
	
    for(int i=0; i<(int)index_list.Size(); i++)
    {
	if(index_list[i].IsInteger())
        {
            int n=index_list[i].Integer();
            if(n>=0 && n<Database::cards.Cards())
                index.push_back(BookIndexEntry(n));
            else
                Warning("book_set_index: invalid image number "+ToString(n));
        }
	else if(index_list[i].IsString())
	    index.push_back(BookIndexEntry(index_list[i].String()));
	else
	    throw LangErr("book_set_index","invalid index entry "+tostr(args[i]).String());
    }

    book->SetIndex(index);
    Refresh(book);
	
    return Null;
}

/// book_set_entry(o,L) - Set card entries of the
/// book $o$. A list $L$ is a list of pairs
/// $((n_1,e_1),\dots,(n_k,e_k))$. Each $n_i$ is the card number of
/// the entry $e_i$ to be
/// set. Entry data $e_i$ is a vector $(c,p_s,s,p_m,f,d)$, where $c$ is the
/// number of cards owned, $p_s$ is the lowest price offered, $s$ is
/// name of the seller, $p_m$ is price set by me, $f$ is number of
/// cards for sale from my collection and $d$ is number of cards in my
/// current deck. Alternative shorter form for entries is $(c,f,p_m)$,
/// which sets only those fields of the entry.
Data Table::book_set_entry(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger()
      || !args[1].IsList())
	ArgumentError("book_set_entry",args);

    OBJECTVAR(book,CardBook,args[0].Integer());
	
    const Data& L=args[1];

    bool need_refresh=false;

    for(size_t i=0; i<L.Size(); i++)
    {
	if(!L[i].IsList(2) || !L[i][0].IsInteger() || (!L[i][1].IsList(2) && !L[i][1].IsList(3) && !L[i][1].IsList(5) && !L[i][1].IsList(6)))
	    throw LangErr("book_set_entry","invalid entry "+tostr(L[i]).String());
		
	int card=L[i][0].Integer();
	if(card < 0 || card >= Database::cards.Cards())
	    continue; // Ignore invalids

	BookEntry& e=book->EntryOf(card);
	need_refresh |= book->CardVisible(card);

	if(L[i][1].Size()==6 || L[i][1].Size()==5)
	{
	    e.count=L[i][1][0].Integer();
	    e.price=L[i][1][1].Real();
	    e.seller=L[i][1][2].String();
	    e.myprice=L[i][1][3].Real();
	    e.forsale=L[i][1][4].Integer();
	    if(L[i][1].Size()==6)
		e.ondeck=L[i][1][5].Integer();
	}
	else if(L[i][1].Size()==3)
	{
	    e.count=L[i][1][0].Integer();
	    e.forsale=L[i][1][1].Integer();
	    e.myprice=L[i][1][2].Real();
	}
	else if(L[i][1].Size()==2)
	{
	    e.seller=L[i][1][0].String();
	    e.price=L[i][1][1].Real();
	}
	else
	    throw LangErr("book_set_entry","invalid entry "+tostr(L[i][1]).String());
    }

    if(need_refresh)
	Refresh(book);
	
    return Null;
}

/// book_set_deck(o,L) - Set current "ondeck" cards according to
/// the list $L$.
Data Table::book_set_deck(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger()
      || !args[1].IsList())
	ArgumentError("book_set_deck",args);

    OBJECTVAR(book,CardBook,args[0].Integer());

    for(int i=0; i<Database::cards.Cards(); i++)
	book->EntryOf(i).ondeck=0;
		
    int c,n=0;
    Data D=flatten(args[1]);	
    const Data& L=D;
    for(size_t i=0; i<L.Size(); i++)
	if(L[i].IsInteger())
	{
	    c=L[i].Integer();
	    if(c < 0 || c >=Database::cards.Cards())
		continue; // Ignore invalid card number.

	    book->EntryOf(c).ondeck++;
	    n++;
	}

    Refresh(book);

    return n;
}

/// book_entry(o,n) - Return entry of card $n$ from book $o$ (See {\tt book\_set\_entry}).
Data Table::book_entry(const Data& args)
{
    VECTORIZE_SECOND(book_entry);
	
    if(!args.IsList(2) || !args[0].IsInteger()
      || !args[1].IsInteger())
	ArgumentError("book_entry",args);

    OBJECTVAR(book,CardBook,args[0].Integer());

    Data ret;
    ret.MakeList(6);
    int card=args[1].Integer();
	
    if(card < 0)
	throw LangErr("book_entry","invalid card number "+ToString(card));
    else if(card >= Database::cards.Cards())
    {
	Warning("book_entry: invalid image number "+ToString(card));

	ret[0]=0;
	ret[1]=0.0;
	ret[2]="";
	ret[3]=0.0;
	ret[4]=0;
	ret[5]=0;
	    
	return ret;
    }

    BookEntry& e=book->EntryOf(card);

    ret[0]=e.count;
    ret[1]=e.price;
    ret[2]=e.seller;
    ret[3]=e.myprice;
    ret[4]=e.forsale;
    ret[5]=e.ondeck;

    return ret;
}

/// book_cards(o,n) - Return all cards at the page $n$ in a book $o$.
Data Table::book_cards(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("book_cards",args);

    OBJECTVAR(book,CardBook,args[0].Integer());

    int page=args[1].Integer();

    if(page < 0 || page>book->LastPage())
	throw LangErr("book_cards","invalid page number "+ToString(page));

    list<BookIndexEntry*> entries=book->EntriesOfPage(page);

    Data ret;
    ret.MakeList();
    list<BookIndexEntry*>::iterator i;
    for(i=entries.begin(); i!=entries.end(); i++)
    {
	if((*i)->card != -1)
	    ret.AddList(Data((*i)->card));
    }

    return ret;
}

/// book_pageof(o,n) - Return page number of card $n$ in a book $o$.
Data Table::book_pageof(const Data& args)
{
    VECTORIZE_SECOND(book_pageof);
	
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("book_pageof",args);

    OBJECTVAR(book,CardBook,args[0].Integer());

    int card=args[1].Integer();
	
    if(card < 0 || card >= Database::cards.Cards())
	throw LangErr("book_pageof","invalid card number "+ToString(card));

    return book->PageOf(card);
}

/// book_titles(o) - Return list all of pairs $(t_i,p_i)$, where $t_i$
/// is a title from the book $o$ and $p_i$ is the page number
/// containing the title.
Data Table::book_titles(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("book_titles",args);

    OBJECTVAR(book,CardBook,args.Integer());

    const vector<pair<string,int> >& titles=book->Titles();

    Data ret;
    ret.MakeList(titles.size());

    for(int i=0; i<int(titles.size()); i++)
	ret[i]=Data(titles[i].first,titles[i].second / (book->Columns()*book->Rows()));

    return ret;
}

/// create_menu(x,y,n,s,M) - Create a menu at $(x,y)$ with object
/// number $n$ and name $s$. List of menu entries $M$ may contain
/// lists with one $(t,)$, two $(t,s)$ or three $(t,s,a)$ string
/// members. Here $t$ is a text to show on entry, $s$ is shortcut key
/// and $a$ is an action to send menu handler (i.e. code to execute).
Data Table::create_menu(const Data& args)
{
    string nm;
    int num,x,y;

    if(!args.IsList(5) || !args[0].IsInteger()
      || !args[1].IsInteger() || !args[2].IsInteger()
      || !args[3].IsString() || !args[4].IsList())
	ArgumentError("create_menu",args);

    x=args[0].Integer();
    y=args[1].Integer();
    num=args[2].Integer();
    nm=args[3].String();

    if(IsObjectNumber(num))
	throw LangErr("create_menu","Object number "+ToString(num)+" already in use.");
	
    Menu* menu=new Menu(num,0,nm);
    if(!menu)
	throw Error::Memory("Table::create_menu(const Data&)");

    menu->grp.w=0;
    menu->grp.h=0;
    menu->grp.x=x;
    menu->grp.y=y;
    menu->grp.draggable=true;
    menu->grp.visible=true;
    menu->grp.sticky=true;
    menu->grp.ontop=true;
    menu->grp.clickable=true;
    menu->grp.highlight=false;
    menu->grp.compact=false;
    menu->grp.tiny=false;
    menu->grp.fg=BLACK;
    menu->grp.bg=WHITE;
    menu->grp.text.color=BLACK;
    menu->grp.text.font=2;
    menu->grp.text.pointsize=16;
	
    table.push_back(menu);
    objects[num]=menu;

    const Data& L=args[4];

    for(size_t i=0; i<L.Size(); i++)
    {
	if(!L[i].IsList())
	    throw LangErr("create_menu","invalid menu entry '"+tostr(L[i]).String()+"'");

	size_t sz=L[i].Size();
	if(sz < 1 || sz >3)
	    throw LangErr("create_menu","invalid menu entry '"+tostr(L[i]).String()+"'");
	if(!L[i][0].IsString())
	    throw LangErr("create_menu","invalid menu entry '"+tostr(L[i]).String()+"'");
	if(sz>1 && !L[i][1].IsString())
	    throw LangErr("create_menu","invalid menu entry '"+tostr(L[i]).String()+"'");
	if(sz>2 && !L[i][2].IsString())
	    throw LangErr("create_menu","invalid menu entry '"+tostr(L[i]).String()+"'");
			
	string s1,s2,s3;

	if(sz > 0)
	    s1=L[i][0].String();
	if(sz > 1)
	    s2=L[i][1].String();
	if(sz > 2)
	    s3=L[i][2].String();

	menu->Add(s1,s2,s3);
    }
	
    menu->RecalculateSize();
    menu->grp.x-=menu->grp.w/2;
    menu->grp.y-=menu->grp.h/2;
    if(menu->grp.x < 0)
	menu->grp.x=0;
    if(menu->grp.y < 0)
	menu->grp.y=0;
    if(menu->grp.x + menu->grp.w > driver->PhysicalWidth())
	menu->grp.x=driver->PhysicalWidth()-menu->grp.w;
    if(menu->grp.y + menu->grp.h > driver->PhysicalHeight())
	menu->grp.y=driver->PhysicalHeight()-menu->grp.h;
	
    Refresh(menu);

    return num;
}

/// create_listbox(x,y,n,s,r,T,W,r_v,A) - Create a listbox at $(x,y)$
/// with object number $n$ and name $s$. The initial number of rows is
/// $r$ (table grows automatically), the number of visible rows is
/// $n_n$, list of titles is $T$, list of column widths is $W$ and
/// list of alignments (0 - left, 1 - center, 2 - right) is $A$. Note
/// that list box is not visible by default.
Data Table::create_listbox(const Data& args)
{
    string nm;
    int num,x,y,rows,columns,size;

    if(!args.IsList(9) || !args[0].IsInteger()
      || !args[1].IsInteger() || !args[2].IsInteger()
      || !args[3].IsString() || !args[4].IsInteger()
      || !args[5].IsList() || !args[6].IsList()
      || !args[7].IsInteger() || !args[8].IsList())
	ArgumentError("create_listbox",args);

    x=args[0].Integer();
    y=args[1].Integer();
    num=args[2].Integer();
    nm=args[3].String();
    rows=args[4].Integer();
    size=args[7].Integer();

    const Data& titles=args[5];
    const Data& widths=args[6];
    const Data& aligns=args[8];

    if(titles.Size() != widths.Size())
	throw LangErr("create_listbox","Number of titles and number of widths are different");
    if(titles.Size() != aligns.Size())
	throw LangErr("create_listbox","Number of titles and number of alignments are different");
    if(titles.Size() < 2)
	throw LangErr("create_listbox","There must be at least two columns");

    columns=titles.Size();

    if(IsObjectNumber(num))
	throw LangErr("create_listbox","Object number "+ToString(num)+" already in use.");
	
    ListBox* box=new ListBox(num,0,nm,columns,rows,size,17);
    if(!box)
	throw Error::Memory("Table::create_listbox(const Data&)");

    for(int i=0; i<columns; i++)
    {
	if(!widths[i].IsInteger() || widths[i].Integer() <= 0)
	    throw LangErr("create_listbox","Invalid column width");
	if(!aligns[i].IsInteger() || aligns[i].Integer() < 0 || aligns[i].Integer() > 2)
	    throw LangErr("create_listbox","Invalid column alignment");
	if(!titles[i].IsString())
	    throw LangErr("create_listbox","Invalid column title");
			
	box->SetColumn(i,widths[i].Integer(),titles[i].String());
	box->SetAlign(i,aligns[i].Integer());
    }

    table.push_back(box);
    objects[num]=box;

    box->grp.w=0;
    box->grp.h=0;
    box->grp.x=x;
    box->grp.y=y;
    box->grp.draggable=true;
    box->grp.visible=false;
    box->grp.sticky=true;
    box->grp.ontop=true;
    box->grp.clickable=true;
    box->grp.highlight=false;
    box->grp.compact=false;
    box->grp.tiny=false;
    box->grp.fg=Color(200,200,200);
    box->grp.bg=WHITE;
    box->grp.text.color=BLACK;
    box->RecalculateSize();
	
    return num;
}

/// listbox_set_entry(l,i,L) - Set a row $i$ of the list box $l$ to
/// contain a list $L$ as it's data. The size of the list $L$ must
/// match to the number of columns defined to the list box. String
/// entries of $L$ are used as they are and non-string entries are
/// converted to string before use.
Data Table::listbox_set_entry(const Data& args)
{
    if(args.IsList(3) && args[0].IsInteger() && args[1].IsInteger()  && args[2].IsList())
    {
	OBJECTVAR(box,ListBox,args[0].Integer());

	int row=args[1].Integer();
	const Data& L=args[2];

	if(row < 0)
	    throw LangErr("listbox_set_entry","invalid row number "+ToString(row));
	if(L.Size() != box->Columns())
	    throw LangErr("listbox_set_entry","invalid number of elements "+tostr(L).String());

	for(size_t i=0; i<L.Size(); i++)
	{
	    if(L[i].IsString())
		box->SetEntry(row,i,L[i].String());
	    else
		box->SetEntry(row,i,tostr(L[i]).String());
	}			  
	Refresh(box);
    }
    else
	ArgumentError("listbox_set_entry",args);

    return Null;
}

/// listbox_entry(l,r) - Return content of the row $r$ of the listbox
/// $l$.
Data Table::listbox_entry(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("listbox_entry",args);

    OBJECTVAR(box,ListBox,args[0].Integer());
	
    int row=args[1].Integer();
    if(row < 0)
	throw LangErr("listbox_entry","invalid row number "+ToString(row));

    Data ret;

    ret.MakeList(box->Columns());
	
    for(size_t i=0; i<box->Columns(); i++)
	ret[i]=box->GetEntry(row,i);

    return ret;
}

/// listbox_clear(l) - Clear all elements of list box $l$.
Data Table::listbox_clear(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("listbox_clear",args);

    OBJECTVAR(box,ListBox,args.Integer());
    box->Clear();
    Refresh(box);

    return Null;
}


/// listbox_set_deck(l,D,b) - Clear list box $l$ and construct deck
/// listing of the deck $D$ as it's new content. Check missing cards
/// from a card book $b$. Deck $D$ is hashmap from the part names of
/// the deck to the list of card numbers contained in each part.
/// Return dictionary of missing cards.
Data Table::listbox_set_deck(const Data& args)
{
    if(!args.IsList(3) || !args[0].IsInteger() || !args[1].IsList() || !args[2].IsInteger())
	ArgumentError("listbox_set_deck",args);

    OBJECTVAR(box,ListBox,args[0].Integer());
    OBJECTVAR(book,CardBook,args[2].Integer());

    box->Clear();
	
    const Data& L=args[1];

    string cardname; // Name of the current card.
    int total; // Total number of cards in the current deck part.
    int c,r=0,r1,r2;
    bool old_refresh=refresh;
    refresh=false;
	
    map<string,int> cards_missing; // Number of cards missing from collection.
    map<string,int> total_count; // Number of cards in deck all parts combined.
    map<int,int> card_count; // Number of cards with single image in total.

    Data ret;
    ret.MakeList();
    // Loop over deck parts.
    for(size_t i=0; i<L.Size(); i++)
    {
	map<string,int> part_count; // Number of cards in the current part.
	
	if(!L[i].IsList(2) || !L[i][0].IsString() || !L[i][1].IsList())
	    throw LangErr("listbox_set_deck","invalid deck element "+tostr(L[i]).String());

	total=0;

	const Data& K=L[i][1];

	// Loop over cards in the current deck part. Counting cards.
	for(size_t j=0; j<K.Size(); j++)
	{
	    if(!K[j].IsInteger())
		throw LangErr("listbox_set_deck","invalid card number "+tostr(K[j]).String());

	    c=K[j].Integer();
	    if(c < 0 || c >= Database::cards.Cards())
		continue; // Ignore invalid card number.
			
	    cardname=Database::cards.CanonicalName(c);
	    total_count[cardname]++;
	    part_count[cardname]++;
	    card_count[c]++;
	    if(card_count[c] > book->EntryOf(c).count)
		cards_missing[cardname]--;

	    total++;
	}

	box->SetEntry(r++,1,"  "+ucfirst(L[i][0]).String()+" ("+ToString(total)+")");
	r1=r;
	map<string,int>::iterator k;
	string type,color,color_opt,order_opt;
	color_opt=game.Option("deck color by");
	if(color_opt=="")
	    color_opt=game.Option("deck order");
	order_opt=game.Option("deck order");

	list<int> images;
	
	for(k=part_count.begin(); k!=part_count.end(); k++)
	{
	    box->SetEntry(r,0,ToString((*k).second));
	    if(cards_missing[(*k).first])
	      box->SetEntry(r,1,(*k).first+" {right}{red}("+ToString(cards_missing[(*k).first])+")");
	    else
		box->SetEntry(r,1,(*k).first);

	    images=cards.Images((*k).first);
	    if(images.size()==0)
		throw Error::Invalid("listbox_set_deck","cannot reverse map card name '"+(*k).first+"'");
			
	    type=cards.AttrValue(images.front(),order_opt);
	    color=cards.AttrValue(images.front(),color_opt);
	    type=game.Option(color+" text")+type;
			
	    box->SetEntry(r,2,type);
	    r++;
	}
	r2=r-1;
	listbox_sort_rows(Data(args[0],r1,r2,2,1));
    }
    
    map<string,int>::iterator k;
    for(k=cards_missing.begin(); k!=cards_missing.end(); k++)
      if(cards_missing[(*k).first])
        ret.AddList(Data((*k).first,-(cards_missing[(*k).first])));

    refresh=old_refresh;
    Refresh(box);

    return ret;
}

/// listbox_scroll(l,\delta) - Scroll list box content by $\delta$
/// rows up. 
Data Table::listbox_scroll(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsInteger())
	ArgumentError("listbox_scroll",args);
	
    OBJECTVAR(box,ListBox,args[0].Integer());
    int d=args[1].Integer();
    int f=box->First();
	
    if(d==0)
	return f;
    if(d < 0)
    {
	f=f+d;
	if(f < 0)
	    f=0;
	box->SetFirst(f);
    }
    else
	box->SetFirst(box->First()+d);
	
    Refresh(box);
	
    return box->First();
}

/// listbox_sort_rows(l,r_1,r_2,c) - Sort rows $r_1$--$r_2$ of the
/// list box $l$ using column $c$ as a key.
Data Table::listbox_sort_rows(const Data& args)
{
    int c2=-1;
	
    if(!(args.IsList(4)||args.IsList(5)) || !args[0].IsInteger() || !args[1].IsInteger()
      || !args[2].IsInteger() || !args[3].IsInteger())
	ArgumentError("listbox_sort_rows",args);

    OBJECTVAR(box,ListBox,args[0].Integer());

    if(args.IsList(5))
    {
	if(!args[4].IsInteger())
	    ArgumentError("listbox_sort_rows",args);

	c2=args[4].Integer();
	if(c2 < 0 || (size_t)c2 >= box->Columns())
	    throw LangErr("listbox_sort_rows","invalid column "+ToString(c2));
    }

    int r1=args[1].Integer();
    int r2=args[2].Integer();
    int c=args[3].Integer();

    if(c < 0 || (size_t)c >= box->Columns())
	throw LangErr("listbox_sort_rows","invalid column "+ToString(c));
    if(r1 < 0)
	r1=0;
    if(r2 < 0)
	r2=0;
    if((size_t)r1 >= box->Rows())
	r1=box->Rows()-1;
    if((size_t)r2 >= box->Rows())
	r2=box->Rows()-1;

    if(c2 >= 0)
	box->SortRows(r1,r2,c,c2);
    else
	box->SortRows(r1,r2,c);
	
    Refresh(box);

    return Null;
}

/// refresh(i) - Disable screen refresh (i.e. don't update screen
/// until {\tt refresh(1)} is called) if $i=0$ or enable it if $i\ne 0$.
/// Return refresh status before this call.
Data Table::_refresh(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("refresh",args);

    int old=refresh;
	
    refresh=args.Integer() != 0;
    if(refresh)
	Refresh();
	
    return old;
}

/// redraw(o) - Draw object $o$ again.
Data Table::redraw(const Data& args)
{
    VECTORIZE(redraw);
	
    if(!args.IsInteger())
	ArgumentError("redraw",args);

    int n=args.Integer();
    if(objects.find(n) == objects.end())
	throw LangErr("redraw","invalid object number "+ToString(n));

    Refresh(objects[n]);

    return n;
}

/// set_name(o,s) - Give a name $s$ to an object $o$. Return old name of the object.
Data Table::set_name(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsString())
	ArgumentError("set_name",args);

    Object* obj=GetObject("set_attr",args[0]);
    string name=GetString("set_attr",args[1]);

    string old=obj->Name();
    obj->SetName(name);

    return old;
}

/// beep(o,s) - Produce a beep.
Data Table::beep(const Data& args)
{
    if(!args.IsNull())
	ArgumentError("beep",args);

    driver->Beep();

    return Null;
}

/// blink(o) - Produce a visual alert.
Data Table::blink(const Data& args)
{
    if(!args.IsInteger())
		ArgumentError("blink",args);

	driver->Blink(args.Integer());

    return Null;
}

/// intersect(o) - Return a list of all other objects intersecting an object
/// $o$. Alternatively intersect($x$,$y$,$w$,$h$) returns a list
/// of all objects intersecting rectangle.
Data Table::intersect(const Data& args)
{
    int x=0,y=0,w=0,h=0;
    int o=-1;
	
    if(args.IsList(4) && args[0].IsInteger() && args[1].IsInteger()
      && args[2].IsInteger()  && args[3].IsInteger())
    {
	x=args[0].Integer();
	y=args[1].Integer();
	w=args[2].Integer();
	h=args[3].Integer();
    }
    else if(args.IsInteger())
    {
	Object* obj=GetObject("intersect",args.Integer());

	x=obj->grp.x;
	y=obj->grp.y;
	w=obj->grp.w;
	h=obj->grp.h;
	o=obj->Number();
    }
    else
	ArgumentError("intersect",args);

    Data ret;
    ret.MakeList();

    list<Object*>::iterator i;
    for(i=table.begin(); i!=table.end(); i++)
	if((*i)->Number()!=o && INTERSECT(x,y,w,h,(*i)->grp.x,(*i)->grp.y,(*i)->grp.w,(*i)->grp.h))
	    ret.AddList((*i)->Number());
	
    return ret;
}

/// set_cardsize(t,s) - Set card zoom factor $s$ for card type
/// $t$. The type $t$ is a string "hand", "book", "deck" or
/// "table". Returns old size.
Data Table::set_cardsize(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsString() || !args[1].IsInteger())
	ArgumentError("set_cardsize",args);

    string t=args[0].String();
    int sz=args[1].Integer();
    int old;
	
    if(sz < 1)
	throw LangErr("set_cardsize","invalid size "+ToString(sz));

    if(t=="hand")
    {
	old=HandCardSize();
	SetHandCardSize(sz);
    }
    else if(t=="deck")
    {
	old=DeckCardSize();
	SetDeckCardSize(sz);
    }
    else if(t=="book")
    {
	old=BookCardSize();
	SetBookCardSize(sz);
    }
    else if(t=="table")
    {
	old=TableCardSize();
	SetTableCardSize(sz);
    }
    else
	throw LangErr("set_cardsize","invalid type "+t);
		
    return old;
}

/// text_width(f,sz,s) - Return the text width of a string $s$ when
/// rendered at point size $sz$ using font $f$.
Data Table::text_width(const Data& args)
{
    if(!args.IsList(3) || !args[0].IsInteger() || !args[1].IsInteger() || !args[2].IsString())
	ArgumentError("text_width",args);
	
    int f=args[0].Integer();
    int sz=args[1].Integer();
    string s=args[2].String();

    if(f < 0 || f>=last_font)
	throw LangErr("text_width","invalid font "+ToString(f));
    if(sz < 1)
	throw LangErr("text_width","invalid size "+ToString(sz));

    int w,h;
    TextStyle style;
    style.font=f;
    style.pointsize=sz;
    driver->GetNextTextLine(s,style,w,h,9999999);

    return w;
}

/// text_height(f,sz,s) - Return the text width of a string $s$ when
/// rendered at point size $sz$ using font $f$.
Data Table::text_height(const Data& args)
{
    if(!args.IsList(3) || !args[0].IsInteger() || !args[1].IsInteger() || !args[2].IsString())
	ArgumentError("text_height",args);
	
    int f=args[0].Integer();
    int sz=args[1].Integer();
    string s=args[2].String();

    if(f < 0 || f>=last_font)
	throw LangErr("text_height","invalid font "+ToString(f));
    if(sz < 1)
	throw LangErr("text_height","invalid size "+ToString(sz));
		
    int w,h;
    TextStyle style;
    style.font=f;
    style.pointsize=sz;
    driver->GetNextTextLine(s,style,w,h,9999999);

    return h;
}

/// screen2table(x,y) - Convert screen coordinates (x,y) to table coordinates (x',y').
Data Table::screen2table(const Data& args)
{
    if(!args.IsList(2) || ! args[0].IsInteger() || ! args[1].IsInteger())
	ArgumentError("screen2table",args);

    int x,y;

    x=args[0].Integer();
    y=args[1].Integer();

    Screen2Table(x,y);

    return Data(x,y);
}

/// table2screen(x,y) - Convert screen coordinates (x,y) to table coordinates (x',y').
Data Table::table2screen(const Data& args)
{
    if(!args.IsList(2) || ! args[0].IsInteger() || ! args[1].IsInteger())
	ArgumentError("table2screen",args);

    int x,y;

    x=args[0].Integer();
    y=args[1].Integer();

    Table2Screen(x,y);

    return Data(x,y);
}

/// w(w) - Convert a width component to the physical pixel width.
Data Table::w(const Data& args)
{
    if(! args.IsInteger())
	ArgumentError("w",args);

    int w;

    w=args.Integer();
    Width2Screen(w);

    return w;
}

/// h(h) - Convert a width component to the physical pixel width.
Data Table::h(const Data& args)
{
    if(! args.IsInteger())
	ArgumentError("h",args);

    int h;

    h=args.Integer();
    Height2Screen(h);

    return h;
}

/// title(o) - Return title of the object $o$.
Data Table::title(const Data& args)
{
    VECTORIZE(title);
	
    Object* obj=GetObject("title",args.Integer());

    return obj->Title();
}

/// set_title(o,t) - Set title $t$ for the object $o$ and return previous title.
Data Table::set_title(const Data& args)
{
    if(!args.IsList(2) || !args[0].IsInteger() || !args[1].IsString())
	ArgumentError("set_title",args);
	
    Object* obj=GetObject("title",args[0].Integer());

    string old;

    old=obj->Title();
    obj->SetTitle(args[1].String());
		
    return old;
}

/// get_command(delay) - Get an GUI event as a command from the driver.
Data Table::get_command(const Data& args)
{
    if(!args.IsInteger())
	ArgumentError("get_command",args);

    Command c=driver->WaitCommand(args.Integer());
    Data ret;

    ret.MakeList(5);
    ret[0]=c.command;
    ret[1]=c.argument;
    ret[2]=(int)c.key;
    ret[3]=c.x;
    ret[4]=c.y;

    return ret;
}

/// InputSplit(line) - Return a pair where the first component is the input line
///   before the cursor and the second component after the cursor. Each
///   component is splitted list of the input atoms, i.e. mixed list of
///   characters and tags. If the line is empty string returns a pair
///   of two list each having a string "".
Data Table::InputSplit(const Data& args)
{
    if(!args.IsString())
	ArgumentError("InputSplit",args);
	
    const string& s=args.String();
    Data ret;
    ret.MakeList(2);

    if(s=="")
    {
	ret[0].MakeList(1);
	ret[1].MakeList(1);
	ret[0][0]="";
	ret[1][0]="";

	return ret;
    }

    list<string> left;
    list<string> right;
    list<string>* add_to=&left;

    string tag;
    bool in_tag=false;
    char c;
    char str[2];
    str[1]=0;

    for(size_t i=0; i<s.length(); i++)
    {
	c=s[i];
	if(c=='{')
	{
	    if(in_tag)
		for(size_t j=0; j<tag.length(); j++)
		{
		    str[0]=tag[j];
		    (*add_to).push_back(str);
		}

	    tag="{";
	    in_tag=true;
	}
	else if(c=='}')
	{
	    tag+="}";
	    if(tag=="{|}")
		add_to=&right;
	    else
		(*add_to).push_back(tag);
	    tag="";
	    in_tag=false;	    
	}
	else
	{
	    if(in_tag)
		tag+=c;
	    else
	    {
		str[0]=c;
		(*add_to).push_back(str);
	    }
	}
    }
    
    ret[0].MakeList(left.size());
    ret[1].MakeList(right.size());

    list<string>::const_iterator i;
    int j;

    for(j=0, i=left.begin(); i!=left.end(); j++,i++)
	ret[0][j]=*i;
    for(j=0, i=right.begin(); i!=right.end(); j++,i++)
	ret[1][j]=*i;

    return ret;
}

void Table::InitializeLibrary()
{
    parser.SetFunction("add_marker",&Table::add_marker);
    parser.SetFunction("add_text",&Table::add_text);
    parser.SetFunction("attach",&Table::attach);
    parser.SetFunction("beep",&Table::beep);
    parser.SetFunction("blink",&Table::blink);
    parser.SetFunction("book_entry",&Table::book_entry);
    parser.SetFunction("book_cards",&Table::book_cards);
    parser.SetFunction("book_last_page",&Table::book_last_page);
    parser.SetFunction("book_page",&Table::book_page);
    parser.SetFunction("book_pageof",&Table::book_pageof);
    parser.SetFunction("book_set_deck",&Table::book_set_deck);
    parser.SetFunction("book_set_entry",&Table::book_set_entry);
    parser.SetFunction("book_set_index",&Table::book_set_index);
    parser.SetFunction("book_set_page",&Table::book_set_page);
    parser.SetFunction("book_set_slot",&Table::book_set_slot);
    parser.SetFunction("book_titles",&Table::book_titles);
    parser.SetFunction("card",&Table::card);
    parser.SetFunction("card_data",&Table::card_data);
    parser.SetFunction("cardbox",&Table::cardbox);
    parser.SetFunction("center_of",&Table::center_of);
    parser.SetFunction("change_card",&Table::change_card);
    parser.SetFunction("clear_deck",&Table::clear_deck);
    parser.SetFunction("clear_msgbox",&Table::clear_msgbox);
    parser.SetFunction("create_book",&Table::create_book);
    parser.SetFunction("create_cardbox",&Table::create_cardbox);
    parser.SetFunction("create_deck",&Table::create_deck);
    parser.SetFunction("create_hand",&Table::create_hand);
    parser.SetFunction("create_image",&Table::create_image);
    parser.SetFunction("create_listbox",&Table::create_listbox);
    parser.SetFunction("create_menu",&Table::create_menu);
    parser.SetFunction("create_msgbox",&Table::create_msgbox);
    parser.SetFunction("deck",&Table::deck);
    parser.SetFunction("del_cardbox",&Table::del_cardbox);
    parser.SetFunction("del_cardbox_all_recenter",&Table::del_cardbox_all_recenter);
    parser.SetFunction("del_cardbox_recenter",&Table::del_cardbox_recenter);
    parser.SetFunction("del_deck",&Table::del_deck);
    parser.SetFunction("del_deck_bottom",&Table::del_deck_top);
    parser.SetFunction("del_deck_top",&Table::del_deck_top);
    parser.SetFunction("del_hand",&Table::del_hand);
    parser.SetFunction("del_marker",&Table::del_marker);
    parser.SetFunction("del_object",&Table::del_object);
    parser.SetFunction("del_all_texts",&Table::del_all_texts);
    parser.SetFunction("del_text",&Table::del_text);
    parser.SetFunction("detach",&Table::detach);
    parser.SetFunction("dump",&Table::dump);
    parser.SetFunction("get_attr",&Table::get_attr);
    parser.SetFunction("get_texts",&Table::get_texts);
    parser.SetFunction("h",&Table::h);
    parser.SetFunction("hand",&Table::hand);
    parser.SetFunction("has_text",&Table::has_text);
    parser.SetFunction("image_height",&Table::image_height);
    parser.SetFunction("image_width",&Table::image_width);
    parser.SetFunction("image_pixel",&Table::image_pixel);
    parser.SetFunction("inplay",&Table::inplay);
    parser.SetFunction("invert",&Table::invert);
    parser.SetFunction("intersect",&Table::intersect);
    parser.SetFunction("listbox_clear",&Table::listbox_clear);
    parser.SetFunction("listbox_set_deck",&Table::listbox_set_deck);
    parser.SetFunction("listbox_entry",&Table::listbox_entry);
    parser.SetFunction("listbox_set_entry",&Table::listbox_set_entry);
    parser.SetFunction("listbox_scroll",&Table::listbox_scroll);
    parser.SetFunction("listbox_sort_rows",&Table::listbox_sort_rows);
    parser.SetFunction("load_image",&Table::load_image);
    parser.SetFunction("lower",&Table::lower);
    parser.SetFunction("message",&Table::message);
    parser.SetFunction("mouse",&Table::mouse);
    parser.SetFunction("move_object",&Table::move_object);
    parser.SetFunction("msgbox_scroll",&Table::msgbox_scroll);
    parser.SetFunction("msgbox_search",&Table::msgbox_search);
    parser.SetFunction("msgbox_tail",&Table::msgbox_tail);
    parser.SetFunction("object_data",&Table::object_data);
    parser.SetFunction("object_name",&Table::object_name);
    parser.SetFunction("object_type",&Table::object_type);
    parser.SetFunction("objects",&Table::_objects);
    parser.SetFunction("put_cardbox",&Table::put_cardbox);
    parser.SetFunction("put_cardbox_recenter",&Table::put_cardbox_recenter);
    parser.SetFunction("put_deck_bottom",&Table::put_deck_bottom);
    parser.SetFunction("put_deck_top",&Table::put_deck_top);
    parser.SetFunction("put_hand",&Table::put_hand);
    parser.SetFunction("put_inplay",&Table::put_inplay);
    parser.SetFunction("raise",&Table::raise);
    parser.SetFunction("rot",&Table::rot);
    parser.SetFunction("scale_image",&Table::scale_image);
    parser.SetFunction("refresh",&Table::_refresh);
    parser.SetFunction("redraw",&Table::redraw);
    parser.SetFunction("replace_cardbox_recenter",&Table::replace_cardbox_recenter);
    parser.SetFunction("replace_cardbox",&Table::replace_cardbox);
    parser.SetFunction("screen2table",&Table::screen2table);
    parser.SetFunction("set_attr",&Table::set_attr);
    parser.SetFunction("set_bgcolor",&Table::set_bgcolor);
    parser.SetFunction("set_cardsize",&Table::set_cardsize);
    parser.SetFunction("set_fgcolor",&Table::set_fgcolor);
    parser.SetFunction("set_name",&Table::set_name);
    parser.SetFunction("set_title",&Table::set_title);
    parser.SetFunction("set_textalign",&Table::set_textalign);
    parser.SetFunction("set_textcolor",&Table::set_textcolor);
    parser.SetFunction("set_textfont",&Table::set_textfont);
    parser.SetFunction("set_textmargin",&Table::set_textmargin);
    parser.SetFunction("set_textsize",&Table::set_textsize);
    parser.SetFunction("set_textvalign",&Table::set_textvalign);
    parser.SetFunction("table2screen",&Table::table2screen);
    parser.SetFunction("tap",&Table::tap);
    parser.SetFunction("tap_left",&Table::tap_left);
    parser.SetFunction("tapped",&Table::tapped);
    parser.SetFunction("tapped_left",&Table::tapped_left);
    parser.SetFunction("text_width",&Table::text_width);
    parser.SetFunction("text_height",&Table::text_height);
    parser.SetFunction("title",&Table::title);
    parser.SetFunction("trigger",&Table::trigger);
    parser.SetFunction("untap",&Table::untap);
    parser.SetFunction("untapped",&Table::untapped);
    parser.SetFunction("w",&Table::w);
    parser.SetFunction("load_sound",&Table::load_sound);
    parser.SetFunction("play_sound",&Table::play_sound);
    parser.SetFunction("card_sound",&Table::card_sound);
    parser.SetFunction("get_command",&Table::get_command);

    parser.SetFunction("InputSplit",&Table::InputSplit);
}
