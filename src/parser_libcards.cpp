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

#include <sys/types.h>
#if defined(_MSC_VER)
# include "compat.h"
#else
# include <dirent.h>
#endif
#include <ctype.h>
#include <algorithm>
#include "tools.h"
#include "parser.h"
#include "carddata.h"
#include "parser_functions.h"

using namespace Database;

namespace Evaluator
{
    namespace Libcards
    {
	/// attrs(c) - Return the sorted list of pairs containg
	/// attribute names and values of the cards number $c$. This
	/// list can be used as dictionary.
	Data attrs(const Data& args)
	{
	    VECTORIZE(Libcards::attrs);

	    if(!args.IsInteger())
		ArgumentError("attrs",args);

	    int crd=args.Integer();
	    Data pair;
	    pair.MakeList(2);

	    if(crd < 0 || crd >= cards.Cards())
		return Null;
	    else
	    {
		Data ret;
		list<string> attr=cards.Attributes(crd);
		ret.MakeList(attr.size());
		list<string>::iterator j=attr.begin();
		for(size_t i=0; i<attr.size(); i++)
		{
		    pair[0]=*j;
		    pair[1]=cards.AttrValue(crd,*j);
		    ret[i]=pair;
		    j++;
		}

		return sort(ret);
	    }
	}

	/// text(n) - Card text of the card number $n$.
	Data text(const Data& args)
	{
	    VECTORIZE(Libcards::text);
	
	    if(!args.IsInteger())
		ArgumentError("text",args);
	    int crd=args.Integer();
		
	    if(crd < 0 || crd >= cards.Cards())
		return Null;
	    else
		return cards.Text(crd);
	}

	/// name(n) - Name of the card number $n$.
	Data name(const Data& args)
	{
	    VECTORIZE(Libcards::name);

	    if(!args.IsInteger())
		ArgumentError("name",args);
	    int crd=args.Integer();
		
	    if(crd < 0 || crd >= cards.Cards())
		return Null;
	    else
		return cards.Name(crd);
	}
		
	/// canonical_name(n) - Unique name of a card number $n$,
	/// which identifies the card by adding a set name and key
	/// attribute if otherwise undistinguishable. However, cards
	/// with the same properties and the same name have also the
	/// same canonical name if both belong in the same set.	
	Data canonical_name(const Data& args)
	{
	    VECTORIZE(Libcards::canonical_name);

	    if(!args.IsInteger())
		ArgumentError("canonical_name",args);
			
	    int crd=args.Integer();
		
	    if(crd < 0 || crd >= cards.Cards())
		return Null;
	    else
		return cards.CanonicalName(crd);
	}

	/// card_attr(k,n) - Return card attribute of the card number
	/// $n$ with key $k$. If the cards has {\tt <attr key="}$k${\tt " value="}$v${\tt "/>} 
	/// defined in XML-description, return $v$. Otherwise return
	/// \{\tt NULL\}.
	Data card_attr(const Data& args)
	{
	    VECTORIZE_SECOND(Libcards::card_attr);

	    if(!args.IsList(2) || !args[0].IsString() || !args[1].IsInteger())
		ArgumentError("card_attr",args);

	    int crd=args[1].Integer();

	    if(crd < 0 || crd >= cards.Cards())
		return Null;

	    string s;
	    s=cards.AttrValue(crd,args[0].String());

	    if(s != "")
		return s;
		
	    return Null;
	}

	/// Attr(k,n) - Same as card_attr, but return "" instead of NULL for missing attribute.
	Data Attr(const Data& args)
	{
	    VECTORIZE_SECOND(Libcards::Attr);

	    if(!args.IsList(2) || !args[0].IsString() || !args[1].IsInteger())
		ArgumentError("Attr",args);

	    int crd=args[1].Integer();
			
	    if(crd < 0 || crd >= cards.Cards())
		return Null;

	    return cards.AttrValue(crd,args[0].String());
	}
		
	/// card_back(c) - Return background image number of the
	/// card.
	Data card_back(const Data& args)
	{
	    VECTORIZE(Libcards::card_back);

	    if(args.IsNull())
		return 0;
	    if(!args.IsInteger())
		return Null;

	    int crd=args.Integer();
			
	    if(crd < 0 || crd >= cards.Cards())
		return Null;
	
	    return cards.Back(crd);
	}

	/// card_front(c) - Return front image number of the card if differs.
	Data card_front(const Data& args)
	{
	    VECTORIZE(Libcards::card_back);

	    if(args.IsNull())
		return 0;
	    if(!args.IsInteger())
		return Null;

	    int crd=args.Integer();
			
	    if(crd < 0 || crd >= cards.Cards())
		return Null;
	
	    return cards.Front(crd);
	}

	/// game_data() - Return dictionaty of game data containgin
	/// entries \{\tt "game"\} (name of the game), \{\tt "gamedir"\}
	/// (directory name for the game), \{\tt "play"\} (attribute to call
	/// triggers for when playing a card) and \{\tt "location hint"\}
	/// (attributes to call triggers for when deciding card
	/// placement).
	Data game_data(const Data& args)
	{
	    if(!args.IsNull())
		ArgumentError("game_data",args);

	    int n;
	    Data ret,*p;
	
	    ret.MakeList();

	    p=ret.FindKey(string("game"));
	    (*p)[1]=game.Name();
	
	    p=ret.FindKey(string("play"));
	    (*p)[1].MakeList();
	    p=&(*p)[1];
	    n=game.Triggers("play");
	    for(int i=0; i<n; i++)
		(*p).AddList(game.Trigger(i,"play"));

	    p=ret.FindKey(string("location hint"));
	    (*p)[1].MakeList();
	    p=&(*p)[1];	
	    n=game.Triggers("location hint");
	    for(int i=0; i<n; i++)
		(*p).AddList(game.Trigger(i,"location hint"));

	    p=ret.FindKey(string("gamedir"));
	    (*p)[1]=game.Gamedir();
			
	    return ret;
	}

	/// sets() - List of set abbreviations of loaded card sets.
	Data sets(const Data& args)
	{
	    if(!args.IsNull())
		ArgumentError("sets",args);

	    Data ret;
	    list<string> sets=cards.Sets();
	    list<string>::iterator i;
			
	    ret.MakeList();
	    for(i=sets.begin(); i!=sets.end(); i++)
		if(*i != "")
		    ret.AddList(*i);

	    return ret;
	}

	/// set_of(c) - return set abbreviation of the set where a card
	/// number $c$ belongs to or \{\tt NULL\} if none.
	Data set_of(const Data& args)
	{
	    VECTORIZE(Libcards::set_of);
			
	    if(!args.IsInteger())
		ArgumentError("set_of",args);

	    int crd=args.Integer();
		
	    if(crd < 0 || crd >= cards.Cards())
		return Null;
	    else
		return cards.Set(crd);
	}

	/// set_data(s) - Return a vector $(n,c_1,c_2)$ for card set
	/// with abbreviation $s$. Here $n$ is the name of the set,
	/// $c_1$ is the first card of the set and $c_2$ is the last
	/// card of the set.
	Data set_data(const Data& args)
	{
	    VECTORIZE(Libcards::set_data);

	    if(!args.IsString())
		ArgumentError("set_data",args);

	    Data ret;
	    ret.MakeList(3);
			
	    ret[0]=cards.SetName(args.String());
	    ret[1]=cards.FirstOfSet(args.String());
	    ret[2]=cards.LastOfSet(args.String());

	    return ret;
	}
		
	/// load_deck(s) - If a deck named $s$ is available, load it
	/// and return a list of strings containing each line of the text file.
	/// Return NULL if the deck cannot be loaded.
	Data load_deck(const Data& args)
	{
	    VECTORIZE(Libcards::load_deck);
			
	    if(!args.IsString())
		ArgumentError("load_deck",args);
			
	    string file;

	    file=args.String();
	    if(!FileExist(file))
		return Null;

	    security.ReadFile(file);
			
	    ifstream f(file.c_str());

	    string s;
	    Data ret;
	
	    ret.MakeList();

	    while(f)
	    {
		s=readline(f);
		if(!f)
		{
		    if(s!="")
			ret.AddList(s);
						
		    break;
		}

		ret.AddList(s);
	    }

	    return ret;
	}

	static void find_decks(const string& dir,Data& deck_list)
	{
	    struct dirent *e;
	    DIR* d;
	    string f;

	    security.OpenDir(dir);
	    d=opendir(dir.c_str());
	    if(d)
	    {
		while((e=readdir(d)) != NULL)
		{
		    f=e->d_name;
		    if(f!=".." && f!="." && f!="CVS" && f!=".svn")
		    {
			if(IsDirectory(dir+f))
			    find_decks(dir+f+"/",deck_list);
			else
			    deck_list.AddList(dir+f);
		    }
		}
		closedir(d);
	    }
	}

	/// decks() - Return a list of strings containgin deck names
	/// available or {\tt NULL} if cannot open deck directory.
	Data decks(const Data& args)
	{
	    if(!args.IsNull())
		ArgumentError("decks",args);

	    string file;
	    Data ret;
	    ret.MakeList();

	    // Global deck files.
	    file=CCG_DATADIR;
	    file+="/decks/";
	    file+=game.Gamedir();
	    file+="/";
	    find_decks(file,ret);

	    // Check users own import deck files.
	    file=getenv("HOME");
	    file+="/.gccg/";
	    file+=Database::game.Gamedir();
	    file+="/import/";
	    find_decks(file,ret);

	    // Check users own export deck files.
	    file=getenv("HOME");
	    file+="/.gccg/";
	    file+=Database::game.Gamedir();
	    file+="/export/";
	    find_decks(file,ret);
			
	    return ret;
	}

	/// scripts() - Return a list of strings containing extension
	/// scripts under scripts/Game or scripts/global directory.
	Data scripts(const Data& args)
	{
	    if(!args.IsNull())
		ArgumentError("scripts",args);

	    // Search game-specific directory
	    string file;
	    file=CCG_DATADIR;
	    file+="/scripts/";
	    file+=game.Gamedir();
	    file+="/";

	    Data ret;
	    ret.MakeList();

	    struct dirent *e;
	    DIR* d;
	    string f;

	    security.OpenDir(file);
	    d=opendir(file.c_str());
	    if(d)
	    {
		while((e=readdir(d)) != NULL)
		{
		    f=e->d_name;
		    if(!IsDirectory(file+f))
			ret.AddList(f);
		}
		closedir(d);
	    }
	    
	    // Search global add-ons directory
	    file=CCG_DATADIR;
	    file+="/scripts/global/";

	    security.OpenDir(file);
	    d=opendir(file.c_str());
	    if(d)
	    {
		while((e=readdir(d)) != NULL)
		{
		    f=e->d_name;
		    if(!IsDirectory(file+f))
			ret.AddList(f);
		}
		closedir(d);
	    }
			
	    return ret;
	}

	/// server_scripts() - Return a list of strings containing extension
	/// scripts under scripts/Game-server or scripts/global-server directory.
	Data server_scripts(const Data& args)
	{
	    if(!args.IsNull())
		ArgumentError("server_scripts",args);

	    // Search game-specific directory
	    string file;
	    file=CCG_DATADIR;
	    file+="/scripts/";
	    file+=game.Gamedir();
	    file+="-server/";

	    Data ret;
	    ret.MakeList();

	    struct dirent *e;
	    DIR* d;
	    string f;

	    security.OpenDir(file);
	    d=opendir(file.c_str());
	    if(d)
	    {
		while((e=readdir(d)) != NULL)
		{
		    f=e->d_name;
		    if(!IsDirectory(file+f))
			ret.AddList(f);
		}
		closedir(d);
	    }
	    
	    // Search global add-ons directory
	    file=CCG_DATADIR;
	    file+="/scripts/global-server/";

	    security.OpenDir(file);
	    d=opendir(file.c_str());
	    if(d)
	    {
		while((e=readdir(d)) != NULL)
		{
		    f=e->d_name;
		    if(!IsDirectory(file+f))
			ret.AddList(f);
		}
		closedir(d);
	    }
			
	    return ret;
	}
		
	/// images(s) - Return a list of image numbers, which has a
	/// card name $s$.
	Data images(const Data& args)
	{
	    VECTORIZE(Libcards::images);
			
	    if(!args.IsString())
		return Null;
	    string name=args.String();
			
	    list<int> crds=cards.Images(name);
	    Data ret;

	    ret.MakeList();
	    list<int>::iterator i;
	    for(i=crds.begin(); i!=crds.end(); i++)
		ret.AddList(Data(*i));
			
	    return ret;
	}

	/// fuzzy_images(s) - Return a list of image numbers, which
	/// has almost the card name $s$.
	Data fuzzy_images(const Data& args)
	{
	    VECTORIZE(Libcards::fuzzy_images);
			
	    if(!args.IsString())
		return Null;
	    string name=args.String();
			
	    list<int> crds=cards.Images(name,false);
	    if(crds.size()==0)
		crds=cards.Images(name,true);
			
	    Data ret;

	    ret.MakeList(crds.size());
	    list<int>::iterator i;
	    int j;
	    for(j=0,i=crds.begin(); i!=crds.end(); i++,j++)
		ret[j]=Data(*i);
			
	    return ret;
	}

	// Sorting criteria functions.

	class SortFunction
	{
	public:
	    virtual ~SortFunction()
	    {}
	    virtual string operator()(int card)=0;
	    virtual string Title(int card)
	    {return operator()(card);}
	};
		
	class SortByAttrFn : public SortFunction
	{
	    string attribute;
			
	public:
			
	    SortByAttrFn(const string& attribute_)
	    {attribute=attribute_;}
	    virtual ~SortByAttrFn()
	    {}
			
	    string operator()(int card)
	    {
		return Database::cards.AttrValue(card,attribute);
	    }
	};

	class SortByNameFn : public SortFunction
	{			
	public:

	    SortByNameFn()
	    {}
	    virtual ~SortByNameFn()
	    {}
			
	    string operator()(int card)
	    {
		return Database::cards.Name(card);
	    }
	};

	class SortBySetFn : public SortFunction
	{			
	public:
			
	    SortBySetFn()
	    {}
	    virtual ~SortBySetFn()
	    {}
			
	    string operator()(int card)
	    {
		return Database::cards.SetName(Database::cards.Set(card));
	    }
	};

	class SortByAgeFn : public SortFunction
	{			
	public:
			
	    SortByAgeFn()
	    {}
	    virtual ~SortByAgeFn()
	    {}
			
	    string operator()(int card)
	    {
		string s="0000000000"+ToString(Database::cards.Age(card));
				
		return s.substr(s.length()-7,7);
	    }

	    string Title(int card)
	    {
		return Database::cards.SetName(Database::cards.Set(card));
	    }
	};
		
	struct SortOperator : public binary_function<int, int, bool>
	{
	    list<SortFunction*> functions;
	    list<bool> numerical;
	    list<bool> reverse;
	    list<bool> case_insensitive;

	    ~SortOperator()
	    {
	    }
			
	    void AddFunction(SortFunction* fn,bool num,bool rev,bool caseins)
	    {
		functions.push_back(fn);
		numerical.push_back(num);
		reverse.push_back(rev);
		case_insensitive.push_back(caseins);
	    }

	    void FreeAllFunctions()
	    {
		list<SortFunction*>::iterator i;
				
		for(i=functions.begin(); i!=functions.end(); i++)
		{
		    SortFunction* fn=(*i);
		    delete fn;
		}
	    }
			
	    bool operator()(int x, int y)
	    {
		string xval,yval;
				
		list<SortFunction*>::iterator i;
		list<bool>::iterator num=numerical.begin();
		list<bool>::iterator rev=reverse.begin();
		list<bool>::iterator casin=case_insensitive.begin();
		SortFunction* fn;
                int ret;

		for(i=functions.begin(); i!=functions.end(); i++)
		{
		    fn=*i;
		    xval=(*fn)(x);
		    yval=(*fn)(y);
                    ret=0;

                    if(*num)
                    {
                        if(atoi(xval.c_str()) < atoi(yval.c_str()))
                            ret=1;
                        if(atoi(xval.c_str()) > atoi(yval.c_str()))
                            ret=-1;
                    }
                    else if(*casin)
                    {
                        if(ToLower(xval) < ToLower(yval))
                            ret=1;
                        if(ToLower(xval) > ToLower(yval))
                            ret=-1;
                    }
                    else
                    {
                        if(xval < yval)
                            ret=1;
                        if(xval > yval)
                            ret=-1;
                    }

                    if(*rev)
                        ret=-ret;

                    if(ret)
                        return ret > 0;

		    num++;
		    rev++;
		    casin++;
		}
				
		return x < y;
	    }
	};

	/// Create a function evaluating an filter expression.
	static SortFunction* CreateSortFunction(const string& _exp,bool& numerical,bool &reverse,bool &casein)
	{
	    string exp=_exp;

	    numerical=false;
	    reverse=false;
            casein=false;
			
	    while(exp.length() && exp[0]!='[')
	    {
		if(exp.length() && exp[0]=='-')
		{
		    exp=exp.substr(1);
		    reverse=true;
		}
		else if(exp.length() && exp[0]=='N')
		{
		    exp=exp.substr(1);
		    numerical=true;
		}
		else if(exp.length() && exp[0]=='C')
		{
		    exp=exp.substr(1);
		    casein=true;
		}
		else
		    throw LangErr("sort_by","invalid criteria modifier '"+exp.substr(0,1)+"'");
	    }
			
	    if(exp=="[name]")
		return new SortByNameFn();
	    else if(exp=="[set]")
		return new SortBySetFn();
	    else if(exp=="[age]")
		return new SortByAgeFn();
	    else if(exp.length() && exp[0]=='[' && exp[exp.length()-1]==']')
		return new SortByAttrFn(exp.substr(1,exp.length()-2));

	    throw LangErr("sort_by","invalid sort criteria '"+exp+"'");
	}
		
	static SortOperator CreateSortOperator(const Data& args)
	{
	    bool num,rev,casin;
	    SortFunction* fn;
	    SortOperator ret;

	    if(args.IsString())
	    {
		fn=CreateSortFunction(args.String(),num,rev,casin);
		ret.AddFunction(fn,num,rev,casin);
	    }
	    else if(args.IsList())
	    {
		for(size_t i=0; i<args.Size(); i++)
		{
		    if(!args[i].IsString())
			throw LangErr("sort_by","invalid sorting criteria "+tostr(args).String());

		    fn=CreateSortFunction(args[i].String(),num,rev,casin);
		    ret.AddFunction(fn,num,rev,casin);
		}
	    }
	    else
		throw LangErr("sort_by","invalid sorting criteria "+tostr(args).String());
			
	    return ret;
	}
		
	/// sort_by(k,L,t) - Sort a list of card numbers $L$ according
	/// to the key $k$. If $k$ is a list, then the first list member is used
	/// as a primary key, second as a secundary key and so on.
	/// Each key can be a string '[$att$]', where $attr$ is a card
	/// attribute. Then that attribute of the card is used as a
	/// sorting key. Special attribute "[name]" sorts according
	/// to card name and "[set]" accorfing to the card set.
	/// The key can be prepended by a character 'N' to force
	/// numerical sorting and by a character '-' to force reverse sorting.
        /// If key is prepended by a character 'C', sort is case insensitive.
	/// If an integer $t > 0$ is given, add title entries to the list.
	Data sort_by(const Data& args)
	{
	    int title_level=0;
			
	    if(!args.IsList(2) && !args.IsList(3))
		ArgumentError("sort_by",args);
	    if(!args[0].IsString() && !args[0].IsList())
		throw LangErr("sort_by","invalid sort criteria "+tostr(args[0]).String());
	    if(!args[1].IsList())
		throw LangErr("sort_by","invalid card list "+tostr(args[1]).String());
	    if(args.IsList(3))
	    {
		if(!args[2].IsInteger())
		    throw LangErr("sort_by","title depth "+tostr(args[2]).String()+" must be integer");
		title_level=args[2].Integer();
		if(title_level < 0 || (args[0].IsString() && title_level > 1) || (args[0].IsList() && title_level > (int)args[0].Size()))
		    throw LangErr("sort_by","invalid title depth "+ToString(title_level));
					
	    }
	    if(args[0].IsList() && args[0].Size()==0)
		throw LangErr("sort_by","empty sort criteria list");
				
	    Data L=args[1];

	    if(L.Size()==0)
		return L;
			
	    // Check validity of the content of the list and initialize list.
	    vector<int> index;
	    index.resize(L.Size());
	    int c;
	    for(size_t i=0; i<L.Size(); i++)
	    {
		if(!L[i].IsInteger())
		    ArgumentError("sort_by",args);
		c=L[i].Integer();
		if(c < 0 || c >= cards.Cards())
		    throw LangErr("sort_by","invalid card number "+ToString(c));
				
		index[i]=L[i].Integer();
	    }

	    // Sort
	    SortOperator sortop;

	    sortop=CreateSortOperator(args[0]);
	    sort(index.begin(),index.end(),sortop);
	    sortop.FreeAllFunctions();
			
	    // Construct index
	    string title,addtitle;
			
	    if(title_level<=0)
		for(size_t i=0; i<L.Size(); i++)
		    L[i]=Data(index[i]);
	    else
	    {
		L.MakeList();
		string prev_title;
		vector<SortFunction*> titlefn(title_level);

		// Create title functions.
		for(int j=0; j<title_level; j++)
		{
		    bool dummy1,dummy2,dummy3;
					
		    if(args[0].IsString())
			titlefn[j]=CreateSortFunction(args[0].String(),dummy1,dummy2,dummy3);
		    else
			titlefn[j]=CreateSortFunction(args[0][j].String(),dummy1,dummy2,dummy3);
		}
				
		for(size_t i=0; i<index.size(); i++)
		{
		    // Compute title.
		    title="";
		    for(int j=0; j<title_level; j++)
		    {
			addtitle=titlefn[j]->Title(index[i]);

			if(j==0)
			{
			    if(addtitle=="")
				addtitle="None";
			}
			else
			{
			    if(addtitle!="")
				title+=" | ";
			}

			title+=addtitle;
		    }

		    // Add title and element.
		    if(i==0 || title!=prev_title)
		    {
			L.AddList(title);
			prev_title=title;
		    }
					
		    L.AddList(index[i]);
		}

		// Release title functions.
		for(int j=0; j<title_level; j++)
		    delete titlefn[j];
	    }
			
	    return L;
	}

	/// game_option(o) - Return the value of an option $o$ defined in game.xml file.
	Data game_option(const Data& args)
	{
	    VECTORIZE(Libcards::game_option);
			
	    if(!args.IsString())
		ArgumentError("game_option",args);
			
	    return game.Option(args.String());
	}

	/// inline_images() - Return the list containing inline image tags.
	Data inline_images(const Data& args)
	{
	    if(!args.IsNull())
		ArgumentError("inline_images",args);

	    list<XML::Element*> imgs;
	    list<XML::Element*>::iterator i;
			
	    imgs=game.XML().Base()->Subs("image");

	    Data ret;
	    ret.MakeList(imgs.size());
	    int j=0;
	    for(i=imgs.begin(); i!=imgs.end(); i++)
		ret[j++]=(**i)["tag"];
			
	    return ret;
	}

	/// rarities() - Return the list containing all rarities.
	Data rarities(const Data& args)
	{
	    if(!args.IsNull())
		ArgumentError("rarities",args);

	    set<string> rarities=cards.Rarities();
	    set<string>::const_iterator i;
	    Data ret;

	    int j=0;
	    ret.MakeList(rarities.size());
	    for(i=rarities.begin(); i!=rarities.end(); i++)
		ret[j++]=*i;

	    return ret;
	}

	/// attributes() - Return the list containing all attributes.
	Data attributes(const Data& args)
	{
	    if(!args.IsNull())
		ArgumentError("attributes",args);

	    set<string> attributes=cards.Attributes();
	    set<string>::const_iterator i;
	    Data ret;

	    int j=0;
	    ret.MakeList(attributes.size());
	    for(i=attributes.begin(); i!=attributes.end(); i++)
		ret[j++]=*i;

	    return ret;
	}

	/// rules() - Return the list containing all rule files for this game.
	Data rules(const Data& args)
	{
	    if(!args.IsNull())
		ArgumentError("rules",args);

	    Data ret;
	    DIR* d;	
	    struct dirent *e;
	    string f;
	    ret.MakeList();
	    string gamedir=game.Gamedir();
			
	    security.OpenDir(CCG_DATADIR"/scripts/");
	    d=opendir(CCG_DATADIR"/scripts/");
	    if(d)
	    {
		while((e=readdir(d)) != NULL)
		{
		    f=e->d_name;

		    if(f.length() > 5 && f.length() > gamedir.length() && f.substr(0,gamedir.length())==gamedir && f.substr(f.length()-6,6)==".rules")
		    {
			ret.AddList(f);
		    }
		}
		closedir(d);

		return ret;
	    }

	    return Null;
	}

	/// cards([set]) - Return the list of all non-special card
	/// numbers if no arguments given. Otherwise, return the card
	/// numbers from the given set.
	Data cards(const Data& args)
	{
	    VECTORIZE(Libcards::cards);
			
	    if(args.IsNull())
	    {			
		Data ret;
		ret.MakeList();
			
		list<string> sets=::cards.Sets();
		for(list<string>::iterator i=sets.begin(); i!=sets.end(); i++)
		{
		    int n1=::cards.FirstOfSet(*i);
		    int n2=::cards.LastOfSet(*i);
		    for(int j=n1; j<=n2; j++)
			if(::cards.IsCard(j))
			    ret.AddList(j);
		}

		return ret;
	    }
	    else if(args.IsString())
	    {
		Data ret;
		ret.MakeList();
			
		string set=args.String();
		int n1=::cards.FirstOfSet(set);
		int n2=::cards.LastOfSet(set);
		for(int j=n1; j<=n2; j++)
		    if(::cards.IsCard(j))
			ret.AddList(j);

		return ret;
	    }
	    else
		ArgumentError("cards",args);

	    return 0;
	}

	/// having_rarity(rarity, list of cards) - Return weighted
	/// list of cards having the given rarity. For example, if a
	/// card have rarity 'CB+CS2' and we ask for rarity 'CS' then
	/// this function returns a list having two occurrences of the
	/// card.
	Data having_rarity(const Data& args)
	{
	    if(!args.IsList(2) || !args[0].IsString() || !args[1].IsList())
		ArgumentError("having_rarity",args);
				
	    Data ret;
	    ret.MakeList();
	    int crd,n;
	    string r;
	    string rarity=args[0].String();
	    list<string> rarities;
	    list<string>::iterator j;
			
	    for(size_t i=0; i<args[1].Size(); i++)
	    {
		if(!args[1][i].IsInteger())
		    throw LangErr("having_rarity","invalid card list entry "+tostr(args[1][i]).String());
		crd=args[1][i].Integer();
		if(crd < 0 || crd >= ::cards.Cards())
		    throw LangErr("having_rarity","invalid card number "+ToString(crd));

		rarities=::cards.Rarities(crd);
		for(j=rarities.begin(); j!=rarities.end(); j++)
		{
		    r=*j;
		    if(rarity==r)
			ret.AddList(crd);
		    else if(rarity==r.substr(0,rarity.length()))
		    {
			n=atoi(r.substr(rarity.length()).c_str());
			while(n)
			{
			    n--;
			    ret.AddList(crd);
			}
		    }
		}
	    }

	    return ret;
	}

	/// age(n) - Card age of the card number $n$ or set named $n$.
	///   Return NULL if $n$ is not a valid card number or set name.
	Data age(const Data& args)
	{
	    VECTORIZE(Libcards::age);
	
	    if(args.IsString())
	    {
		const string& abbrev=args.String();
		if(Database::cards.IsSet(abbrev))
		    return Database::cards.Age(Database::cards.FirstOfSet(abbrev));
		else
		    return Null;
	    }
	    if(!args.IsInteger())
		ArgumentError("age",args);

	    int crd=args.Integer();
		
	    if(crd < 0 || crd >= Database::cards.Cards())
		return Null;
	    else
		return Database::cards.Age(crd);
	}
    }



    void InitializeLibcards()
    {
	external_function["age"]=&Libcards::age;
	external_function["Attr"]=&Libcards::Attr;
	external_function["attrs"]=&Libcards::attrs;
	external_function["attributes"]=&Libcards::attributes;
	external_function["cards"]=&Libcards::cards;
	external_function["canonical_name"]=&Libcards::canonical_name;
	external_function["card_attr"]=&Libcards::card_attr;
	external_function["card_back"]=&Libcards::card_back;
	external_function["card_front"]=&Libcards::card_front;
	external_function["decks"]=&Libcards::decks;
	external_function["game_data"]=&Libcards::game_data;
	external_function["game_option"]=&Libcards::game_option;
	external_function["having_rarity"]=&Libcards::having_rarity;
	external_function["images"]=&Libcards::images;
	external_function["inline_images"]=&Libcards::inline_images;
	external_function["fuzzy_images"]=&Libcards::fuzzy_images;
	external_function["load_deck"]=&Libcards::load_deck;
	external_function["name"]=&Libcards::name;
	external_function["rarities"]=&Libcards::rarities;
	external_function["rules"]=&Libcards::rules;
	external_function["scripts"]=&Libcards::scripts;
	external_function["set_of"]=&Libcards::set_of;
	external_function["sets"]=&Libcards::sets;
	external_function["set_data"]=&Libcards::set_data;
	external_function["sort_by"]=&Libcards::sort_by;
	external_function["text"]=&Libcards::text;
    }
}
