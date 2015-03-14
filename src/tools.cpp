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
#include <sys/types.h>
#if defined(_MSC_VER)
# include "compat.h"
#else
# include <dirent.h>
#endif
#include "tools.h"
#include "error.h"
#include "carddata.h"

#ifdef __ANDROID__
#include <android/log.h>
#endif

using namespace std;

void log(const std::string& s)
{
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "gccg", "%s", s.c_str());
#else
    cout << s << endl;
#endif
}

string readline(istream& I)
{
    string ret;

    getline(I,ret);

    return ret;
}

string ToString(int i)
{
	string sign,s;
	char digit[2];
	digit[1]=0;
		
	if(i < 0)
	{
		i=-i;
		sign="-";
	}
	do
	{
		digit[0]=char(unsigned(i) % 10)+'0';
		i/=10;
		s=(digit+s);
		
	}while(i);

	return sign+s;
}

string ToLower(const string& s)
{
	string ret=s;

	for(size_t i=0; i<ret.length(); i++)
		ret[i]=tolower(ret[i]);

	return ret;
}

string Trim(const string& s_)
{
	string s=s_;
	while(s.length() && IsSpace(s[0]))
		s=s.substr(1);
	while(s.length() && IsSpace(s[s.length()-1]))
		s=s.substr(0,s.length()-1);

	return s;
}

bool FileExist(const string& filename)
{

	ifstream f(filename.c_str());

	if(f)
		return true;

	return false;
}

bool IsDirectory(const string& filename)
{
	DIR* d;

	d=opendir(filename.c_str());
	if(d)
	{
		closedir(d);
		return true;
	}
	
	return false;
}

static bool JokerMatch(const char* joker,const char* s)
{
	switch(*joker)
	{
	  case 0:
		  return *s==0;
	  case '*':
		  return JokerMatch(joker+1,s) || (*s && JokerMatch(joker,s+1));
	  default:
		  return *joker==*s && JokerMatch(joker+1,s+1);
	}
}

bool JokerMatch(const string& joker,const string& s)
{
	return JokerMatch(joker.c_str(),s.c_str());
}

string StripDirs(string s)
{
    size_t n=0;

    for(size_t i=0; i<s.length(); i++)
        if(s[i]=='/' || s[i]=='\\')
            n=i;

    if(n)
        n++;

    return s.substr(n);
}

string StripAccents(char c)
{
    string ret;
	// NOTE:
	//  this could all be sped up by counting on the ASCII/ISO sort order with a series of if's
	// if (c >= 192 && c <= 197)	c = 'A'; return;
	// if (c == 199)			c = 'C'; return;
	// if (c >= 200 && c <= 203)	c = 'E'; return;
	// 		etc.

	switch(c)
	{
	  case '�':	// 192
	  case '�':	// 193
	  case '�':	// 194
	  case '�':	// 195
	  case '�':	// 196
	  case '�':	// 197
		  ret="A";
		  break;

	  // 198 AE ligature
	  case '�':	// 198
		  ret="AE";
		  break;
	  case '�':
		  ret="ae";
		  break;

	  case '�':	// 199
		  ret="C";
		  break;

	  case '�':	// 200
	  case '�':	// 201
	  case '�':	// 202
	  case '�':	// 203
		  ret="E";
		  break;

	  case '�':	// 204
	  case '�':	// 205
	  case '�':	// 206
	  case '�':	// 207
		  ret="I";
		  break;

	  case '�':	// 208
		  ret="D";
		  break;

	  case '�':	// 209
		  ret="N";
		  break;

	  case '�':	// 210
	  case '�':	// 211
	  case '�':	// 212
	  case '�':	// 213
	  case '�':	// 214
		  ret="O";
		  break;

	  // !!! ??? !!!
	  case '�':	// 215
		  ret="*";
		  break;

	  case '�':	// 216
		  ret="O";
		  break;

	  case '�':	// 217
	  case '�':	// 218
	  case '�':	// 219
	  case '�':	// 220
		  ret="U";
		  break;

	  case '�':	// 221
		  ret="Y";
		  break;

	  // � 222
	  // � 223


	  case '�':	// 224
	  case '�':	// 225
	  case '�':	// 226
	  case '�': // 227
	  case '�':	// 228
	  case '�':	// 229
		  ret="a";
		  break;

	  // ae ligature (230)

	  case '�':	// 231
		  ret="c";
		  break;

	  case '�':	// 232
	  case '�':	// 233
	  case '�':	// 234
	  case '�':	// 235
		  ret="e";
		  break;

	  case '�':	// 236
	  case '�':	// 237
	  case '�':	// 238
	  case '�':	// 239
		  ret="i";
		  break;

	  case '�':	// 240 (d-diaresis)
		  ret="d";
		  break;

	  case '�':	// 241
		  ret="n";
		  break;

	  case '�':	// 242
	  case '�':	// 243
	  case '�':	// 244
	  case '�':	// 245
	  case '�':	// 246
		  ret="o";
		  break;

	  // !!! ??? !!!
	  case '�':	// 247
		  ret="/";
		  break;

	  case '�':	// 248
		  ret="o";
		  break;

	  case '�':	// 249
	  case '�':	// 250
	  case '�':	// 251
	  case '�':	// 252
		  ret="u";
		  break;

	  case '�':	// 253
	  case '�':	// 255
		  ret="y";
		  break;

	  // 254 �
	}

	return ret;
}

string Fuzzify(const string& s,bool no_tag)
{
	string ret;
			
	for(size_t i=0; i<s.length(); i++)
	{
		if(s[i]=='{')
		{
                    if(no_tag)
                    {
			while(i<s.length() && s[i]!='}')
                            i++;
                    }
                    else
                    {
			while(i<s.length() && s[i]!='}')
				ret+=s[i++];
			if(i<s.length() && s[i]=='}')
				ret+=s[i];
                    }
		}
		else if((s[i]>='0' && s[i]<='9') || (s[i]>='A' && s[i]<='Z') || (s[i]>='a' && s[i]<='z'))
			ret+=tolower(s[i]);
		else if(unsigned(s[i])>127)
                {
                    string r=StripAccents(s[i]);
                    for(size_t j=0; j<r.length(); j++)
                        ret+=tolower(r[j]);
                }
	}
	
	return ret;
}

static char HexDigit(int digit)
{
	if(digit < 0 || digit > 15)
		throw Error::Invalid("HexDigit(int)","illegal digit "+ToString(digit));
	if(digit < 10)
		return '0'+(char)digit;

	return 'a'+(char)digit-10;
}

string ToHex(unsigned char c)
{
	char s[3];
	
	s[0]=HexDigit((c&0xf0)>>4);
	s[1]=HexDigit(c&0x0f);
	s[2]=0;
	
	return s;
}

static unsigned int HexValue(unsigned char c)
{
	if(c>='0' && c<='9')
		return c-'0';
	if(c>='a' && c<='f')
		return c-'a'+10;

	throw Error::Invalid("HexValue(unsigned char)","invalid hex character");
}

char ToChar(const string& s)
{
	if(s.length()!=2)
		throw Error::Invalid("ToChar(int)","invalid string '"+s+"'");

	return (char)((HexValue(s[0])*16 + HexValue(s[1])) & 0xff);
}

string HexEncode(const string& s)
{
	string ret;
	for(size_t i=0; i<s.length(); i++)
		if(!isalnum(s[i]) || s[i]=='%')
			ret+="%"+ToHex(s[i]);
		else
			ret+=s[i];

	return ret;
}

string HexDecode(const string& s)
{
	string ret;
	
	for(size_t i=0; i<s.length(); i++)
		if(s[i]=='%')
		{
			ret+=ToChar(s.substr(i+1,2));
			i+=2;
		}
		else
			ret+=s[i];
	
	return ret;
}

string FindFile(string name,string cat,string alt)
{
    string fname;
    string gamedir=Database::game.Ready() ? Database::game.Gamedir() : "";

    name=StripDirs(name);

    if(gamedir!="")
    {
        fname=CCG_DATADIR"/../"+ToLower(gamedir)+"/"+cat+"/"+gamedir+"/"+name;
        if(FileExist(fname))
            return fname;
        if(alt!="")
        {
            fname=CCG_DATADIR"/../"+ToLower(gamedir)+"/"+alt+"/"+gamedir+"/"+name;
            if(FileExist(fname))
                return fname;
        }
    }

    if(gamedir!="")
    {
        fname=CCG_DATADIR"/"+cat+"/"+gamedir+"/"+name;
        if(FileExist(fname))
            return fname;
        if(alt!="")
        {
            fname=CCG_DATADIR"/"+alt+"/"+gamedir+"/"+name;
            if(FileExist(fname))
                return fname;
        }
    }

    fname=CCG_DATADIR"/"+cat+"/"+name;
    if(FileExist(fname))
        return fname;
    if(alt!="")
    {
        fname=CCG_DATADIR"/"+alt+"/"+name;
        if(FileExist(fname))
            return fname;
    }

    return "";
}
