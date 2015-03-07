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

#ifndef GAME_H
#define GAME_H

#include <list>
#include <vector>
#include <deque>
#include <iostream>
#include <map>
#include "driver.h"
#include "parser.h"
#include "triggers.h"
#include "carddata.h"
#include "localization.h"

#define DEFAULT_DESIGN_WIDTH	1024
#define DEFAULT_DESIGN_HEIGHT	768

// MACROS

/// True if (x,y) is inside or on boundary of w x h box at (x0,y0).
#define ATBOX(x,y,x0,y0,w,h) ((x) >= (x0) && (x) < ((x0) + (w)) && (y) >= (y0) && (y) < ((y0)+(h)))
/// True if two boxes intersects.
#define INTERSECT(x,y,w,h,x1,y1,w1,h1) !((x)+(w)-1 < (x1) || (x1)+(w1)-1 < (x) || (y)+(h)-1 < (y1) || (y1)+(h1)-1 < (y))
/// Maximum.
#define MAX(x,y) ((x) > (y) ? (x) : (y))
/// Minimum.
#define MIN(x,y) ((x) < (y) ? (x) : (y))
/// Assign smallest box to (x,y,w,h) containting both two boxes.
#define BOUNDINGBOX(x,y,w,h,x1,y1,w1,h1,x2,y2,w2,h2) (x=MIN(x1,x2),y=MIN(y1,y2),w=MAX(x1+w1,x2+w2)-x,h=MAX(y1+h1,y2+h2)-y)

// DECLARATIONS

/// Game elements.
namespace CCG
{
    class Table;

    /// Image numbers of markers.
    extern vector<int> marker_image;
    /// Table object controlling game objects.
    extern Table* table;

    /// Classification of objects.
    enum ObjectType
	{
	    TypeImage,TypeCounter,TypeMarker,TypeCardInPlay,TypeHand,TypeDeck,TypeMessageBox,TypeCardBox,TypeCardBook,TypeMenu,TypeListBox
	};

    /// Graphical description of an game element.
    struct Widget
    {
	/// Widget's x-coordinate.
	int x;
	/// Widget's y-coordinate.
	int y;
	/// Widget width.
	int w;
	/// Widget height.
	int h;
	/// Is this widget visible?
	int visible;
	/// The object can be dragged around.
	int draggable;
	/// When dragging this object, drag parent instead.
	int dragparent;
	/// If 1 this object do not move when scrolling screen.
	int sticky;
	/// If 1 this object stays always on the top of the object stack.
	int ontop;
	/// If 1 this object stays always on the bottom of the object stack.
	int onbottom;
	/// If this object can be touched with mouse.
	int clickable;
	/// Highlighted border (usually used during dragging)
	int highlight;
	/// If object is presented in compact form if available.
	int compact;
	/// If object is presented in tiny form if available.
	int tiny;
	/// Try to fit texts inside widget if too large.
	int fit;
	/// This widget needs redraw.
	bool redraw;
		
	/// Foreground color
	Driver::Color fg;
	/// Background color
	Driver::Color bg;
	/// Current text style.
	Driver::TextStyle text;

	Widget()
	{fg=::Driver::WHITE; bg=::Driver::BLACK; visible=draggable=dragparent=sticky=ontop=onbottom=clickable=highlight=compact=tiny=fit=0; redraw=true;}
    };
	
    /// General base class for any object.
    class Object
    {
    protected:

	/// Number of each marker types on object.
	vector<int> markers;
		
	/// Parent object or 0 if none.
	Object* parent;
	/// Associated objects.
	list<Object*> attachments;
	/// Additional texts on object.
	vector<string> texts;

	/// Serial number of object.
	int number;
	/// Surface number or -1 if not allocated yet.
	int surface;
	/// Name of the object if any.
	string name;
	/// Title of the object if any (default is capitalized name).
	string title;

	/// Release earlier surface if exist and reallocate. Return number of the new surface.
	int AllocateSurface(int w,int h);

    public:

	/// Graphical description.
	Widget grp;

	/// Construct an object with number n and owner p. Parent object is set to prnt.
	Object(int n,Object* prnt,string _name="");
	/// Destructor.
	virtual ~Object();

	/// Print ASCII presentation of this object.
	virtual void Dump() const;

	/// Object number.
	int Number() const
	{return number;}

	/// Surface number.
	int Surface() const
	{return surface;}
	/// Attach this object to another.
	void Attach(Object* obj)
	{Detach(); parent=obj; obj->attachments.push_back(this);}
	/// Detach this object if attached.
	void Detach()
	{if(parent) {parent->attachments.remove(this); parent=0;}}
	/// Detach this object if attached and detach all of this card's attachemnts.
	void DetachAll();
	/// Parent address.
	Object* Parent() const
	{return parent;}
	/// Return address of the topmost element in attachment chain, or 0 if loop found.
	Object* Root();
	/// Return list of attached objects.
	list<Object*> Group()
	{return attachments;}
	/// Return the leftmost x-coordinate of this object and it's attachments.
	int GroupX() const;
	/// Return the leftmost y-coordinate of this object and it's attachments.
	int GroupY() const;
	/// Return the rightmost x-coordinate of this object and it's attachments.
	int GroupX2() const;
	/// Return the rightmost y-coordinate of this object and it's attachments.
	int GroupY2() const;
	/// Return total width of this object and it's attachments.
	int GroupW() const
	{return GroupX2() - GroupX() + 1;}
	/// Return total height of this object and it's attachments.
	int GroupH() const
	{return GroupY2() - GroupY() + 1;}

	/// Add one marker of the given type.
	void AddMarker(int type)
	{
	    if(type < 0 || type >= int(marker_image.size()))
		throw Error::Range("Object::AddMarker(int)","invalid type");
	    markers[type]++;
	    grp.redraw=true;
	}
	/// Remove one marker of the given type if available.
	void DelMarker(int type)
	{
	    if(type < 0 || type >= int(marker_image.size()))
		throw Error::Range("Object::DelMarker(int)","invalid type");
	    if(markers[type] > 0)
		markers[type]--;
	    grp.redraw=true;
	}
	/// Current number of markers of the given type.
	int Markers(int type) const
	{
	    if(type < 0 || type >= int(marker_image.size()))
		throw Error::Range("Object::Markers(int)","invalid type");
	    return markers[type];
	}

	/// Add text to the object.
	void AddText(const string& s);
	/// Delete text from the object.
	void DelText(const string& s);
	/// Return the number of texts added.
	int Texts() const
	{return texts.size();}
	/// Return i:th text on object.
	string Text(int i) const;
	/// Set title.
	void SetTitle(const string& t)
	{title=t;}
	string Title() const;
		
	/// Type of the object.
	virtual ObjectType Type() const = 0;
	/// Name of the object.
	virtual string Name() const
	{return name.empty() ? string("unknown") : name; }

	/// Set the name of the object.
	void SetName(const string& s)
	{name=s;}

	/// Draw object additions (usually called from class spesific Draw()).
	virtual void Draw();

	/// Update w and h components of the widget.
	virtual void RecalculateSize()
	{AllocateSurface(grp.w,grp.h);}

	/// Return 1 if object is clickable and (x,y) belongs to visible part of image.
	virtual bool ClickableAt(int x,int y) const;

	/// Short cut to table width conversion.
	int W(int w) const;
	/// Short cut to table height conversion.
	int H(int h) const;
    };

    /// Free image object
    class Image : public Object
    {
	int image;
		
    public:

	Image(int img,int n,Object* prnt,string _name="") : Object(n,prnt,_name)
	{image=img; grp.text.margin=10;}

	/// Type of the object.
	ObjectType Type() const
	{return TypeImage;}
	/// Draw object additions (usually called from class spesific Draw()).
	void Draw();
	/// Update w and h components of the widget.
	void RecalculateSize();
    };

	
    /// Playcard on table.
    class CardInPlay: public Object
    {
	/// Player number of the owner.
	int owner;
	/// Card number of visible side.
	int card;
	/// Card number if facedown.
	int realcard;
	/// Rotation angle.
	int angle;
		
    public:

	/// Constructor.
	CardInPlay(int num,Object* parent,int _angle,int crd,int own) : Object(num,parent)
	{grp.text.font=4; realcard=card=crd; owner=own; angle=_angle;}
	/// Destructor.
	virtual ~CardInPlay()
	{}
		
	ObjectType Type() const
	{return TypeCardInPlay;}

	void Dump() const;

	/// Tap a card.
	void Tap();
	/// Tap a card left.
	void TapLeft();
	/// Untap a card.
	void Untap();
	/// Invert a card.
	void Invert();
	/// Rotate card to specific angle.
	void Rot(int angle);
	/// Return current rotation angle of the card.
	int Angle() const
	{return angle;}
	/// Is card tapped.
	bool IsTapped() const
	{return angle==90;}
	/// Is card tapped left.
	bool IsLeftTapped() const
	{return angle==270;}
	/// Is card untapped.
	bool IsUntapped() const
	{return angle==0;}
	/// Is card upsidedown.
	bool IsUpsideDown() const
	{return angle==180;}
	/// Card number of the visible side of the card.
	int Number() const
	{return card;}
	/// Real card nuber of the card of known.
	int RealCard() const
	{return realcard;}
	/// Owner of the card.
	int Owner() const
	{return owner;}
	/// Card name.
	string Name() const
	{return Database::cards.Name(card);}
	/// Change card image.
	void ChangeCard(int n)
	{realcard=card; card=n; grp.redraw=true;}

	void Draw();
	void RecalculateSize();
    };
	
    /// Cards on players' hand.
    class Hand : public Object
    {
	/// Card offsets at screen.
	int xoffset, yoffset;

	/// Card numbers in hand.
	vector<int> cards;
	/// Size of the cards.
	int size;

    public:

	/// Constructor.
	Hand(int num,Object* parent,const string& name) : Object(num,parent,name)
	{grp.fg.r=250; grp.fg.g=130; grp.fg.b=30;size=100;xoffset=yoffset=20;}
	/// Desctructor.
	virtual ~Hand()
	{}
	ObjectType Type() const
	{return TypeHand;}
	size_t Cards() const
	{return cards.size();}

	/// Return card number at hand position idx.
	int operator[](int idx) const
	{if(idx < 0 || idx >= (int)Cards()) throw Error::Range("int Hand::operator[](int)","invalid hand card number"); return cards[idx];}
	/// Add a card into the hand.
	void Add(int card);
	/// Remove a card from the hand.
	void Del(int index);

	/// Return index of the card at (x,y) or -1 if none.
	int CardAt(int x,int y) const;

	void Dump() const;
	bool ClickableAt(int x,int y) const;
	void Draw();
	void RecalculateSize();
    };

    /// Message box.
    class MessageBox : public Object
    {
	/// Text contents.
	deque<string> content;
	/// Initial style for each line.
	deque<Driver::TextStyle> linestyle;
	/// How many lines.
	size_t lines;
	/// How many bottom lines are skipped (i.e. scrolling backward) when rendering.
	int offset;
		
    public:
		
	/// Constructor.
	MessageBox(int num,Object* parent,size_t _lines,string name) : Object(num,parent,name)
	{lines=_lines; offset=0;}
	/// Destructor.
	virtual ~MessageBox()
	{}

	ObjectType Type() const
	{return TypeMessageBox;}
	string Name() const
	{if(name!="") return name; else return "message box";}

	/// Add text to the box.
	void WriteMessage(const string& text);
	/// Messages.
	const deque<string>& Messages() const
	{return content;}
	/// Scroll messages one line up if possible.
	void ScrollUp();
	/// Scroll messages one line down if possible.
	void ScrollDown();
	/// Clear all messages.
	void Clear()
	{content.clear(); linestyle.clear(); offset=0;}
	/// Return lines to skip from bottom.
	int Offset() const
	{return offset;}

	void Draw();
    };
			
    /// Some ordered stack of cards.
    class Deck : public Object
    {
	/// Card images beloging to the deck.
	deque<int> cards;
	/// Size percentage of the cards in the deck.
	int size;

    public:

	/// Constructor.
	Deck(int num,Object* parent,string name) : Object(num,parent,name)
	{size=100;}
	/// Destructor.
	virtual ~Deck()
	{}

	/// Add a card to the top of the deck.
	void AddTop(int card)
	{cards.push_back(card);	grp.redraw=true;}
	/// Add a card to the bottom of the deck.
	void AddBottom(int card)
	{cards.push_front(card); grp.redraw=true;}
	/// Remove top card of the deck if available.
	int DelTop()
	{int ret=-1; if(cards.size()) {ret=cards.back(); cards.pop_back(); grp.redraw=true;} return ret;}
	/// Remove bottom card of the deck if available.
	int DelBottom()
	{int ret=-1; if(cards.size()) {ret=cards.front(); cards.pop_front(); grp.redraw=true;} return ret;}
	/// Remove all cards from the deck.
	void DelAll()
	{cards.clear(); grp.redraw=true;}
	/// Remove the card at position 'i' counting from the top.
	void Del(int index);
	/// Size of the deck.
	size_t Size() const
	{return cards.size();}
	/// Return the topmost card of the deck or -1 if empty.
	int Top() const
	{
	    if(cards.size())
		return cards.back();
	    return -1;
	}
	/// Return card number 'idx' counting from the bottom of the deck.
	int operator[](size_t idx) const
	{
	    // note: can't be negative, size_t is always positive
	    if(idx >= Size())
		throw Error::Range("int Deck::operator[](int)","invalid deck card number");
	    return cards[idx];
	}

		
	ObjectType Type() const
	{return TypeDeck;}
	void Draw();
	void RecalculateSize();
	bool ClickableAt(int x,int y) const;
	void Dump() const;
    };

    class CardBox : public Object
    {
	/// Maximum number of cards per row.
	int cards_per_row;
	/// Maximum number of cards per column without overlap.
	int cards_per_col;
	/// Current card size factor.
	int size;
	/// Width of the widest card.
	int max_cardwidth;
	/// Height of the tallest card.
	int max_cardheight;
	/// Current number of rows.
	int rows;
	/// Current overlapping in pixels.
	int overlap;
	/// True, if all cards do not have the same size.
	bool rotate;
	/// Card numbers in box.
	vector<int> cards;
		
    public:

	/// Constructor.
	CardBox(int num,Object* parent,const string& name) : Object(num,parent,name)
	{cards_per_row=0; cards_per_col=0;}
	/// Desctructor.
	virtual ~CardBox()
	{}
	ObjectType Type() const
	{return TypeCardBox;}
	size_t Cards() const
	{return cards.size();}

	/// Return card number at hand position idx.
	int operator[](int idx) const
	{if(idx < 0 || idx >= (int)Cards()) throw Error::Range("int CardBox::operator[](int)","invalid card number"); return cards[idx];}
	/// Add a card into the card box.
	void Add(int card)
	{cards.push_back(card); grp.redraw=true;}
	/// Remove a card from the card box.
	void Del(int index);

	/// Return index of the card at (x,y) or -1 if none.
	int CardAt(int x,int y) const;

	void Dump() const;
	void Draw();
	void RecalculateSize();
    };
	
    /// A card entry in the card book.
    struct BookEntry
    {
	/// Card number.
	int card;
	/// Number of these cards.
	int count;
	/// Number of these cards on current deck.
	int ondeck;
	/// Number of my cards for sale.
	int forsale;
	/// My selling price if for sale.
	double myprice;
	/// Name of the salesman, if any ("" otherwise).
	string seller;
	/// Currently the lowest known market price.
	double price;

	BookEntry()
	{card=0; count=0; ondeck=0; forsale=0; myprice=0.0; price=0.0;}
	BookEntry(int cardnumber)
	{card=cardnumber; count=0; ondeck=0; forsale=0; myprice=0.0; price=0.0;}
    };

    /// Indexing entry for the card book.
    struct BookIndexEntry
    {
	/// Cardnumber or -1 if an title entry.
	int card;
	/// Title name or "" if an card entry.
	string title;

	BookIndexEntry()
	{card=-1; title="";}
	BookIndexEntry(const string& _title)
	{card=-1; title=_title;}
	BookIndexEntry(int _card)
	{card=_card; title="";}
    };
	
    /// A card book.
    class CardBook : public Object
    {
	/// Pointer to parser for accessing some variables.
	Evaluator::Parser<Table> *parser;
	/// Book entries indexed by card number.
	vector<BookEntry> cards;
	/// Index table to map card slot in the book to card entries.
	/// This is currently visible content of the book.
	vector<BookIndexEntry> index;
	/// Index table to map card numbers to slot entries or -1 if filtered off.
	vector<int> card_index;
	/// List of pairs (title,slot index) of current collection titles.
	vector<pair<string,int> > titles;
	/// List of tab widths.
	vector<int> tabwidths;

	/// Number of card columns.
	int columns;
	/// Number of card rows.
	int rows;
	/// Current page (pg.1 means page==0).
	int page;
	/// Selected slot or -1 if none.
	int slot;
	/// Card width.
	int cardw;
	/// Card height.
	int cardh;
	/// Margin size between cards.
	int margin;
	/// Height of the title bar.
	int titleh;
	/// Height of the tabs toolbar.
	int toolh;
	/// Width of the tabs toolbar (may be wider than the book).
	int toolw;
	/// Height of the tabs.
	int tabh;
	/// Space between left corners of two tab.
	int tabskip;

	/// Draw card book background graphics.
	void DrawBackground(int x,int y);
	/// Draw empty card slot.
	void DrawSlot(int x,int y,int w,int h,bool odd_even,bool selected);
	/// Draw a card.
	void DrawCard(BookEntry& entry,int x,int y,int w,int h,int trade_limit);
	/// Draw a card in spreadsheet mode.
	void DrawSpreadsheetCard(BookEntry& entry,int x,int y,int w,int h,int trade_limit);
	/// Draw a title.
	void DrawTitle(const string& title,int x,int y,int w,int h);
	/// Draw a tab.
	void DrawTab(int section, const string& title,bool active);
	/// Draw all tabs.
	void DrawTabs();
	/// Return relative position of the section tab.
	void TabPos(int section,int& x,int& y) const;
	/// Return the actual width of the section tab, which may differ from preferred.
	int TabWidth(int section) const;
	/// Return the actual height of the section tab, which may differ from preferred.
	int TabHeight(int section) const;

    public:

	CardBook(int num,Object* parent,const string& name,int basecard,int w,int h,Evaluator::Parser<Table> *parser);
	virtual ~CardBook();

	ObjectType Type() const
	{return TypeCardBook;}
	void Dump() const;
	void Draw();
	void RecalculateSize();

	/// Return position index (0..cols x rows - 1) of the card at (x,y) or -1 if none.
	int SlotAt(int x,int y) const;
	/// Return card number at the current page book position idx or -1 if none.
	int CardAt(int idx) const;
	/// Return section tab index at (x,y) or -1 if none.
	int TabAt(int x,int y) const;
	/// Number of currently selected entries.
	int Entries() const
	{return index.size();}
	/// List of entries on page n.
	list<BookIndexEntry*> EntriesOfPage(int n);
	/// Return reference to index entry n.
	BookIndexEntry& Entry(int n);
	/// Return reference to index entry n.
	const BookIndexEntry& Entry(int n) const;
	/// Return reference to entry of card number n.
	BookEntry& EntryOf(int n);
	/// Return page number of the card n or -1 if filtered off.
	int PageOf(int n) const;
	/// Change current page.
	void SetPage(int newpage);
	/// Number of card rows.
	int Rows() const
	{return rows;}
	/// Number of card columns.
	int Columns() const
	{return columns;}
	/// Current page.
	int Page() const
	{return page;}
	/// Return the title at the first slot of the page or previous
	/// title from earlier pages.
	string TitleOfPage(int) const;
	/// Return current last page.
	int LastPage() const;
	/// Return true if card 'c' is visible, i.e. on the current page.
	bool CardVisible(int c) const;
	/// Set new index to the book.
	void SetIndex(const list<BookIndexEntry>& entries);
	/// Select a slot. Return the page number or -1 if the title is not found.
	int SetSlot(const string& title);
	/// Select a slot. Return the page number or -1 if the card is not found.
	int SetSlot(int card);
	/// Return a list of pairs (title,page) of collection titles.
	const vector<pair<string,int> >& Titles() const
	    {return titles;}
	/// Return the number of sections.
	int Sections() const;
	/// Return the title of the section or "" if invalid section number.
	const string& Section(int section) const;
	/// Return the number of the currently selected section or -1 if none.
	int CurrentSection() const;
    };

    /// Menu entries
    struct MenuEntry
    {
	string text;
	string shortcut;
	string action;

	MenuEntry(const string& txt,const string& key,const string& act)
	{text=txt;shortcut=key;action=act;}
    };
	
    /// Menu object.
    class Menu : public Object
    {
	/// Content of menu entries.
	list<MenuEntry> entries;
	/// Entries, that have separator before the entry.
	list<int> separators;
	/// Number of columns.
	int columns;
	/// Number of rows.
	int rows;
	/// Entry width.
	int entryw;
	/// Entry height.
	int entryh;
	/// Title height.
	int titleh;
			
    public:

	Menu(int num,Object* parent,const string& name) : Object(num,parent,name)
	{rows=0; columns=0;entryw=0;entryh=0;titleh=0;}
	virtual ~Menu()
	{}

	/// Add an entry to menu.
	void Add(const string& text,const string& shortcut,const string& action);
	/// Return index of menu entry at (x,y) or -1 if none.
	int At(int x,int y) const;
	/// Return a reference to n:th menu entry.
	const MenuEntry& operator[](int n) const;
			
	ObjectType Type() const
	{return TypeMenu;}
	void Dump() const;
	void Draw();
	void RecalculateSize();
    };

    /// General list object.
    class ListBox : public Object
    {
	/// First line to show.
	int first;
	/// Number of lines to show.
	int windowsize;
	/// Height of one line in pixels (including border).
	int height;
	/// Index of the key column.
	int key;
	/// Column widths in pixels (including border).
	vector<int> width;
	/// Column alignments (0 - left, 1 - center, 2 - right)
	vector<int> align;
	/// Column titles.
	vector<string> title;
	/// Content of list box entries (titles are vectors of one string).
	vector<vector<string> > row;
	/// Mapping of index column contents to real index.
	map<string,int> index;
		
    public:

	ListBox(int num,Object* parent,const string& name,int columns,int rows,int wsize,int height);
	virtual ~ListBox()
	{}

	/// Return size of the table.
	size_t Rows() const;
	/// Return the number of columns of the table.
	size_t Columns() const
	{return title.size();}
	/// Return index of list box entry at (x,y) or -1 if none.
	int At(int x,int y) const;
	/// Set width of the column and it's title.
	void SetColumn(int column,int width,const string& title);
	/// Set alignment of the column.
	void SetAlign(int column,int align);
	/// Set entry of table. Enlarge table if necessary.
	void SetEntry(int row,int column,const string& content);
	/// Set first row to show.		
	void SetFirst(int row)
	{
	    if(row < 0)
		throw Error::Range("ListBox::SetFirst","negative row");
	    first=row;
	    grp.redraw=true;
	}
	/// Return first visible row.
	int First() const
	{return first;}

	/// Get entry of table. Enlarge table if necessary.
	const string& GetEntry(int row,int column);
	/// Return true if all elements in row are empty.
	bool IsEmptyRow(int row) const;
	/// Remove all entries.
	void Clear();
	/// Sort rows from r1 to r2 using column c as a key and c2 as secondary key.
	void SortRows(int r1,int r2,int c,int c2=-1);
		
	ObjectType Type() const
	{return TypeListBox;}
	void Dump() const;
	void Draw();
	void RecalculateSize();
    };

    /// Complete description of the game table.
#ifndef USE_SQUIRREL
    class Table
#else
    class Table : public Evaluator::Libsquirrel::SQHelper 
#endif
    {
	// Variables
	// ---------

	/// Debugging mode 0 - none, 1 - some, 2 - all
	int debugmode;
	/// Size percentage of the cards on table.
	int table_card_size;
	/// Size percentage of the cards in hand.
	int hand_card_size;
	/// Size percentage of the cards in deck.
	int deck_card_size;
	/// Size percentage of the cards in collection.
	int book_card_size;

	/// Design size of the screen.
	static int design_width,design_height;
	/// Origin of the table coordinates in physical pixel coordinates.
	int tablex0,tabley0;
	/// Size of the table in physical coordinates.
	int table_width,table_height;
		
	/// Client program.
	Evaluator::Triggers event_triggers;
	/// If false, screen refresh is disabled.
	bool refresh;
	/// Number of last usable font.
	int last_font;
	/// List of objects on table. Last object is the topmost.
	list<Object*> table;
	/// Additional access helper that maps object numbers to objects.
	map<int,Object*> objects;
	/// Language parser for this table.
	Evaluator::Parser<Table> parser;

	// Widget functions
	// ----------------
		
	/// Find 'clickable' widget at screen coordinates x,y. Return 0 if none found.
	Object* WidgetAt(int scrx,int scry);
	/// Add intersecting widget pointers to the list.
	void GetIntersectingWidgets(int x0,int y0,int w,int h,list<Object*>& widgs) const;
	/// Draw the widget to it'äs surface.
	void Draw(Object* obj) const;
	/// Move object by given offset.
	void Move(Object* obj,int dx,int dy);
	/// Move object and it's group by given offset.
	void MoveGroup(Object* obj,int dx,int dy);
	/// Hide all objects of the object and it's group.
	void HideGroup(Object* obj);
	/// Reveal all objects of the object and it's group.
	void ShowGroup(Object* obj);
	/// Return all attached objects recursively.
	void GetGroup(Object* obj,list<Object*>& grp) const;
	/// Raise an object above other objects.
	void Raise(Object* obj);
	/// Lower an object below other objects.
	void Lower(Object* obj);
	/// Raise objects above other objects keeping their relative positions unchanged.
	void Raise(const list<Object*>& objs);
				
	/// Delete object.
	void Delete(Object* obj);
	/// Redraw all widgets on given area. If update flag is set, tell display driver to update it's visible screen.
	void Refresh(int x0,int y0,int w,int h,bool update=true) const;
	/// Redraw widgets under widget area. If update flag is set, tell display driver to update it's visible screen.
	void Refresh(Object* obj,bool update=true) const
	{Refresh(obj->grp.x,obj->grp.y,obj->grp.w,obj->grp.h,update);}
	/// Refresh union of the old position and the current position.
	void Refresh(const Widget&old, Object* obj,bool update=true) const;
	/// Redraw all widgets. If update flag is set, tell display driver to update it's visible screen.
	void Refresh(bool update=true) const;
	/// Perform mouse dragging for object at screen coordinates x,y. Store top left coordinates of the object to (scrx,scry) and object number to 'num' when finished and return true if there was an onject at x,y.
	bool DragWidget(const string& button, int& scrx,int& scry,int& num);
	/// Check if trigger exists and evaluate it if it does exist in which case return 1.
	bool TryTrigger(const string& key1,const string& key2,Evaluator::Data& value);

	// Structural functions
	// --------------------		

	/// Return 1, if object number is in use.
	bool IsObjectNumber(int number);
	/// Find an object referred by number. If not found, throw Error::Syntax.
	Object* GetObject(const char* function,const Evaluator::Data& args);
	/// Find a card referred by number. If not found or object is not a card, throw Error::Syntax.
	CardInPlay* GetCard(const char* function,const Evaluator::Data& args);

	// Other
	// ----

	/// Set card size percentage with respect to original for play cards.
	void SetTableCardSize(int prcnt);
	/// Set card size percentage with respect to original for decks.
	void SetDeckCardSize(int prcnt);
	/// Set card size percentage with respect to original for hand cards.
	void SetHandCardSize(int prcnt);
	/// Set card size percentage with respect to original for collection cards.
	void SetBookCardSize(int prcnt);
	/// Show warning message.
	void Warning(const string& message);

	//  Implementations of script language GUI functions
	//  ------------------------------------------------
		
	Evaluator::Data _objects(const Evaluator::Data&);
	Evaluator::Data add_marker(const Evaluator::Data&);
	Evaluator::Data add_text(const Evaluator::Data&);
	Evaluator::Data attach(const Evaluator::Data&);
	Evaluator::Data attrs(const Evaluator::Data&);
	Evaluator::Data beep(const Evaluator::Data&);
	Evaluator::Data blink(const Evaluator::Data&);
	Evaluator::Data book_cards(const Evaluator::Data&);
	Evaluator::Data book_entry(const Evaluator::Data&);
	Evaluator::Data book_last_page(const Evaluator::Data&);
	Evaluator::Data book_page(const Evaluator::Data&);
	Evaluator::Data book_pageof(const Evaluator::Data&);
	Evaluator::Data book_set_deck(const Evaluator::Data&);
	Evaluator::Data book_set_index(const Evaluator::Data&);
	Evaluator::Data book_set_entry(const Evaluator::Data&);
	Evaluator::Data book_set_page(const Evaluator::Data&);
	Evaluator::Data book_set_slot(const Evaluator::Data&);
	Evaluator::Data book_sort(const Evaluator::Data&);
	Evaluator::Data book_titles(const Evaluator::Data&);
	Evaluator::Data card(const Evaluator::Data&);
	Evaluator::Data card_data(const Evaluator::Data&);
	Evaluator::Data cardbox(const Evaluator::Data&);
	Evaluator::Data center_of(const Evaluator::Data&);
	Evaluator::Data change_card(const Evaluator::Data&);
	Evaluator::Data clear_deck(const Evaluator::Data&);
	Evaluator::Data clear_msgbox(const Evaluator::Data&);
	Evaluator::Data create_book(const Evaluator::Data&);
	Evaluator::Data create_cardbox(const Evaluator::Data&);
	Evaluator::Data create_deck(const Evaluator::Data&);
	Evaluator::Data create_hand(const Evaluator::Data&);
	Evaluator::Data create_image(const Evaluator::Data&);
	Evaluator::Data create_listbox(const Evaluator::Data&);
	Evaluator::Data create_menu(const Evaluator::Data&);
	Evaluator::Data create_msgbox(const Evaluator::Data&);
	Evaluator::Data deck(const Evaluator::Data&);
	Evaluator::Data del_cardbox(const Evaluator::Data&);
	Evaluator::Data del_cardbox_all_recenter(const Evaluator::Data&);
	Evaluator::Data del_cardbox_recenter(const Evaluator::Data&);
	Evaluator::Data del_deck(const Evaluator::Data&);
	Evaluator::Data del_deck_bottom(const Evaluator::Data&);
	Evaluator::Data del_deck_top(const Evaluator::Data&);
	Evaluator::Data del_hand(const Evaluator::Data&);
	Evaluator::Data del_marker(const Evaluator::Data&);
	Evaluator::Data del_object(const Evaluator::Data&);
	Evaluator::Data del_all_texts(const Evaluator::Data&);
	Evaluator::Data del_text(const Evaluator::Data&);
	Evaluator::Data detach(const Evaluator::Data&);
	Evaluator::Data dump(const Evaluator::Data&);
	Evaluator::Data get_attr(const Evaluator::Data&);
	Evaluator::Data get_command(const Evaluator::Data&);
	Evaluator::Data get_texts(const Evaluator::Data&);
	Evaluator::Data h(const Evaluator::Data&);
	Evaluator::Data hand(const Evaluator::Data&);
	Evaluator::Data has_text(const Evaluator::Data&);
	Evaluator::Data image_height(const Evaluator::Data&);
	Evaluator::Data image_width(const Evaluator::Data&);
	Evaluator::Data image_pixel(const Evaluator::Data&);
	Evaluator::Data inplay(const Evaluator::Data&);
	Evaluator::Data intersect(const Evaluator::Data&);
	Evaluator::Data invert(const Evaluator::Data&);
	Evaluator::Data listbox_clear(const Evaluator::Data&);
	Evaluator::Data listbox_entry(const Evaluator::Data&);
	Evaluator::Data listbox_set_deck(const Evaluator::Data&);
	Evaluator::Data listbox_set_entry(const Evaluator::Data&);
	Evaluator::Data listbox_scroll(const Evaluator::Data&);
	Evaluator::Data listbox_sort_rows(const Evaluator::Data&);
	Evaluator::Data load_image(const Evaluator::Data&);
	Evaluator::Data lower(const Evaluator::Data&);
	Evaluator::Data message(const Evaluator::Data&);
	Evaluator::Data msgbox_scroll(const Evaluator::Data&);
	Evaluator::Data msgbox_search(const Evaluator::Data&);
	Evaluator::Data msgbox_tail(const Evaluator::Data&);
	Evaluator::Data mouse(const Evaluator::Data&);
	Evaluator::Data move_object(const Evaluator::Data&);
	Evaluator::Data name(const Evaluator::Data&);
	Evaluator::Data object_data(const Evaluator::Data&);
	Evaluator::Data object_name(const Evaluator::Data&);
	Evaluator::Data object_type(const Evaluator::Data&);
	Evaluator::Data put_cardbox(const Evaluator::Data&);
	Evaluator::Data put_cardbox_recenter(const Evaluator::Data&);
	Evaluator::Data put_deck_bottom(const Evaluator::Data&);
	Evaluator::Data put_deck_top(const Evaluator::Data&);
	Evaluator::Data put_hand(const Evaluator::Data&);
	Evaluator::Data put_inplay(const Evaluator::Data&);
	Evaluator::Data raise(const Evaluator::Data&);
	Evaluator::Data _refresh(const Evaluator::Data&);
	Evaluator::Data redraw(const Evaluator::Data&);
	Evaluator::Data replace_cardbox_recenter(const Evaluator::Data&);
	Evaluator::Data replace_cardbox(const Evaluator::Data&);
	Evaluator::Data rot(const Evaluator::Data&);
	Evaluator::Data scale_image(const Evaluator::Data&);
	Evaluator::Data screen2table(const Evaluator::Data& args);
	Evaluator::Data set_attr(const Evaluator::Data&);
	Evaluator::Data set_name(const Evaluator::Data&);
	Evaluator::Data set_cardsize(const Evaluator::Data&);
	Evaluator::Data set_bgcolor(const Evaluator::Data&);
	Evaluator::Data set_fgcolor(const Evaluator::Data&);
	Evaluator::Data set_textfont(const Evaluator::Data&);
	Evaluator::Data set_textalign(const Evaluator::Data&);
	Evaluator::Data set_textcolor(const Evaluator::Data&);
	Evaluator::Data set_textmargin(const Evaluator::Data&);
	Evaluator::Data set_textvalign(const Evaluator::Data&);
	Evaluator::Data set_textsize(const Evaluator::Data&);
	Evaluator::Data set_title(const Evaluator::Data&);
	Evaluator::Data table2screen(const Evaluator::Data& args);
	Evaluator::Data tap(const Evaluator::Data&);
	Evaluator::Data tap_left(const Evaluator::Data&);
	Evaluator::Data tapped(const Evaluator::Data&);
	Evaluator::Data tapped_left(const Evaluator::Data&);
	Evaluator::Data text_width(const Evaluator::Data&);
	Evaluator::Data text_height(const Evaluator::Data&);
	Evaluator::Data title(const Evaluator::Data&);
	Evaluator::Data trigger(const Evaluator::Data&);
	Evaluator::Data untap(const Evaluator::Data&);
	Evaluator::Data untapped(const Evaluator::Data&);
	Evaluator::Data w(const Evaluator::Data&);
	Evaluator::Data load_sound(const Evaluator::Data&);
	Evaluator::Data play_sound(const Evaluator::Data&);
	Evaluator::Data card_sound(const Evaluator::Data&);

	//  Faster implementations of some script language functions
	//  --------------------------------------------------------

	Evaluator::Data InputSplit(const Evaluator::Data&);

    public:

	/// Create table object.
	Table(const string& triggerfile1,bool fullscreen,bool debug,bool fulldebug,bool nographics,int scrw,int scrh);
	///  Desctructor.
	~Table();

	/// Print ASCII presentation of this object.
	void Dump() const;

	/// Define all functions of the parser.
	void InitializeLibrary();
	/// Execute given code.
	Evaluator::Data Execute(const string &code);
	/// Main loop of the client.
	void Main(const string& server,int port,string username="");

	/// Query functions
	/// ---------------

	/// Return size percentage of play cards.
	int TableCardSize()
	{return table_card_size;}
	/// Return size percentage of hand cards.
	int HandCardSize()
	{return hand_card_size;}
	/// Return size percentage of deck cards.
	int DeckCardSize()
	{return deck_card_size;}
	/// Return size percentage of collection cards.
	int BookCardSize()
	{return book_card_size;}

	// Coordinate conversions
	// ----------------------

	/// Convert table coordinates to physical screen coordinates.
	void Table2Screen(int& x,int& y) const;
	/// Convert physical screen coordinates to table coordinates.
	void Screen2Table(int& x,int& y) const;
	/// Scale width to the physical width in pixels.
	void Width2Screen(int& w) const;
	/// Short cut to width conversion.
	int W(int w) const
	{ Width2Screen(w); return w;}
	/// Scale heigth to the physical height in pixels.
	void Height2Screen(int& h) const;
	/// Short cut to height conversion.
	int H(int h) const
	{ Height2Screen(h); return h;}
	/// Override default design w/h
	static void SetDesignWidthHeight(int w, int h) { design_width = w; design_height = h; }
#ifdef USE_SQUIRREL

    // Squirrel proxy helper
    // ---------------------

    // Needed for the Squirrel interface.
    class TableProxyHelper : public Evaluator::Libsquirrel::SQProxyHelper
    {
    private:
        // Hopefully the parser will stay for the duration of the program.
        Evaluator::Parser<Table> *parser_;
    public:
        TableProxyHelper(Evaluator::Parser<Table> *parser)
        {
            parser_ = parser;
        }

        Evaluator::Data call(Evaluator::Data& arg)
        {
            return parser_->call(arg);
        }
    };
    Evaluator::Libsquirrel::SQProxyHelper* make_sq_proxy_helper()
    {
        TableProxyHelper *ph = new TableProxyHelper(&parser); 
        return ph;
    }
#endif
    };
#ifndef USE_SQUIRREL
	
#endif
}

#endif
