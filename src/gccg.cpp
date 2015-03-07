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

#include <fstream>
#include <signal.h>
#include "parser.h"
#include "triggers.h"
#include "tools.h"
#include "carddata.h"
#include "localization.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace Evaluator
{
	void InitializeLibcards();
}

using namespace Evaluator;

/// Parser tester.
#ifndef USE_SQUIRREL
class Shell
#else
class Shell : public Application
#endif
{
#ifdef USE_SQUIRREL
private:
    // Needed for the Squirrel interface.
    class ShellProxyHelper : public Evaluator::Libsquirrel::SQProxyHelper
    {
        public:
            Evaluator::Data call(Evaluator::Data& arg)
            {
                cout << "Can't call GCCG script from Squirrel in the parser tester." << endl;
                abort();
            }
    };
#endif
public:
	/// Main program for parser tester.
	void main(Parser<Shell>& P);
#ifdef USE_SQUIRREL

    Evaluator::Libsquirrel::SQProxyHelper* make_sq_proxy_helper()
    {
        // NOTE: This bombs bigtime if you try to call GCCG
        // from the shell tester.
        ShellProxyHelper *ph = new ShellProxyHelper(); 
        return ph;
    }
#endif
};

void Shell::main(Parser<Shell>& P)
{
	try
	{
		Triggers T;
		P(T("init",""));

		string line;
		
		while(cin)
		{
			if(Evaluator::quitsignal)
				return;
			
			cout << ">";
			line=readline(cin);

			try
			{
				cout << tostr(P(line+";")) << endl;
			}
			catch(Evaluator::LangErr e)
			{
				cerr << e.Message() << endl;
			}
		}
	}
	catch(Error::General e)
	{
		cerr << e.Message() << endl;
	}
}

int main(int argc,char** argv)
{
	string lang="en";
	
	cout << PACKAGE << "-Script shell v" << VERSION << endl;
	cout << "(c) 2001,2002,2003,2004,2005 Tommi Ronkainen" << endl;
	
	Evaluator::savedir=CCG_SAVEDIR;
	Evaluator::InitializeLibnet();
	Evaluator::InitializeLibcards();

	security.Disable();

	try {
		int arg=1;
		Parser<Shell> P(0);

		while(arg < argc)
		{
			if(string("--help")==argv[arg] || string("-h")==argv[arg] || string("-?")==argv[arg])
			{
				cout << "usage: gccg [--security] [--lang <code>] [--debug] [--load <game.xml>] [<script file>...]" << endl;
				return 0;
			}
			else if(string("--debug")==argv[arg])
			{
				Localization::debug=true;
			}
			else if(string("--security")==argv[arg])
			{
				security.Enable();
				security.AllowReadFile(CCG_DATADIR"/decks/*");
				security.AllowReadWriteFile(CCG_SAVEDIR"/*");
				security.AllowExecute(CCG_DATADIR"/scripts/*");
				security.AllowExecute(string(getenv("HOME"))+"/.gccg/init");
				security.AllowWriteFile("./vardump");
				security.AllowOpenDir(CCG_SAVEDIR"/*");
				security.AllowOpenDir(CCG_DATADIR"/scripts");
				security.AllowOpenDir(CCG_DATADIR"/scripts/global");
			}
			else if(string("--lang")==argv[arg] && arg < argc-1)
			{
				lang=argv[++arg];
				Localization::SetLanguage(lang);
				Localization::ReadDictionary(CCG_DATADIR"/lib/dictionary.client");
			}
			else if(string("--load")==argv[arg] && arg < argc-1)
			{
				string file=CCG_DATADIR;
				file+="/xml/";
				file+=argv[++arg];
				cout << "Loading " << file << endl;
				Database::game.ReadFile(file);
				if(lang!="en")
				{
					string df=CCG_DATADIR"/lib/"+Database::game.Gamedir()+"/dictionary.client";
					if(FileExist(df))
						Localization::ReadDictionary(df);
				}
				for(int i=0; i<Database::game.CardSets(); i++)
				{
					file=CCG_DATADIR;
					file+="/xml/";
					file+=Database::game.Gamedir();
					file+="/";
					file+=Database::game.CardSet(i);
					cout << "Loading " << file << endl;
					Database::cards.AddCards(file);
				}
				Evaluator::savedir+="/";
				Evaluator::savedir+=Database::game.Gamedir();
				cout << "Using save dir " << Evaluator::savedir << endl;

				security.AllowOpenDir(CCG_DATADIR"/decks/"+Database::game.Gamedir());
				security.AllowOpenDir(CCG_DATADIR"/scripts/"+Database::game.Gamedir());
				security.AllowOpenDir(string(getenv("HOME"))+"/.gccg/"+Database::game.Gamedir()+"/import");
				security.AllowOpenDir(string(getenv("HOME"))+"/.gccg/"+Database::game.Gamedir()+"/export");
				Evaluator::execdir=CCG_DATADIR;
				Evaluator::execdir+="/scripts/";
				Evaluator::execdir+=Database::game.Gamedir();
				cout << "Using exec dir " << Evaluator::execdir << endl;
			}
			else
			{
				ifstream f(argv[arg]);
				cout << "Running " << argv[arg] << endl;
				if(!f)
				{
					cerr << "cannot open "+string(argv[arg]) << endl;
					return 2;
				}
				
				try
				{
					string program;
					string line;
					
					while(f)
					{
						if(Evaluator::quitsignal)
							break;
						line=readline(f);
						if(line.length() && line[0]=='#')
							continue;
						
						program+=line;
						program+="\n";
					}

					P(program);
				}
				catch(Error::General e)
				{
					cerr << e.Message() << endl;
					return 1;
				}
			}
			arg++;
		}
		
		Shell sh;
		sh.main(P);
	}
	catch(Error::General e)
	{
		cerr << e.Message() << endl;
		return 1;
	}
        catch(exception& e)
        {
            cerr << "Unhandled exception: " << e.what() << endl;
        }
}
