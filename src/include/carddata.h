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

#ifndef CARDDATA_H
#define CARDDATA_H

#include "xml_parser.h"
#include <list>
#include <vector>
#include <map>
#include <set>

using namespace std;

/// Namespace for card database and inquiry functions.
namespace Database
{
	/// Storage for card information.
	class CardSet
	{
		/// XML document tree containing card data.
		XML::Document db;
		/// Quick pointers to card elements.
		vector<XML::Element*> card;
		/// Name lookup table containing number of cards having the given name.
		map<string,list<int> > numbers;
		/// Name lookup table containing mapping from abbreviations to setname.
		map<string,string> set_name;
		/// Name lookup table containing mapping from abbreviations to set directories.
		map<string,string> directory;
		/// List of set abbreviations in xml order.
		list<string> sets;
		/// Order number of the set in historical order.
		map<string,int> age;
		/// Set of rarities.
		set<string> rarities;
		/// Set of all attributes found.
		set<string> attributes;
		/// The first card of the set (indexed by abbreviations)
		map<string,int> first_card;
		/// The last card of the set (indexed by abbreviations)
		map<string,int> last_card;
		/// Next free card id.
		int nextcard;

		/// Add card set: copy 'name' attribute to each image and establish name lookup tables.
		void Validate(XML::Document& D);

		/// Protect copying. It's unusable without proper handler.
		CardSet(const CardSet&)
			{}
		/// Protect copying. It's unusable without proper handler.
		CardSet& operator=(const CardSet&)
			{return *this;}

	  public:

		/// Create an empty data base.
		CardSet();
		/// Add cards of the XML file. Copy 'set' attribute to every image loaded from the set 'name'.
		void AddCards(const string& filename);
		/// How many cards.
		int Cards() const
			{return nextcard;}

        /// Return the filename of the image.
		string ImageFile(int cardnumber) const;
		/// Return all card numbers of cards having the name s.
		list<int> Images(const string& s,bool fuzzymatch=false);
        /// Return card number of the backside image.
		int Back(int cardnumber) const;
        /// Return card number of the frontside image if different from card number.
		int Front(int cardnumber) const;
		/// Return 1 if card is a real card and not a card back, token, etc..
		bool IsCard(int cardnumber) const;
		/// Return 1 if a string is an set abbreviation.
		bool IsSet(const string& s) const;

		/// Return the list of all attribute names.
		set<string> Attributes() const
			{return attributes;}
		/// Return the list of attribute names valid for the card.
		list<string> Attributes(int cardnumber) const;
		/// Return value of attribute a or empty string if not set.
		string AttrValue(int cardnumber,const string& a) const;
		/// Return the name of the card.
		string Name(int cardnumber) const;
		/// Return the canonical name of the card.
		string CanonicalName(int cardnumber) const;
		/// Return the set abbreviation of the card.
		string Set(int cardnumber) const;
		/// Return order number of the set.
		int Age(int cardnumber);
		/// Return text of the card.
		string Text(int cardnumber) const;
		/// Return list of rarities the card have (i.e. split the rarity at '+')
		list<string> Rarities(int cardnumber) const;
		/// Return set directory.
		string SetDirectory(const string& abbrev)
			{return directory[abbrev];}
		/// Return set name.
		string SetName(const string& abbrev)
			{return set_name[abbrev];}
		/// Return all abbreviations of sets currently loaded.
		list<string> Sets() const
			{return sets;}
		/// Return all rarites of cards currently loaded.
		set<string> Rarities() const
			{return rarities;}
		/// Return first card of the set.
		int FirstOfSet(const string& set_abbrev)
			{return first_card[set_abbrev];}
		/// Return last card of the set.
		int LastOfSet(const string& set_abbrev)
			{return last_card[set_abbrev];}
		/// Return number of cards in the set.
		int SetSize(const string& set_abbrev)
			{return last_card[set_abbrev]-first_card[set_abbrev]+1;}

		/// Reference to XML document.
		const XML::Document& XML() const
			{return db;}
	};

	/// Print XML-version of the card database.
	ostream& operator<<(ostream& O,const CardSet& C);

	/// Game specific information: phases, triggers.
	class Game
	{
		/// XML document tree containing data.
		XML::Document db;
		/// Options.
		map<string,string> options;
		
	  public:

		/// Create an empty game info.
		Game()
			{}

		/// Read game info from the file.
		void ReadFile(const string& filename);
		/// How many phases this game have.
		int Phases() const;
		/// Phasename.
		string Phase(int num) const;
		/// How many cardsets to load.
		int CardSets() const;
		/// Name of the card set.
		string CardSet(int num) const;
		/// Number of trigger definition with 'event' attribute 'name'.
		int Triggers(const string& name);
		/// Nth trigger definition with 'event' attribute 'name'.
		string Trigger(int n,const string& name);
		/// Return gamedirectory.
		string Gamedir() const;
		/// Return name of the game.
		string Name() const;
		/// Return value of the option.
		string Option(const string& option);
		
		/// Reference to XML document.
		const XML::Document& XML() const
			{return db;}

                /// Check if data has been loaded.
                bool Ready()
                    {return db.Base()==0 ? false : true;}
	};

	/// Print XML-version of the game info.
	ostream& operator<<(ostream& O,const Game& G);
	
	/// Global card database.
	extern CardSet cards;
	/// Global game information.
	extern Game game;
}

#endif
