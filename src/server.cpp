/*
    Gccg - Generic collectible card game.
    Copyright (C) 2001-2013 Tommi Ronkainen

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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#if !defined(__BCPLUSPLUS__) && !defined(_MSC_VER)
# include <unistd.h>
#elif !defined(_MSC_VER)
# include <dir.h>
#endif
#if defined(_MSC_VER)
#include <direct.h>
#else
#include <dirent.h>
#endif

#include <signal.h>
#include <map>

#include "game.h"

// Macros to access parser variable structures.
#define VAR(ptr,name) Data *ptr=&parser.Variable(name)
#define MAP(ptr,key) ptr=ptr->FindKey(key);ptr=&(*ptr)[1]
#define VEC(ptr,index) ptr=&(*ptr)[index]
#define MAPTO(ptr,key,ptr2) ptr2=ptr->FindKey(key);ptr2=&(*ptr2)[1]
#define VECTO(ptr,index,ptr2) ptr2=&(*ptr)[index]

using namespace std;
using namespace Evaluator;

//
// Server
// ======

class Server
{
	string error_trigger1,error_trigger2;
	Evaluator::Triggers event_triggers;

	/// Check if user exist.
	bool IsUser(const Data& username)
		{VAR(ptr,"users"); return ptr->HasKey(username);}
	/// Create a new empty card entry for collection.
	Data NewCard() const
		{return Data(0,0,0.0);}
	/// Return address of the card structure in variable 'users'. Initialize if not exist.
	Data* Card(const Data& user,const Data& number);
	/// Remove obsolete 'prices' entries.
	int RemoveObsoletePrices(const Data& card_number);
	
	Data add_to_collection(const Data&args);
	Data check_card(const Data&args);
	Data count_cards(const Data&args);
	Data del_prices(const Data&args);
	Data have_list(const Data&args);
	Data want_list(const Data&args);
	Data is_user(const Data&args);
	Data min_price(const Data&args);
	Data price_list(const Data&args);
	Data remove_obsolete_prices(const Data& args);
	Data set_error_trigger(const Data&);
	Data set_forsale(const Data&args);
	Data set_price(const Data&args);
	Data user_has_cards(const Data&args);
	Data get_card_data(const Data&args);
#ifdef USE_SQUIRREL
    
    // Needed for the Squirrel interface.
    class ServerProxyHelper : public Evaluator::Libsquirrel::SQProxyHelper
    {
    private:
        // Hopefully the parser will stay for the duration of the program.
        Evaluator::Parser<Server> *parser_;
    public:
        ServerProxyHelper(Evaluator::Parser<Server> *parser)
        {
            parser_ = parser;
        }

        Evaluator::Data call(Evaluator::Data& arg)
        {
            return parser_->call(arg);
        }
    };
    
#endif

  public:

	Evaluator::Parser<Server> parser;
	
	Server(const list<string>& triggers,double bet,map<string,string> options);
	void TryTrigger(const string& str1,const string& str2);
#ifdef USE_SQUIRREL

    // Squirrel proxy helper
    // ---------------------

    Evaluator::Libsquirrel::SQProxyHelper* make_sq_proxy_helper()
    {
        ServerProxyHelper *ph = new ServerProxyHelper(&parser); 
        return ph;
    }
#endif
};

// Globals
// =======

/// Server itself.
static Server* server=0;

// Server member functions
// =======================

Server::Server(const list<string>& triggers,double bet,map<string,string> options) : parser(this)
{
	parser.SetVariable("database.cards",Database::cards.Cards());
	parser.SetVariable("bet",bet);
	
	map<string,string>::iterator j;
	for(j=options.begin(); j!=options.end(); j++)
		parser.SetVariable((*j).first,(*j).second);
	
	parser.SetFunction("add_to_collection",&Server::add_to_collection);
	parser.SetFunction("check_card",&Server::check_card);
	parser.SetFunction("count_cards",&Server::count_cards);
	parser.SetFunction("del_prices",&Server::del_prices);
	parser.SetFunction("is_user",&Server::is_user);
	parser.SetFunction("have_list",&Server::have_list);
	parser.SetFunction("want_list",&Server::want_list);
	parser.SetFunction("min_price",&Server::min_price);
	parser.SetFunction("price_list",&Server::price_list);
	parser.SetFunction("remove_obsolete_prices",&Server::remove_obsolete_prices);
	parser.SetFunction("set_error_trigger",&Server::set_error_trigger);
	parser.SetFunction("set_forsale",&Server::set_forsale);
	parser.SetFunction("set_price",&Server::set_price);
	parser.SetFunction("user_has_cards",&Server::user_has_cards);
	parser.SetFunction("get_card_data",&Server::get_card_data);

	list<string>::const_iterator i;
	for(i=triggers.begin(); i!=triggers.end(); i++)
	{
		cout << "Loading " << (*i) << endl;
		event_triggers.ReadFile(*i);
	}
}

void Server::TryTrigger(const string& str1,const string& str2)
{
	string e1,e2;

	try
	{
		parser(event_triggers(str1,str2));
	}
	catch(Evaluator::LangErr e)
	{
		if(error_trigger1 != "")
		{
			parser.SetVariable("error.message",e.Message());
			e1=error_trigger1;
			e2=error_trigger2;
			error_trigger1="";
			error_trigger2="";
			TryTrigger(e1,e2);
			error_trigger1=e1;
			error_trigger2=e2;
			parser.ClearStacks();
		}
		else
		{
			cout << endl << flush;
			cout << "=================================================================" << endl;
			cout << e.Message() << endl;
			cout << endl << "Stacktrace: " << endl;
			parser.stacktrace(Null);
			exit(1);
		}

	}
}

/// set_error_trigger(key1,key2) - Set hook to call when error occurrs during script.
Data Server::set_error_trigger(const Data& args)
{
	if(!args.IsList(2) || !args[0].IsString() || !args[1].IsString())
		ArgumentError("set_error_trigger",args);

	string e1,e2;

	e1=error_trigger1;
	e2=error_trigger2;
	error_trigger1=args[0].String();
	error_trigger2=args[1].String();

	return Data(e1,e2);
}

Data* Server::Card(const Data& user,const Data& number)
{
	VAR(col,"users");
	MAP(col,user);
	VEC(col,2);
	Data* entry;
	MAPTO(col,number,entry);
	if(entry->IsNull())
		*entry=NewCard();
	
	return entry;
}

// Scan unused pricing information and remove them. Return number of
// entries removed.
int Server::RemoveObsoletePrices(const Data& card_number)
{
	VAR(prices,"prices");
	MAP(prices,card_number);
	if(prices->IsNull())
	{
		prices->MakeList();
		return false;
	}

	// Get list of obsolete sellers.
	list<Data> obsolete;
	const Data& P=*prices;
	Data *entry,*sell;
	for(size_t i=0; i<P.Size(); i++)
	{
	  entry=Card(P[i][0],card_number);
		VECTO(entry,1,sell);
		if(*sell < 1)
			obsolete.push_back(P[i][0]);
	}

	// Clean prices.
	for(list<Data>::iterator i=obsolete.begin(); i!=obsolete.end(); i++)
		*prices=parser.del_entry(Data(*i,*prices));

	return (int)obsolete.size();
}

/// remove_obsolete_prices(card numbers) - Helper function to make
///   transition from older server to the caching server easier. Clean
///   obsolete entries from 'prices' variable. Return number of
///   removed entries.
Data Server::remove_obsolete_prices(const Data& args)
{
	int sum=0;

	if(!args.IsList())
		ArgumentError("remove_obsolete_prices",args);

	for(size_t i=0; i<args.Size(); i++)
	{
		if(!args[i].IsInteger())
			ArgumentError("remove_obsolete_prices",args[i]);

		sum+=RemoveObsoletePrices(args[i].Integer());
	}

	return sum;
}

/// check_card(user name,card number) - Check existense of a user and a card and create new empty card to collection if not already exist.
Data Server::check_card(const Data& args)
{
	if(!args.IsList(2) || !args[0].IsString() || !args[1].IsInteger())
		ArgumentError("check_card",args);
	if(!IsUser(args[0]))
		throw LangErr("check_card","invalid user "+tostr(args[0]).String());
		
	if(args[1].Integer() < 0)
		throw LangErr("check_card","invalid card number");
	
	Card(args[0],args[1]);
	
	return Null;
}

/// add_to_collection(user,list of cards) - Add a list of cards to collection (variable 'users') of the given user. Note: there is no error chcekcing for card numbers.
Data Server::add_to_collection(const Data& args)
{
	if(!args.IsList(2) || !args[0].IsString() || !args[1].IsList())
		ArgumentError("add_to_collection",args);

	VAR(col,"users");
	MAP(col,args[0]);
	VEC(col,2);

	const Data& L=args[1];
	Data *entry,*want;
	
	for(size_t i=0; i<L.Size(); i++)
	{
		if(!L[i].IsInteger())
			ArgumentError("add_to_collection",args);

		MAPTO(col,L[i],entry);

		if(entry->IsNull())
			*entry=NewCard();

		VECTO(entry,1,want);
		VEC(entry,0);
		*entry=*entry+1;
		if(*want < Data(0))
			*want=*want+1;
	}
	
	return Null;
}

/// set_price(user, card, price) - Set the price for a card. Return NULL if fails, 1 if success.
Data Server::set_price(const Data& args)
{
	if(!args.IsList(3) || !args[0].IsString() || !args[1].IsInteger() || !args[2].IsReal())
		ArgumentError("set_price",args);

	if(!IsUser(args[0]))
		throw LangErr("set_price","invalid user "+tostr(args[0]).String());

	Data *entry=Card(args[0],args[1]);
	if(entry->IsNull())
		*entry=NewCard();

	double price=args[2].Real();
	if(price < 0.0)
		return Null;
		
	VEC(entry,2);
	*entry=price;

	VAR(prices,"prices");
	MAP(prices,args[1]);
	if(prices->IsNull())
		prices->MakeList();
	MAP(prices,args[0]);
	*prices=price;
	
	return 1;
}

/// have_list(user asking, user target) - Return a list of cards which target user has more than
///   his trade_limit or has at least one for sale; and asking user has in his want list.
Data Server::have_list(const Data& args)
{
	if(!args.IsList(2) || !args[0].IsString() || !args[1].IsString())
		ArgumentError("have_list",args);

	if(!IsUser(args[0]))
		throw LangErr("have_list","invalid user "+tostr(args[0]).String());
	if(!IsUser(args[1]))
		throw LangErr("have_list","invalid user "+tostr(args[0]).String());

	VAR(asker,"users"); // asker=users{args}
	MAP(asker,args[0]); // asker=users{args[0]}
	VEC(asker,2);       // asker=users{args[0]}[2]
	VAR(user,"users");
	MAP(user,args[1]); // users=users{args[1]}

	// Find the limit
	Data *limit;
	VECTO(user,3,limit);          // limit=users{args[1]}[3]
	VEC(limit,0);                 // limit=users{args[1]}[3][0]
	MAP(limit,Data("trade_limit"));// limit=users{args[1]}[3][0]{"trade_limit"}

	int trade_limit;
	if(limit->IsNull())
		trade_limit=4;
	else
		trade_limit=limit->Integer();

	// Check all cards
	Data ret;
	Data *cards,*entry;
	VECTO(user,2,cards); // cards=users{args}[2]

	ret.MakeList();
	for(size_t i=0; i<cards->Size(); i++)
	{
		if((*cards)[i][1][1] > 0 || (*cards)[i][1][0].Integer() > trade_limit)
		{
			// Add only wanted cards
			MAPTO(asker,(*cards)[i][0],entry);
			if(entry->IsNull())
				*entry=NewCard();
			if((*entry)[1] < 0)
				ret.AddList((*cards)[i][0]);
		}
	}
	
	return ret;
}

/// want_list(user asking, user target) - Return a list of cards which target wants.
Data Server::want_list(const Data& args)
{
	if(!args.IsList(2) || !args[0].IsString() || !args[1].IsString())
		ArgumentError("want_list",args);

	if(!IsUser(args[0]))
		throw LangErr("want_list","invalid user "+tostr(args[0]).String());
	if(!IsUser(args[1]))
		throw LangErr("want_list","invalid user "+tostr(args[0]).String());

	VAR(asker,"users"); // asker=users{args}
	MAP(asker,args[0]); // asker=users{args[0]}
	VEC(asker,2);       // asker=users{args[0]}[2]
	VAR(user,"users");
	MAP(user,args[1]); // users=users{args[1]}

	// Check all cards
	Data ret;
	Data *cards,*entry;
	VECTO(user,2,cards); // cards=users{args}[2]

	ret.MakeList();
	for(size_t i=0; i<cards->Size(); i++)
	{
		// Add only cards that asker have
		MAPTO(asker,(*cards)[i][0],entry);
		if(entry->IsNull())
			*entry=NewCard();

		if((*cards)[i][1][1] < 0 && (*entry)[0] > 0)
			ret.AddList((*cards)[i][0]);
	}
	
	return ret;
}

/// min_price(card number) - Find the cheapest offer for sale. Return
///   (list of sellers,price) pair or NULL if there are 0 cards for sale.
Data Server::min_price(const Data& args)
{
	VECTORIZE(min_price);

	if(!args.IsInteger())
		ArgumentError("min_price",args);

// 	RemoveObsoletePrices(args);
	
	// Search price entry for the card.
	VAR(prices,"prices");
	MAP(prices,args);
	if(prices->IsNull())
		prices->MakeList();

	// Search list of sellers with the best offer.
	const Data& L=*prices;

	Data ret;
	ret.MakeList(2);

	double price=0.0;
	list<string> sellers;

	for(size_t i=0; i<L.Size(); i++)
	{
		double pr=L[i][1].Real();
		
		if(sellers.size()==0 || pr < price)
		{
			sellers.clear();
			sellers.push_back(L[i][0].String());
			price=pr;
		}
		else if(pr==price)
			sellers.push_back(L[i][0].String());
	}

	if(sellers.size()==0)
		return Null;

	// Fill the seller list for return value.
	ret[0].MakeList(sellers.size());
	ret[1]=price;
	list<string>::iterator i;
	size_t j=0;
	for(i=sellers.begin(); i!=sellers.end(); i++)
		ret[0][j++]=*i;

	return ret;
}

/// price_list(list of card numbers or single card number) - With 
///   single card number argument, return the best offer for the card
///   in format (card numer,(seller,price)). If there are not any
///   cards for sale, return NULL. If a list of card numbers
///   is given, return list of the prices for those cards. Without any
///   arguments, return the full list of all prices. NULLs are removed
///   when returning a list.
Data Server::price_list(const Data& args)
{
	if(args.IsInteger())
	{
		int card_number=args.Integer();
		
		VAR(prices,"prices");
	
		// Create empty return value entry:
		//
		//    (card number,("",0.0))
		//
		Data ret(card_number,Data(string(""),0.0));

		// Check if there are any prices.
		if(!prices->HasKey(card_number))
			return Null;

		// Price list for the card
		Data& P=(*prices->FindKey(args))[1];

		double price,minprice=0.0;
		bool first=true;
		list<string> sellers;
		list<int> seller_index;
		
		// Scan through pairs P[j]=(seller,price) and find the best offer
		for(size_t j=0; j<P.Size(); j++)
		{
			price=P[j][1].Real();
			if(first || price < minprice)
			{
				first=false;
				minprice=price;
				sellers.clear();
				seller_index.clear();
				sellers.push_back(P[j][0].String());
				seller_index.push_back(j);
			}
			else if(price==minprice)
			{
				sellers.push_back(P[j][0].String());
				seller_index.push_back(j);
			}
			
		}

		ret[1][1]=minprice;

		if(sellers.size()==0)
			return Null;
		else if(sellers.size()==1)
			ret[1][0]=sellers.front();
		else
			ret[1][0]=string(ToString(sellers.size())+" sellers");

		return ret;
	}
	else if(args.IsList())
	{
		Data ret;
		ret.MakeList();
		Data e;
		for(size_t i=0; i<args.Size(); i++)
		{
			e=price_list(args[i]);
			if(!e.IsNull())
				ret.AddList(e);
		}
		
		return ret;
	}
	else if(args.IsNull())
	{
		VAR(prices,"prices");
		return price_list(prices->Keys());
	}
	else
		ArgumentError("price_list",args);

	return Null;
}

/// set_forsale(user, card, how many) - Set the number of cards for sale (or wanted if < 0). Return NULL if fails, 1 if success and 2 if success and clients may need price refresh.
Data Server::set_forsale(const Data& args)
{
	if(!args.IsList(3) || !args[0].IsString() || !args[1].IsInteger() || !args[2].IsInteger())
		ArgumentError("set_forsale",args);

	if(!IsUser(args[0]))
		throw LangErr("set_forsale","invalid user "+tostr(args[0]).String());

	Data* entry=Card(args[0],args[1]);
	if(entry->IsNull())
		*entry=NewCard();

	Data *have,*forsale;

	VECTO(entry,0,have);
	VECTO(entry,1,forsale);

	int new_forsale=args[2].Integer();
	int old_forsale=forsale->Integer();

	if(*have < new_forsale)
		return Null;

	*forsale=new_forsale;

	if((old_forsale > 0 && new_forsale<=0) || (old_forsale <= 0 && new_forsale > 0))
		return 2;

	return 1;
}

/// user_has_cards(user, card list) - Return number of missing cards in card list (nmb. of cards,card number).
Data Server::user_has_cards(const Data& args)
{
	if(!args.IsList(2) || !args[1].IsList())
		ArgumentError("user_has_cards",args);

	if(!IsUser(args[0]))
		throw LangErr("user_has_cards","invalid user "+tostr(args[0]).String());

	const Data& L=args[1];
	Data* entry;
	int proxies=0;
	
	VAR(col,"users");
	MAP(col,args[0]);
	VEC(col,2);

	for(size_t i=0; i<L.Size(); i++)
	{
		if(!L[i].IsList(2) || !L[i][0].IsInteger() || !L[i][1].IsInteger())
			throw LangErr("user_has_cards","invalid entry "+tostr(L[i]).String());

		entry=Card(args[0],L[i][1]);
		if((*entry)[0].Integer() < L[i][0].Integer())
			proxies+=L[i][0].Integer() - (*entry)[0].Integer();
	}
	
	return proxies;
}

/// count_cards(user) - Return triplet (# cards owned,# cards for sale,# cards wanted) for user.
Data Server::count_cards(const Data& args)
{
	if(!args.IsString())
		ArgumentError("count_cards",args);
	if(!IsUser(args))
		throw LangErr("count_cards","invalid user "+args.String());

	VAR(col,"users");
	MAP(col,args);
	VEC(col,2);

	const Data& L=*col;
	const Data* entry;
	int n,sale=0,wanted=0,total=0;
	
	for(size_t i=0; i<L.Size(); i++)
	{
		entry=&L[i][1];
		total += (*entry)[0].Integer();
		n=(*entry)[1].Integer();
		if(n<0)
			wanted-=n;
		else
			sale+=n;
	}

	return Data(total,sale,wanted);
}

/// get_card_data(user,card list) - Return a list of full update entries for cards given.
///  check the validity of all arguments and return NULL if some of the arguments 
///  are not valid.
Data Server::get_card_data(const Data& args)
{
	if(!args.IsList(2) || !args[0].IsString())
		ArgumentError("get_card_data",args);
	if(!IsUser(args[0]))
		throw LangErr("get_card_data","invalid user "+args.String());

	if(!args[1].IsList())
	    return Data();

	VAR(collection,"users");
	MAP(collection,args[0]);
	VEC(collection,2);

	const Data& L=args[1];
	Data ret;
	ret.MakeList();
	Data entry;
	entry.MakeList(5);
	Data price;
	Data* card_data;

	for(size_t i=0; i<L.Size(); i++)
	{
	    if(!L[i].IsInteger())
		return Data();

	    int card=L[i].Integer();
	    if(card < 0)
		return Data();

	    // Get price data
	    price=price_list(L[i]);
	    if(price.IsNull())
	    {
		entry[1]=0.0;
		entry[2]="";
	    }
	    else
	    {
		entry[1]=price[1][1];
		entry[2]=price[1][0];
	    }
	    // Get collection data
	    if(collection->HasKey(L[i]))
	    {
		MAPTO(collection,L[i],card_data);
 		entry[0]=(*card_data)[0];
 		entry[3]=(*card_data)[2];
 		entry[4]=(*card_data)[1];
	    }
	    else
	    {
		entry[0]=0;
		entry[3]=0.0;
 		entry[4]=0;
	    }

	    ret.AddList(Data(L[i],entry));
	}

	if(ret.Size())
	    return ret;

	return Data();
}


/// is_user(user name) - Return 1 if user exist.
Data Server::is_user(const Data& args)
{
	if(!args.IsString())
		ArgumentError("is_user",args);

	return (int)IsUser(args);
}

/// del_prices(user name) - Delete all prices from 'prices' variable set by the given player. Return 1 if succesful.
Data Server::del_prices(const Data& args)
{
	if(!args.IsString())
		ArgumentError("del_prices",args);

	if(!IsUser(args))
		return Null;

	VAR(prices,"prices");
	Data& P=*prices;

	for(size_t i=0; i<P.Size();)
	{
		Data& Q=P[i][1]; // Q = user to price map for the card
		for(size_t j=0; j<Q.Size(); j++)
		{
			if(Q[j][0]==args)
			{
				Q.DelList(j);
				break;
			}
		}
		
		if(Q.Size()==0)
		{
			P.DelList(i);
		}
		else
			i++;
	}

	return 1;
}

// MAIN
// ====

void usage()
{
	cout << "usage: server [options] <game.xml>" << endl;
	cout << "  options: --players <number of players>" << endl;
	cout << "           --debug" << endl;
	cout << "           --full-debug" << endl;
	cout << "           --tournament" << endl;
	cout << "           --bet <money>" << endl;
	cout << "           --server-ip <local server connecion ip>" << endl;
	cout << "           --port <server port>" << endl;
	cout << "           --load <trigger file>" << endl;
	cout << "  game server specific:" << endl;
	cout << "           --server <meta server>" << endl;
	cout << "           --rules <rules file>" << endl;
}

int main(int argc,const char** argv)
{
	int status=0;
	bool dont_load_card_data=false;

	try
	{
		Evaluator::InitializeLibnet();
		Evaluator::InitializeLibcards();

		if(argc < 2)
		{
			usage();
			return 1;
		}

		cout << PACKAGE << " v" << VERSION << " Server ("  << SYSTEM << ")" << endl;
		cout << "(c) 2001-2009 Tommi Ronkainen" << endl;

		security.AllowReadFile(CCG_SAVEDIR"/*");
		security.AllowWriteFile("./vardump");

		int port=-1,players=0;
		Data ret;
		int arg=1;
		bool debug=false;
		bool tournament=false;
		bool fulldebug=false;
		double bet=0.0;
		map<string,string> options;
		list<string> triggers;

		/* Parse options */
		string opt=argv[arg];
		while(opt.length() && opt[0]=='-')
		{
			if(opt=="--players")
				players=atoi(argv[++arg]);
			else if(opt=="--bet")
				bet=atof(argv[++arg]);
			else if(opt=="--load")
			{
				string triggerfile=CCG_DATADIR;
				triggerfile+="/scripts/";
				arg++;
				if(string(argv[arg]).substr(0,4)=="meta")
					dont_load_card_data=true;
				triggerfile+=argv[arg];
				triggers.push_back(triggerfile);
			}
			else if(opt=="--debug")
				debug=true;
			else if(opt=="--full-debug")
				fulldebug=true;
			else if(opt=="--tournament")
				tournament=true;
			else if(opt=="--port")
				port=atoi(argv[++arg]);
			else if(opt=="--server")
				options["server"]=argv[++arg];
			else if(opt=="--server-ip")
				options["server.ip"]=argv[++arg];
			else if(opt=="--rules")
				options["rules"]=argv[++arg];
			else
			{
				cerr << "server: invalid option "+opt << endl;
				return 1;
			}
			arg++;
			if(arg>=argc)
				break;
			opt=argv[arg];
		}

		/* Load game description */
			
		Evaluator::savedir=CCG_SAVEDIR;
		if(!opendir(Evaluator::savedir.c_str()))
#if defined(WIN32) || defined(__BCPLUSPLUS__)
			_mkdir(Evaluator::savedir.c_str());
#else
			mkdir(Evaluator::savedir.c_str(),0700);
#endif

		if(arg >= argc)
		{
			usage();
			return 1;
		}

		opt=CCG_DATADIR;
		opt+="/xml/";
		opt+=argv[arg++];
		cout << "Loading " << opt << endl;
		Database::game.ReadFile(opt);
		Evaluator::savedir+="/";
		Evaluator::savedir+=Database::game.Gamedir();

		security.AllowWriteFile(CCG_SAVEDIR"/"+Database::game.Gamedir()+"/*");
		security.AllowExecute(CCG_DATADIR"/scripts/meta-server.functions");
		security.AllowExecute(CCG_DATADIR"/scripts/factory-server.functions");
		security.AllowExecute(CCG_DATADIR"/scripts/central-server.functions");
		security.AllowExecute(CCG_DATADIR"/scripts/server.functions");
		security.AllowExecute(CCG_DATADIR"/scripts/common.include");
		security.AllowExecute(CCG_DATADIR"/scripts/"+Database::game.Gamedir()+"-meta-server.include");
		security.AllowExecute(CCG_DATADIR"/scripts/"+Database::game.Gamedir()+"-factory-server.include");
		security.AllowExecute(CCG_DATADIR"/scripts/"+Database::game.Gamedir()+"-central-server.include");
		security.AllowExecute(CCG_DATADIR"/scripts/"+Database::game.Gamedir()+"-server.include");
		security.AllowExecute(CCG_DATADIR"/scripts/"+Database::game.Gamedir()+"-common.include");
		security.AllowExecute(CCG_DATADIR"/scripts/"+Database::game.Gamedir()+".rules");
		security.AllowWriteFile("./vardump");
		security.AllowOpenDir(CCG_DATADIR"/scripts");
		security.AllowOpenDir(CCG_DATADIR"/scripts/global");
		security.AllowOpenDir(CCG_DATADIR"/scripts/global-server");
		security.AllowOpenDir(CCG_DATADIR"/scripts/"+Database::game.Gamedir());	
		security.AllowOpenDir(CCG_DATADIR"/scripts/"+Database::game.Gamedir()+"-server");
		security.AllowOpenDir(CCG_SAVEDIR"/"+Database::game.Gamedir()+"/users-db");

		if(options.find("rules")!=options.end())
			security.AllowExecute(CCG_DATADIR"/scripts/"+options["rules"]);

		if(!opendir(Evaluator::savedir.c_str()))
#if defined(WIN32) || defined(__BCPLUSPLUS__)
			_mkdir(Evaluator::savedir.c_str());
#else
			mkdir(Evaluator::savedir.c_str(),0700);
#endif

		if(arg < argc)
		{
			usage();
			return 1;
		}

		/* Parse card descriptions */
		if(!dont_load_card_data)
			for(int i=0; i<Database::game.CardSets(); i++)
			{
				opt=CCG_DATADIR;
				opt+="/xml/";
				opt+=Database::game.Gamedir();
				opt+="/";
				opt+=Database::game.CardSet(i);

                                if(!FileExist(opt))
                                {
                                    opt=CCG_DATADIR;
                                    opt+="/../";
                                    opt+=ToLower(Database::game.Gamedir());
                                    opt+="/xml/";
                                    opt+=Database::game.Gamedir();
                                    opt+="/";
                                    opt+=Database::game.CardSet(i);
                                }

				cout << "Loading " << opt << endl;
				Database::cards.AddCards(opt);
				if(Evaluator::quitsignal)
					throw Error::Quit(1);
			}

		cout << "Total of " << Database::cards.Cards() << " cards loaded." << endl;
		server=new Server(triggers,bet,options);

		if(debug)
			server->parser.SetVariable("options.debug",1);
		if(tournament)
			server->parser.SetVariable("options.tournament",1);
		if(fulldebug)
			Evaluator::debug=true;

		cout << "Calling \"init\" \"\"" << endl;
		server->TryTrigger("init","");
		
		if(port > 0)
			server->parser.SetVariable("port",port);
		if(players > 0)
			server->parser.SetVariable("players_wanted",players);

		security.AllowCreateSocket(server->parser.Variable("port").Integer());
		if(server->parser.Variable("meta.server").IsString())
			security.AllowConnect(server->parser.Variable("meta.server").String(),server->parser.Variable("meta.port").Integer());
		if(server->parser.Variable("factory.server_name").IsString())
			security.AllowConnect(server->parser.Variable("factory.server_name").String(),server->parser.Variable("factory.port").Integer());
	
		cout << "Calling \"init\" \"server\"" << endl;
		server->TryTrigger("init","server");
		cout << "Calling \"init\" \"game\"" << endl;
		server->TryTrigger("init","game");

		while(1)
		{
			if(Evaluator::quitsignal)
			{
				status=1;
				break;
			}

			server->TryTrigger("main","");
		}
	}
	catch(Error::Quit q)
	{
		status=q.ExitCode();
	}
	catch(Error::General e)
	{
		cout << endl << flush;
		cerr << e.Message() << endl;
		status=255;
	}
        catch(exception& e)
        {
            cerr << "Unhandled exception: " << e.what() << endl;
        }

	// Finish.
	try
	{
		if(server)
		{
			cout << "Calling \"exit\" \"\"" << endl;
			server->TryTrigger("exit","");
		}
	}
	catch(Error::General e)
	{
		cout << endl << flush;
		cerr << e.Message() << endl;
	}

	cout << "Done." << endl;

	if(server)
		delete server;
	
	return status;
}


