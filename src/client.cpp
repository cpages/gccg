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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#if !defined(__BCPLUSPLUS__) && !defined(_MSC_VER)
# include <unistd.h>
#elif defined(__BCPLUSPLUS__)
# include <dir.h>
#endif
#if defined(_MSC_VER)
# include "compat.h"
# include <direct.h>
#else
# include <dirent.h>
#endif
#include <algorithm>
#include <stdlib.h>
#include "game.h"
#include "error.h"
#include "version.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "tools.h"

using namespace CCG;
using namespace Database;
using namespace Evaluator;
using namespace Driver;

//
// Class Client
// ============

void Table::Main(const string& server,int port,string username)
{
	Command command;
	Object* obj;
	Data ret,net;
	static int counter=0;

	// Initialize variables.
	parser.SetVariable("database.cards",Database::cards.Cards());
	if(username != "")
		parser.SetVariable("username",username);
	else
		parser.SetVariable("username",string(getenv("USER")));

	if(server != "")
		parser.SetVariable("server.name",server);
	if(port > 0)
		parser.SetVariable("port",port);
        
        parser.SetVariable("marker_images",(int)CCG::marker_image.size());

	SetTableCardSize(atoi(game.Option("table card size").c_str()));
	SetHandCardSize(atoi(game.Option("hand card size").c_str()));
	SetDeckCardSize(atoi(game.Option("deck card size").c_str()));
	SetBookCardSize(atoi(game.Option("book card size").c_str()));
	
	// Load inline images.
	list<XML::Element*> load_inline;
	list<XML::Element*>::iterator i;
	
        for(int d=1; d<=6; d++)
        {
            string file=CCG_DATADIR"/graphics/";
            file+="dice";
            file+=ToString(d);
            file+=".png";

            string tag="{dice"+ToString(d)+"}";

            driver->RegisterInlineImage(file,tag);
        }

	load_inline=game.XML().Base()->Subs("image");
	for(i=load_inline.begin(); i!=load_inline.end(); i++)
	{
		string file=CCG_DATADIR"/graphics/";
		file+=game.Gamedir();
		file+="/";
		file+=(**i)["file"];
                if(!FileExist(file))
                {
                    file=CCG_DATADIR"/graphics/";
                    file+=(**i)["file"];
                }

		driver->RegisterInlineImage(file,(**i)["tag"]);
	}
	
	// Call initialization triggers.
	cout << Localization::Message("Calling %s","\"init\" \"client\"") << endl;
	TryTrigger("init","client",ret);
	cout << Localization::Message("Calling %s","\"init\" \"game\"") << endl;
	TryTrigger("init","game",ret);

	// Main loop.
	try
	{
	    time_t t,timer_second,timer_second5,timer_second15,timer_minute;
		
	    timer_second=::time(0);
	    timer_second5=timer_second;
	    timer_second15=timer_second;
	    timer_minute=timer_second;
		
	    while(1)
	    {
		do
		{
		    if(++counter % 5 == 0)
			TryTrigger("timer","",net);

		    t=::time(0);
				
		    if(t!=timer_second)
		    {
			timer_second=timer_second+1;
			TryTrigger("timer","second",net);
		    }
		    if(t>=timer_second5)
		    {
			timer_second5=timer_second5+5;
			TryTrigger("timer","5 seconds",net);
		    }
		    if(t>=timer_second15)
		    {
			timer_second15=timer_second15+15;
			TryTrigger("timer","15 seconds",net);
		    }
		    if(t>=timer_minute)
		    {
			timer_minute=timer_minute+60;
			TryTrigger("timer","minute",net);
		    }
				
		    command=driver->WaitCommand(10);
				
		} while(command.command == "" && !Evaluator::quitsignal);

		if(Evaluator::quitsignal)
		    break;
			
		parser.UnsetVariable("book.card_index");
		parser.UnsetVariable("book.tab");
		parser.UnsetVariable("card.name");
		parser.UnsetVariable("card.number");
		parser.UnsetVariable("cardbox.card_index");
		parser.UnsetVariable("hand.card_index");
		parser.UnsetVariable("listbox.index");
		parser.UnsetVariable("menu.index");
		parser.UnsetVariable("menu.text");
		parser.UnsetVariable("menu.shortcut");
		parser.UnsetVariable("menu.action");
		parser.UnsetVariable("object.name");
		parser.UnsetVariable("object.number");
		parser.UnsetVariable("object.type");
			
		obj=WidgetAt(command.x,command.y);

		if(obj)
		{
		    parser.SetVariable("object.number",obj->Number());
		    parser.SetVariable("object.name",obj->Name());
		    switch(obj->Type())
		    {
		      case TypeCardInPlay:
			  {
			      CardInPlay* card=dynamic_cast<CardInPlay*>(obj);
			      parser.SetVariable("object.type",string("card"));
			      parser.SetVariable("card.name",card->Name());
			      parser.SetVariable("card.number",card->Number());					  
			      break;
			  }
				
		      case TypeHand:
			  {
			      Hand* hand=dynamic_cast<Hand*>(obj);
			      int i=hand->CardAt(command.x,command.y);
			      parser.SetVariable("object.type",string("hand"));
			      if(i >= 0)
			      {
				  parser.SetVariable("hand.card_index",i);
				  parser.SetVariable("card.name",cards.Name((*hand)[i]));
				  parser.SetVariable("card.number",(*hand)[i]);
			      }
			      break;
			  }

		      case TypeCardBox:
			  {
			      CardBox* cardbox=dynamic_cast<CardBox*>(obj);
			      int i=cardbox->CardAt(command.x,command.y);
			      if(i >= 0)
			      {
				  parser.SetVariable("object.type",string("cardbox"));
				  parser.SetVariable("cardbox.card_index",i);
				  parser.SetVariable("card.name",cards.Name((*cardbox)[i]));
				  parser.SetVariable("card.number",(*cardbox)[i]);
			      }
			      break;
			  }

		      case TypeDeck:
			  {
			      Deck* deck=dynamic_cast<Deck*>(obj);
			      parser.SetVariable("object.type",string("deck"));
			      if(deck->Size())
			      {
				  parser.SetVariable("card.name",cards.Name(deck->Top()));
				  parser.SetVariable("card.number",deck->Top());
			      }
			      break;
			  }

		      case TypeImage:
			  {
			      parser.SetVariable("object.type",string("image"));
			      break;
			  }
			  
		      case TypeCardBook:
			  {
			      parser.SetVariable("object.type",string("book"));
			      CardBook* cardbook=dynamic_cast<CardBook*>(obj);
			      int i=cardbook->SlotAt(command.x,command.y);
			      if(i >= 0)
			      {
				  parser.SetVariable("book.card_index",i);
				  parser.SetVariable("card.name",cards.Name(cardbook->CardAt(i)));
				  parser.SetVariable("card.number",cardbook->CardAt(i));
			      }
			      i=cardbook->TabAt(command.x,command.y);
			      if(i >= 0)
			      {
				  parser.SetVariable("book.tab",i);
			      }
			      break;
			  }

		      case TypeMenu:
			  {
			      Menu* menu=dynamic_cast<Menu*>(obj);
			      int e;
			      parser.SetVariable("object.type",string("menu"));
			      e=menu->At(command.x,command.y);
			      parser.SetVariable("menu.index",e);
			      if(e >= 0)
			      {
				  MenuEntry ent=(*menu)[e];
				  parser.SetVariable("menu.text",ent.text);
				  parser.SetVariable("menu.shortcut",ent.shortcut);
				  parser.SetVariable("menu.action",ent.action);
			      }
			      break;
			  }

		      case TypeListBox:
			  {
			      ListBox* listbox=dynamic_cast<ListBox*>(obj);
			      int e;
			      parser.SetVariable("object.type",string("listbox"));
			      e=listbox->At(command.x,command.y);
			      parser.SetVariable("listbox.index",e);
			      if(e >= 0)
			      {
			      }
			      break;
			  }

		      default:
			  break;
		    }
		} // if(obj)

		parser.SetVariable("mouse.x",command.x);
		parser.SetVariable("mouse.y",command.y);

		if(command.command=="redraw")
		{
		    Refresh();
		}
		else if(command.command=="left drag begin")
		{
		    string button;
		    if(command.command=="left drag begin")
			button="left";
		    else
			button="middle";

		    if(obj)
		    {
			if(!obj->grp.draggable && !obj->grp.dragparent)
			    TryTrigger(button+" click","",ret);
			else
			{
			    TryTrigger(button+" drag begin","",ret);
			    int x,y,n;
			    x=command.x;
			    y=command.y;
			    if(DragWidget(button,x,y,n))
			    {
				int objx,objy,objw,objh;
							
				objx=x;
				objw=objects[n]->grp.w;
				objy=y;
				objh=objects[n]->grp.h;						
				objx+=objw/2;
				objy+=objh/2;
						
				Screen2Table(objx,objy);
						
				parser.SetVariable("object.x",objx);
				parser.SetVariable("object.y",objy);
				parser.SetVariable("object.number",n);
				TryTrigger("move object","",ret);
			    }
			    TryTrigger(button+" drag end","",ret);
			}
		    }
		}
		else if(command.command=="left click" || command.command=="middle click")
		{
		    if(obj && obj->Type()==TypeMenu)
		    {
			if(TryTrigger("menu",obj->Name(),ret))
			    continue;
			if(TryTrigger("menu","",ret))
			    continue;
		    }

		    if(obj)
		    {
			if(!TryTrigger(command.command,obj->Name(),ret))
			    TryTrigger(command.command,command.argument,ret);
		    }
		    else
			TryTrigger(command.command,command.argument,ret);					
		}
		else if(command.command=="right click")
		{
		    if(obj)
		    {
			if(!TryTrigger(command.command,obj->Name(),ret))
			    TryTrigger(command.command,command.argument,ret);
		    }
		    else
			TryTrigger(command.command,command.argument,ret);
		}
		else if((command.command=="control key" && command.argument=="q")
		  || (command.command=="quit" && command.argument==""))
		{
		    break;
		}
		else if((command.command=="alt key" && command.argument=="return") ||
		  (command.command=="control alt key" && command.argument=="f"))
		{
		    if(driver->IsFullscreen())
			driver->Fullscreen(0);
		    else
			driver->Fullscreen(1);
		}
		else if(command.command=="shift key" && command.argument=="insert")
		{
			char buf[1024] = {0};
#ifdef _MSC_VER
			win32_get_clipboard_text(buf, 1024);
#else
			// TODO: Unix
#endif
			string b(buf);
			size_t p=0;
			while((p=b.find_first_of("{}",p))!=string::npos)
			{
				switch(b[p])
				{
				case '{': b.replace(p, 1, "{lb}lb{rb}"); p+=10; break;
				case '}': b.replace(p, 1, "{lb}rb{rb}"); p+=10; break;
				}
			}
			parser.SetVariable("clipboard.text",b);
			TryTrigger("paste","",ret);
		}
		else if(command.command=="key" || command.command=="shift key" || command.command=="control key" || command.command=="alt key" || command.command=="control alt key" || command.command=="shift control key" || command.command=="shift alt key" || command.command=="shift control alt key")
		{
		    if(!TryTrigger(command.command,command.argument,ret))
		    {
			char ascii[2];

			if(command.key < 32 || command.key==127)
			    ascii[0]=0;
			else
			    ascii[0]=command.key;
			ascii[1]=0;

			parser.SetVariable("key",command.command+" "+command.argument);
			parser.SetVariable("key.ascii",string(ascii));
			TryTrigger("key","",ret);
			parser.UnsetVariable("key");
			parser.UnsetVariable("key.ascii");
		    }
		}
		else
		{
		    TryTrigger(command.command,command.argument,ret);
		}

	    } // while(1)
	}
	catch(Error::Quit)
	{
	}
	
	cout << Localization::Message("Calling %s","\"exit\" \"\"") << endl;
	TryTrigger("exit","",ret);
}

void usage()
{
    cout << "usage: ccg_client [<options>...] <game.xml>" << endl;
    cout << " options: --debug" << endl;
    cout << "          --full-debug" << endl;
    cout << "          --lang-debug" << endl;
    cout << "          --full" << endl;
	cout << "          --design <width>x<height> (default: " << DEFAULT_DESIGN_WIDTH << "x" << DEFAULT_DESIGN_HEIGHT << ")" << endl;
    cout << "          --geometry <width>x<height>" << endl;
    cout << "          --nographics" << endl;
    cout << "          --nosounds" << endl;
    cout << "          --cache" << endl;
    cout << "          --windows-fonts" << endl;
    cout << "          --server <server name>" << endl;
    cout << "          --port <port number>" << endl;
    cout << "          --user <user name>" << endl;

    exit(0);
}

// MAIN PROGRAM
// ============

int gccg_main(int argc,const char** argv)
{
    bool nographics=false,full=false,debug=false,fulldebug=false;
    string server,username;
    int port=-1;
    int scrw=DEFAULT_DESIGN_WIDTH,scrh=DEFAULT_DESIGN_HEIGHT;
    string lang="en";
	
    try
    {
	Evaluator::InitializeLibnet();
	Evaluator::InitializeLibcards();
#ifdef USE_SQUIRREL
#ifndef USE_SQUIRREL
    Evaluator::InitializeLibsquirrel();
#endif
#endif

	cout << PACKAGE << " v" << VERSION << " Generic CCG Client (" << SYSTEM << ")" << endl;
	cout << "(c) 2001-2009 Tommi Ronkainen" << endl;

	Localization::ReadDictionary(CCG_DATADIR"/lib/dictionary.client");
		
	if(argc < 2)
	    usage();

	int arg=1;

	string opt=argv[arg];
	while(arg < argc && opt.length() && opt[0]=='-')
	{
	    if(opt=="--help" || opt=="-h" || opt=="-?")
		usage();
	    else if(opt=="--full")
		full=true;
	    else if(opt=="--design" && arg+1 < argc)
	    {
			scrw=atoi(argv[++arg]);
			const char* s=argv[arg];
			while(*s && *s != 'x')
				s++;
			s++;
			scrh=atoi(s);
			if(scrw==0 || scrh==0)
			{
				cerr << "ccg_client: invalid design" << endl;
				return 1;
			}
			// also override default geometry
			CCG::Table::SetDesignWidthHeight(scrw,scrh);
		}
	    else if(opt=="--geometry" && arg+1 < argc)
	    {
		scrw=atoi(argv[++arg]);
		const char* s=argv[arg];
		while(*s && *s != 'x')
		    s++;
		s++;
		scrh=atoi(s);
		if(scrw==0 || scrh==0)
		{
		    cerr << "ccg_client: invalid geometry" << endl;
		    return 1;
		}
	    }
	    else if(opt=="--full-debug")
		fulldebug=true;
	    else if(opt=="--lang-debug")
		Localization::debug=true;
	    else if(opt=="--debug")
		debug=true;
	    else if(opt=="--nographics")
		nographics=true;
	    else if(opt=="--windows-fonts")
		use_free_fonts=false;
	    else if(opt=="--cache")
		nocache=false;
	    else if(opt=="--server" && arg+1 < argc)
		server=argv[++arg];
	    else if(opt=="--user" && arg+1 < argc)
		username=argv[++arg];
	    else if(opt=="--port" && arg+1 < argc)
		port=atoi(argv[++arg]);
	    else if(opt=="--lang" && arg < argc-1)
	    {
		lang=argv[++arg];
		Localization::SetLanguage(lang);
		cout << Localization::Message("Selecting language: %s",Localization::LanguageName(lang)) << endl;
	    }
	    else if(opt=="--nosounds")
		nosounds=true;
	    else
	    {
		cerr << "ccg_client: invalid options "+opt << endl;
		return 1;
	    }
	    arg++;
	    if(arg < argc)
		opt=argv[arg];
	}

	// 	security.Disable();
	security.AllowReadFile(CCG_DATADIR"/decks/*");
	security.AllowReadFile(CCG_DATADIR"/graphics/*");

	security.AllowReadFile(CCG_DATADIR"/sounds/*");
	security.AllowExecute(CCG_DATADIR"/scripts/*");
	security.AllowReadFile(CCG_DATADIR"/scripts/*");
	security.AllowReadFile(getenv("HOME")+string("/.gccg/*"));
	security.AllowExecute(getenv("HOME")+string("/.gccg/init*"));
	security.AllowWriteFile("./vardump");
	security.AllowOpenDir(CCG_DATADIR"/scripts");
	security.AllowOpenDir(CCG_DATADIR"/scripts/global");
	
	// Load game description and create save dir.
	if(arg < argc)
	{			
	    opt=CCG_DATADIR;
	    opt+="/xml/";
	    opt+=argv[arg];
	    cout << Localization::Message("Loading game description %s",Localization::File(opt)) << endl;
	    Database::game.ReadFile(opt);

            // Read extended copy if found.
	    opt=CCG_DATADIR;
            opt+="/../";
            opt+=ToLower(Database::game.Gamedir());
            opt+="/xml/";
	    opt+=argv[arg];
            if(FileExist(opt))
            {
                cout << Localization::Message("Loading game description %s",Localization::File(opt)) << endl;
                Database::game.ReadFile(opt);
            }

            arg++;

	    string df=CCG_DATADIR"/lib/"+Database::game.Gamedir()+"/dictionary.client";
	    if(FileExist(df))
		Localization::ReadDictionary(df);
			
	    string f2,f=getenv("HOME");
	    f+="/.gccg";

	    if(!opendir(f.c_str()))
	    {
		cout << Localization::Message("Creating %s",f) << endl;
#ifdef WIN32
		_mkdir(f.c_str());
#else
		mkdir(f.c_str(),0700);
#endif
	    }

	    f+="/";
	    f+=Database::game.Gamedir();
	    if(!opendir(f.c_str()))
	    {
		cout << Localization::Message("Creating %s",f) << endl;
#ifdef WIN32
		_mkdir(f.c_str());
#else
		mkdir(f.c_str(),0700);
#endif
	    }
			
	    Evaluator::savedir=f;
	    f2=f;
	    f+="/import";
	    f2+="/export";
	    if(!opendir(f.c_str()))
	    {
		cout << Localization::Message("Creating %s",f) << endl;
#ifdef WIN32
		_mkdir(f.c_str());
#else
		mkdir(f.c_str(),0700);
#endif
	    }
	    if(!opendir(f2.c_str()))
	    {
		cout << Localization::Message("Creating %s",f2) << endl;
#ifdef WIN32
		_mkdir(f2.c_str());
#else
		mkdir(f2.c_str(),0700);
#endif
	    }

	    Evaluator::execdir=CCG_DATADIR;
	    Evaluator::execdir+="/scripts/";
	    Evaluator::execdir+=Database::game.Gamedir();
	}

	if(arg < argc)
	{
	    usage();
	    return 0;
	}
		
	// Set security
		
	security.AllowReadFile(CCG_DATADIR"/../"+ToLower(Database::game.Gamedir())+"/graphics/"+Database::game.Gamedir()+"/*");
	security.AllowReadFile(CCG_DATADIR"/../"+ToLower(Database::game.Gamedir())+"/graphics/*");

	security.AllowOpenDir(CCG_DATADIR"/scripts/"+Database::game.Gamedir());
	security.AllowOpenDir(CCG_DATADIR"/decks/"+Database::game.Gamedir());
	security.AllowOpenDir(CCG_DATADIR"/decks/"+Database::game.Gamedir()+"/*");
	security.AllowOpenDir(getenv("HOME")+string("/.gccg/")+Database::game.Gamedir()+"/import");
	security.AllowOpenDir(getenv("HOME")+string("/.gccg/")+Database::game.Gamedir()+"/import/*");
	security.AllowOpenDir(getenv("HOME")+string("/.gccg/")+Database::game.Gamedir()+"/export");
	security.AllowOpenDir(getenv("HOME")+string("/.gccg/")+Database::game.Gamedir()+"/export/*");
	security.AllowWriteFile(getenv("HOME")+string("/.gccg/")+Database::game.Gamedir()+"/*");
	security.AllowConnect("*",ANY_PORT);

	// Load card sets.

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
            if(!FileExist(opt))
            {
                cout << Localization::Message("Cannot load %s (maybe need to download extra repository).",Localization::File(opt)) << endl;
                continue;
            }
	    cout << Localization::Message("Loading %s",Localization::File(opt)) << endl;
	    Database::cards.AddCards(opt);
	    if(Evaluator::quitsignal)
		throw Error::Quit(1);
	}

	CCG::Table C(CCG_DATADIR"/scripts/client.triggers",full,debug,fulldebug,nographics,scrw,scrh);
	C.Main(server,port,username);
    }
    catch(Error::General e)
    {
	cout << endl << flush;
	cerr << e.Message() << endl;
#ifdef WIN32
	win32_display_error("Error",e.Message().c_str());
#endif
    }
    catch(exception& e)
    {
        cerr << "Unhandled exception: " << e.what() << endl;
#ifdef WIN32
		win32_display_error("Unhandled exception",e.what());
#endif
    }

    return 0;
}
