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

#include <string>
#include <iostream>
#include <ctype.h>
#include <stdlib.h>

using namespace std;

#ifndef _TOOLS_H
#define _TOOLS_H

/// Read one line from the stream.
string readline(std::istream& I);
/// Convert an integer to string.
string ToString(int i);
/// Convert a string to lower case.
string ToLower(const string& s);

/// Check for given string 's'. Return true and advance '_src' ptr accordingly if found.
inline bool CheckFor(const char* s,const char*& _src,bool nonalnum=false)
{
	register const char* src=_src;

	while(*s)
		if(*s != *src)
			return false;
		else
			s++,src++;

 	if(nonalnum && isalnum(*src)) 
 		return false; 
	
	_src=src;

	return true;
}
/// Remove white space from the beginning of the string.
inline void EatWhiteSpace(register const char*& src)
{
	while(*src && isspace(*src))
		src++;
}
/// Alternate to isspace (debug version on windows will assert on non ANSI chars)
inline int IsSpace(int c) {return c == 0x09 || c == 0x0A || c == 0x0D || c == 0x20;}
/// Remove spaces from the beginning and from the end of the string.
string Trim(const string& s);
/// Remove directories from file path.
string StripDirs(string s);
/// Remove all accents from a letter.
string StripAccents(char c);
/// Remove all accents from letters, drop spaces and special
/// characters and convert to lower case. Special {tag} constructs are
/// preserved unless flag no_tag set.
string Fuzzify(const string& s,bool no_tag=false);

///  Return true if file exist.
bool FileExist(const string& filename);
/// Return true if file is a directory.
bool IsDirectory(const string& filename);

/// Return true if a string matches to a joker pattern.
bool JokerMatch(const string& joker,const string& s);

/// Convert a character to hexadecimal.
string ToHex(unsigned char c);
/// Convert a hexadecimal to character.
char ToChar(const string& s);
/// Encode non-alphanumeric characrers to %hex.
string HexEncode(const string& s);
/// Decode a string from hex encoded string.
string HexDecode(const string& s);
/// Find a file path from the system for a file belonging to the given category.
string FindFile(string name,string cat,string altcat="");
/// better rand for weak implenations of rand on lower bits
inline int better_rand(int range) {
	return range*((double)rand()/(double)((unsigned)RAND_MAX+1));
}
#endif
