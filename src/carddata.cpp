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

#include "carddata.h"
#include "localization.h"
#include "error.h"
#include "tools.h"
#include <algorithm>
#include <stdlib.h>
#include <set>

using namespace Database;

// Global variables
// ================

CardSet Database::cards;
Game Database::game;

// Class CardSet
// =============

CardSet::CardSet()
{
    nextcard=0;
}

void CardSet::Validate(XML::Document& D)
{
    string name=(*D.Base())["name"];
    string abbrev=(*D.Base())["abbrev"];
    string dir=(*D.Base())["dir"];

    age[abbrev]=sets.size();
    if((*D.Base())["age"]!="")
    {
	age[abbrev]=atoi((*D.Base())["age"].c_str());
	for(list<string>::iterator i=sets.begin(); i!=sets.end(); i++)
	    if(age[*i]>=age[abbrev])
		age[*i]++;
    }
	
    sets.push_back(abbrev);
	
    set_name[abbrev]=name;
    directory[abbrev]=dir;
    list<XML::Element*>::iterator i;
    list<XML::Element*> cards=D("cards","card");
    first_card[abbrev]=nextcard;

    for(i=cards.begin(); i!=cards.end(); i++)
    {
	(**i).AddAttribute("set",abbrev);
	numbers[(**i)["name"]].push_back(nextcard);
	nextcard++;
    }
    last_card[abbrev]=nextcard-1;
}
	
void CardSet::AddCards(const string& filename)
{
    int old_nextcard=nextcard;
	
    if(db.Base()==0)
    {
	db.ReadFile(Localization::File(filename));
	Validate(db);
	db.Base()->DelAttribute("name");
    }
    else
    {
	XML::Document add;

	add.ReadFile(Localization::File(filename));
	Validate(add);
	db("cards").front()->AddSubelements(add("cards","card"));
    }

    list<XML::Element*>::const_iterator i;
    card=vector<XML::Element*>(nextcard);
    list<XML::Element*>& C=db("cards").front()->Subs();
    i=C.begin();
    for(int j=0; j<nextcard; j++)
	card[j]=*i++;

    for(int j=old_nextcard; j<nextcard; j++)
    {
	string rarity=AttrValue(j,"rarity");
	while(rarity!="")
	{
	    size_t k;
	    for(k=0; k<rarity.length(); k++)
	      if((rarity[k]>='0' && rarity[k]<='9') || (rarity[k]=='+' && (k+1<rarity.length())))
		    break;

	    rarities.insert(rarity.substr(0,k));
	    while(k<rarity.length() && rarity[k]!='+')
		k++;

	    if(k+1<rarity.length())
		rarity=rarity.substr(k+1,rarity.length());
	    else
		rarity="";
	}

	list<string> att=Attributes(j);
	for(list<string>::iterator n=att.begin(); n!=att.end(); n++)
	    attributes.insert(*n);
    }
}

string CardSet::ImageFile(int cardnumber) const
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::ImageFile(int)","invalid cardnumber "+ToString(cardnumber));

    return (*card[cardnumber])["graphics"];
}
	
string CardSet::Name(int cardnumber) const
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::Name(int)","invalid cardnumber "+ToString(cardnumber));

    return (*card[cardnumber])["name"];
}

list<string> CardSet::Rarities(int cardnumber) const
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::Rarities(int)","invalid cardnumber "+ToString(cardnumber));
	
    string rarity=AttrValue(cardnumber,"rarity");
    list<string> ret;
	
    while(rarity!="")
    {
	size_t k;
	for(k=0; k<rarity.length(); k++)
	    if(rarity[k]=='+' && (k+1<rarity.length()))	// Split only if not last character
		break;
		
	ret.push_back(rarity.substr(0,k));

	if(k+1<rarity.length())
	    rarity=rarity.substr(k+1,rarity.length());
	else
	    rarity="";
    }

    return ret;
}

/// Helper function for CanonicalName(). Remove cards based
/// on a set.
static bool CanonicalName_select_set(list<int>& crds,const string& set)
{
    bool change=false;
			
    for(list<int>::iterator i=crds.begin(); i!=crds.end();)
    {
	if(cards.Set(*i) != set)
	{
	    i=crds.erase(i);
	    change=true;
	}
	else
	    i++;
    }

    return change;
}

/// Helper function for CanonicalName(). Remove cards based
/// on an attribute.
static bool CanonicalName_select_attr(list<int>& crds,const string& attr,const string& value,int len)
{
    bool change=false;

    for(list<int>::iterator i=crds.begin(); i!=crds.end();)
    {
	if(cards.AttrValue(*i,attr).substr(0,len) != value.substr(0,len))
	{
	    i=crds.erase(i);
	    change=true;
	}
	else
	    i++;
    }

    return change;
}

/*/// Helper function for CanonicalName(). Remove cards with
/// the same attributes belonging to the same set.
static void CanonicalName_remove_clones(list<int>& crds,int crd)
{
    list<string> attr=cards.Attributes(crd);
			
    for(list<int>::iterator i=crds.begin(); i!=crds.end();)
    {
	bool remove=(*i != crd);
				
	if(*i != crd)
	{
	    if(cards.Set(*i)!=cards.Set(crd))
		remove=false;
	    else for(list<string>::iterator j=attr.begin(); j!=attr.end(); j++)
	    {
		if(*j != "rarity" && cards.AttrValue(*i,*j) != cards.AttrValue(crd,*j))
		    remove=false;
	    }					
	}

	if(remove)
	    i=crds.erase(i);
	else
	    i++;
    }
} */

/// Helper function for CanonicalName(). Try to extract set name
/// abbreviation from the end of the string. Return empty string of
/// not success, the set abbreviation if success.
static string SetSpecifier(const string& cardname)
{
    string ret;
	size_t i=cardname.length()>0?cardname.length()-1:0;

    if(i>0)
    {
	if(cardname[i]==')')
	{
	    i--;
	    while(i)
	    {
		if(cardname[i]=='(')
		{
		    ret=cardname.substr(i+1,cardname.length()-i-2);
		    break;
		}

		if(!isalnum(cardname[i]))
		    break;
				
		i--;
	    }
	}
    }
	
    return ret;
}

string CardSet::CanonicalName(int crd) const
{
    if(crd < 0 || crd >= nextcard)
	throw Error::Range("CardSet::CanonicalName(int)","invalid cardnumber "+ToString(crd));

    string name=cards.Name(crd);
    list<int> crds=cards.Images(name);

    if(crds.size()==0)
	return "";
    if(crds.size()==1)
	return name;

    // Hmm. This doesn't make sense.
    //    CanonicalName_remove_clones(crds,crd);

    string key=game.Option("canonical key");

    if(key != "")
    {
	int len=1;
	if(game.Option("canonical key length")!="")
	    len=atoi(game.Option("canonical key length").c_str());
				
	if(CanonicalName_select_attr(crds,key,cards.AttrValue(crd,key),len))
	    name+=" ["+cards.AttrValue(crd,key).substr(0,len)+"]";
	if(crds.size()==1)
	    return name;
    }

    if(CanonicalName_select_set(crds,cards.Set(crd)))
	name+=" ("+cards.Set(crd)+")";			
    if(crds.size()==1)
	return name;


    crds.sort();
    int n=1;
    for(list<int>::iterator i=crds.begin(); i!=crds.end(); i++)
	if(*i==crd)
	    break;
	else
	    n++;

    name+=" #"+ToString(n);
	
    return name;
}

int CardSet::Back(int cardnumber) const
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::Back(int)","invalid cardnumber "+ToString(cardnumber));
    if((*card[cardnumber])["back"]!="")
	return atoi((*card[cardnumber])["back"].c_str());

    return 0;
}

int CardSet::Front(int cardnumber) const
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::Front(int)","invalid cardnumber "+ToString(cardnumber));
    if((*card[cardnumber])["front"]!="")
	return atoi((*card[cardnumber])["front"].c_str());

    return 0;
}

bool CardSet::IsCard(int cardnumber) const
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::IsCard(int)","invalid cardnumber "+ToString(cardnumber));

    return Set(cardnumber)!="" && atoi((*card[cardnumber])["hidden"].c_str())==0;
}

bool CardSet::IsSet(const string& s) const
{
    return set_name.find(s)!=set_name.end();
}

string CardSet::Set(int cardnumber) const
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::Set(int)","invalid cardnumber "+ToString(cardnumber));

    return (*card[cardnumber])["set"];
}

int CardSet::Age(int cardnumber)
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::Age(int)","invalid cardnumber "+ToString(cardnumber));

    return age[Set(cardnumber)];
}

string CardSet::Text(int cardnumber) const
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::Text(int)","invalid cardnumber "+ToString(cardnumber));

    return (*card[cardnumber])["text"];
}

list<string> CardSet::Attributes(int cardnumber) const
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::Attributes(int)","invalid cardnumber "+ToString(cardnumber));
			
    list<string> ret;
    list<XML::Element*>& attr=card[cardnumber]->Subs();
    list<XML::Element*>::const_iterator i;
    for(i=attr.begin(); i!=attr.end(); i++)
	ret.push_back((**i)["key"]);

    return ret;
}

string CardSet::AttrValue(int cardnumber,const string& a) const
{
    if(cardnumber < 0 || cardnumber >= nextcard)
	throw Error::Range("CardSet::AttrValue(int,const string)","invalid cardnumber "+ToString(cardnumber));

    if(card[cardnumber]->HasSubAttr("attr","key",a))
	return (*card[cardnumber]->NthSubWithAttr(0,"attr","key",a))["value"];
    else
	return "";
}

/// Check if string s2 is contained in s1 ignoring spaces and accents.
static bool FuzzyMatch(const string& s1,const string& s2)
{
    string r1,r2;

    r1=Fuzzify(s1,true);
    r2=Fuzzify(s2,true);

    return r1==r2 || r1.find(r2)!=string::npos;
}

list<int> CardSet::Images(const string& s,bool fuzzymatch)
{
    string name=s;
	
    while(name.length() && IsSpace(name[0]))
	name=name.substr(1,name.length()-1);

    while(name.length() && IsSpace(name[name.length()-1]))
	name=name.substr(0,name.length()-1);

    list<int> ret;
	
    if(fuzzymatch)
    {
	if(numbers[s].size())
	    ret=numbers[s];
	else
	{
	    for(int i=0; i<Cards(); i++)
		if(FuzzyMatch(Name(i),s))
		    ret.push_back(i);

	    if(ret.size())
	    {
		list<int> better;
		for(list<int>::iterator i=ret.begin(); i!=ret.end(); i++)
		    if(Fuzzify(Name(*i))==Fuzzify(s))
			better.push_back(*i);

		if(better.size())
		    return better;

		return ret;
	    }
	}
    }
    else
	ret=numbers[s];

    // Try to strip set and attribute specifiers.
    if(ret.size()==0)
    {		
	string set;
	string val;

	set=SetSpecifier(s);
	if(set != "")
	{
	    name=name.substr(0,s.length()-set.length()-3);
	}

	int len=1;
	if(game.Option("canonical key length")!="")
	    len=atoi(game.Option("canonical key length").c_str());


	if((int)name.length() >= len+3 && name[name.length()-1]==']' && name[name.length()-2-len]=='[')
	{
	    val=name.substr(name.length()-1-len,len);
	    name=name.substr(0,name.length()-len-3);
	}

	if(name != s)
	    ret=Images(name,fuzzymatch);

	if(set!="")
	    CanonicalName_select_set(ret,set);
	if(val!="")
	    CanonicalName_select_attr(ret,game.Option("canonical key"),val,len);
    }

    // Try to strip #<n> notation.
    if(ret.size()==0)
    {
	name=s;
	int i=name.length()-1;
	while(i>=0)
	{
	    if(name[i]<'0' || name[i]>'9')
		break;
	    i--;
	}
	if(i < int(name.length())-1)
	{
	    if(name[i]=='#')
	    {
		i--;
		if(i > 0 && name[i]==' ')
		{
		    int ord=atoi(name.substr(i+2,name.length()-(i+2)).c_str()) - 1;
		    name=name.substr(0,i);
		    ret=Images(name,fuzzymatch);
		    if(ret.size())
		    {
			if(ord < 0 || ord >=(int)ret.size())
			    return list<int>();
						
			ret.sort();

			list<int> ret2;
			list<int>::iterator j=ret.begin();
			while(ord--)
			    j++;
			ret2.push_back(*j);

			return ret2;
		    }
		}
	    }
	}
    }
	
    return ret;
}

// Class Game
// ==========

void Game::ReadFile(const string& filename)
{
    db.ReadFile(Localization::File(filename));

    list<XML::Element*> L=db.Base()->Subs("option");
    list<XML::Element*>::iterator i;

    for(i=L.begin(); i!=L.end(); i++)
	options[(**i)["name"]]=(**i)["value"];
}

int Game::Phases() const
{
    if(db.Base()==0)
	throw Error::Invalid("Game::Phases()","Game info not loaded");

    return db.Base()->SubElements("phase");
}

string Game::Phase(int num) const
{
    if(db.Base()==0)
	throw Error::Invalid("Game::Phase(int)","Game info not loaded");

    return (*db.Base()->NthSub(num,"phase"))["name"];
}

string Game::Gamedir() const
{
    if(db.Base()==0)
	throw Error::Invalid("Game::Gamedir()","Game info not loaded");

    return (*db.Base())["dir"];
}

string Game::Name() const
{
    if(db.Base()==0)
	throw Error::Invalid("Game::Name()","Game info not loaded");

    return (*db.Base())["name"];
}

int Game::Triggers(const string& name)
{
    if(db.Base()==0)
	throw Error::Invalid("Game::Triggers(const string&)","Game info not loaded");

    return db.Base()->SubsWithAttr("trigger","event",name).size();
}

string Game::Trigger(int n,const string& name)
{
    if(db.Base()==0)
	throw Error::Invalid("Game::Trigger(int,const string&)","Game info not loaded");

    return (*db.Base()->NthSubWithAttr(n,"trigger","event",name))["attribute"];
}

string Game::Option(const string& option)
{
    map<string,string>::iterator i;
    i=options.find(option);
    if(i==options.end())
	return "";
	
    return (*i).second;
}

int Game::CardSets() const
{
    if(db.Base()==0)
	throw Error::Invalid("Game::CardSets()","Game info not loaded");

    return db.Base()->Subs("cardset").size();
}

string Game::CardSet(int num) const
{
    if(db.Base()==0)
	throw Error::Invalid("Game::CardSet(int)","Game info not loaded");

    return (*db.Base()->NthSub(num,"cardset"))["source"];
}

