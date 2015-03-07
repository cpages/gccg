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
#ifndef PARSER_FUNCTIONS_H
#define PARSER_FUNCTIONS_H

/// Evaluate given function for each vector argument and return a list of results.
#define VECTORIZE(fun) 	if(args.IsList()) {	const Data& L=args; \
	Data ret; ret.MakeList(L.Size()); for(size_t i=0; i<L.Size(); i++) \
	ret[i]=fun(L[i]); return ret; }

/// Vectorize with respect to 2nd argument.
#define VECTORIZE_SECOND(fun) 	if(args.IsList(2) && args[1].IsList()) \
    {	Data _arg; _arg.MakeList(2); _arg[0]=args[0]; \
	const Data& L=args[1]; \
	Data ret; ret.MakeList(L.Size()); for(size_t i=0; i<L.Size(); i++) \
	{_arg[1]=L[i]; ret[i]=fun(_arg);} return ret; }

/// Same as VECTORIZE, but discard all NULLs from the list of results.
#define VECTORIZE_NONNULL(fun) 	if(args.IsList()) {	const Data& L=args; \
	Data ret,tmp; ret.MakeList(); for(size_t i=0; i<L.Size(); i++) \
	{tmp=fun(L[i]); if(!tmp.IsNull()) ret.AddList(tmp);} return ret; }

/// Throw an error if stack bigger than 1024 entries.
#define CHECK_STACK_OVERFLOW if(argument_stack.size() > 1024) { \
    stacktrace(Null); \
    cerr << "Parser<Application>::EvalAtom(string&): stack overflow" << endl; \
	while(argument_stack.size()) \
		argument_stack.pop(); \
	abort(); }

namespace Evaluator
{
	// Standard library.
	Data ReadLiteral(const char*& src);
	Data ReadBinary(ifstream & F);
	Data _safemode(const Data& arg);
	Data array(const Data& arg);
	Data copy(const Data& arg);
	Data count(const Data& arg);
	Data date(const Data& args);
	Data find(const Data& args);
	Data first(const Data& arg);
	Data flatten(const Data& arg);
	Data format(const Data& arg);
	Data has_entry(const Data& arg);
	Data head(const Data& arg);
	Data index(const Data& arg);
	Data join(const Data& arg);
	Data last(const Data& arg);
	Data lc(const Data& args);
	Data left(const Data& arg);
	Data length(const Data& arg);
	Data max(const Data& arg);
	Data min(const Data& arg);
	Data print(const Data& arg);
	Data println(const Data& arg);
	Data quit(const Data& arg);
	Data random(const Data& arg);
	Data randomize(const Data& arg);
	Data reverse(const Data& arg);
	Data right(const Data& arg);
	Data seq(const Data& arg);
	Data shuffle(const Data& arg);
	Data sleep(const Data& args);
	Data sort(const Data& arg);
	Data split(const Data& arg);
	Data substr(const Data& arg);
	Data tail(const Data& arg);
	Data time(const Data& args);
	Data toint(const Data& arg);
	Data toreal(const Data& arg);
	Data tostr(const Data& arg);
	void tobinary(const Data& arg, ofstream & F);
	Data toval(const Data& arg);
	Data type_of(const Data& arg);
	Data uc(const Data& args);
	Data ucfirst(const Data& args);
	Data unique(const Data& args);
	Data values(const Data& arg);

	// Net library.
	Data net_client_ip(const Data& arg);
	Data net_client_name(const Data& arg);
	Data net_close(const Data& arg);
	Data net_connect(const Data& arg);
	Data net_create_server(const Data& arg);
	Data net_get(const Data& arg);
	Data net_isopen(const Data& arg);
	Data net_send(const Data& arg);
	Data net_server_close(const Data& arg);
	Data net_server_get(const Data& arg);
	Data net_server_isopen(const Data& arg);
	Data net_server_send(const Data& arg);
	Data net_server_send_all(const Data& arg);

	// Card library.
	Data attrs(const Data& args);
	Data canonical_name(const Data& args);
	Data card_attr(const Data& args);
	Data card_back(const Data& args);
	Data decks(const Data& args);
	Data fuzzy_images(const Data& args);
	Data game_data(const Data& args);
	Data game_option(const Data& args);
	Data images(const Data& args);
	Data inline_images(const Data& args);
	Data load_deck(const Data& args);
	Data name(const Data& args);
	Data rarities(const Data& args);
	Data rules(const Data& args);
	Data save_deck(const Data& args);
	Data set_data(const Data& args);
	Data set_of(const Data& args);
	Data sets(const Data& args);
	Data sort_by(const Data& args);
	Data text(const Data& args);
#ifdef USE_SQUIRREL
	// Squirrel interface library
	//
	// This doesn't compile for some reason,
	// but I don't think it's strictly needed.
	/*
	Data start_sq(const Data& args);
	Data stop_sq(const Data& args);
	Data call_sq(const Data& args);
	Data eval_sq(const Data& args);
	Data get_sq(const Data& args);
	Data load_sq(const Data& args);
    */
#endif
}

#endif
