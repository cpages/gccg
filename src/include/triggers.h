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
#ifndef TRIGGERS_H
#define TRIGGERS_H

#include <string>
#include <map>

using namespace std;

namespace Evaluator
{
	/// Collection of strings with two keys. Usually used as a parser code storage.
	class Triggers
	{
		    /// Storage for strings.
			map<string,map<string,string> > code;
			
		  public:

			/// Append _code to trigger (key1,key2).
			void AddTrigger(const string& key1,const string& key2,const string& _code)
				{code[key1][key2]+=_code;}
			/// Return the code associated with key pair (key1,key2).
			const string& operator()(const string& key1,const string& key2)
				{return code[key1][key2];}
			/// Insert trigger rules from the file.
			void ReadFile(const string& filename);
	};
}

#endif
