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

#include <algorithm>
#include <fstream>
#include "data.h"
#include "triggers.h"
#include "parser.h"
#include "parser_functions.h"

namespace Evaluator
{
    Data Null;
    bool safemode=false;
    bool quitsignal=false;
    bool debug=false;
    string savedir,execdir;
    set<string> safe_exec;

    // General parser functions
    // ------------------------

    void GetVariable(const char*& _src,string& var)
    {
	register const char* src=_src;
	register char c;

	if(CheckFor("NULL",_src))
	{
	    var="";
	    return;
	}

	while((c=*src) && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c=='_' || c=='.' || (src != _src && c >= '0' && c <= '9')))
	    src++;

	var=string(_src,src);
	_src=src;
    }

    void GetVariable(const char*& _src)
    {
	register const char* src=_src;
	register char c;

	if(CheckFor("NULL",_src))
	{
	    return;
	}

	while((c=*src) && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c=='_' || c=='.' || (src != _src && c >= '0' && c <= '9')))
	    src++;

	_src=src;
    }

    bool IsVariable(const string& s)
    {
	register const char* src=s.c_str();
	register char c;

	if(*src==0 || *src=='.')
	    return false;

	if(CheckFor("NULL",src))
	{
	    return false;
	}

	while((c=*src) && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c=='_' || c=='.' || (src != s.c_str() && c >= '0' && c <= '9')))
	    src++;

	return (*src==0);
    }

    void ReadString(const char*& _src,string& var)
    {
	register const char* src=_src;
	char tmp[2];
	tmp[1]=0;

	if(*src==0)
	    throw LangErr("ReadString(const char*&,string&)","Empty string");

	char delim=*src;
	if(delim!='\'' && delim!='"')
	    throw LangErr("ReadString(const char*&,string&)","Missing ' or \"");
	src++;
	_src++;
		
	for(;;)
	{
	    if(*src==0)
		throw LangErr("ReadString(const char*&,string&)","Unterminated string atom: "+(string(_src).substr(0,30)));
	    if(*src=='\\' && delim == '"')
	    {
		src++;
		if(*src=='n')
		{
		    var+=string(_src,src-1);
		    var+="\n";
		    src++;
		    _src=src;
		}
		else
		{
		    var+=string(_src,src-1);
		    tmp[0]=*src;
		    var+=tmp;
		    src++;
		    _src=src;
		}
	    }
	    else if(*src == delim)
		break;
	    else
		src++;
	} 
	var+=string(_src,src);
	src++;
	_src=src;
    }

    void ReadString(const char*& _src)
    {
	register const char* src=_src;

	if(*src==0)
	    throw LangErr("ReadString(const char*&)","Empty string");

	char delim=*src;
	if(delim!='\'' && delim!='"')
	    throw LangErr("ReadString(const char*&)","Missing ' or \"");

	src++;
		
	for(;;)
	{
	    if(*src==0)
		throw LangErr("ReadString(const char*&)","Unterminated string atom: "+(string(_src).substr(0,30)));
	    if(*src=='\\' && delim == '"')
	    {
		src++;
		if(*src==0)
		    throw LangErr("ReadString(const char*&)","Unterminated string atom: "+(string(_src).substr(0,30)));
		src++;
	    }
	    else if(*src == delim)
	    {
		src++;
		break;
	    }
	    else
		src++;
	}
		
	_src=src;
    }

    void ReadParenthesis(const char*& _src,string& var)
    {
	register const char* src=_src;

	if(*src != '(')
	    throw LangErr("ReadParenthesis(const char*&,string&)","Missing '('");

	int parenth_open=1;
	src++;

	while(1)
	{
	    if(*src==0)
		throw LangErr("ReadParenthesis(const char*&,string&)","Missing ')'");

	    if(*src==')')
	    {
		parenth_open--;
		src++;
		if(parenth_open==0)
		    break;
	    }
	    else if(*src=='(')
	    {
		parenth_open++;
		src++;
	    }
	    else if(*src=='\'' || *src=='"')
	    {
		ReadString(src);
	    }
	    else
		src++;
	}

	var=string(_src,src);
	_src=src;
    }

    void ReadParenthesis(const char*& _src)
    {
	register const char* src=_src;

	if(*src != '(')
	    throw LangErr("ReadParenthesis(const char*&,string&)","Missing '('");

	int parenth_open=1;
	src++;

	while(1)
	{
	    if(*src==0)
		throw LangErr("ReadParenthesis(const char*&,string&)","Missing ')'");

	    if(*src==')')
	    {
		parenth_open--;
		src++;
		if(parenth_open==0)
		    break;
	    }
	    else if(*src=='(')
	    {
		parenth_open++;
		src++;
	    }
	    else if(*src=='\'' || *src=='"')
	    {
		ReadString(src);
	    }
	    else
		src++;
	}

	_src=src;
    }

    void ReadStatement(const char*& _src,string& var)
    {
	register const char* src=_src;

	ReadStatement(src);
	var=string(_src,src);
	_src=src;
    }

    void ReadStatement(const char*& _src)
    {
	register const char* src=_src;

	int braces=1;

	EatWhiteSpace(src);
	if(*src==0)
	    return;

	if(*src=='{')
	{
	    src++;
	    while(1)
	    {
		if(*src==0)
		    throw LangErr("ReadStatement(const char*&)","Missing '}'");
			
		if(*src=='{')
		{
		    braces++;
		    src++;
		}
		else if(*src=='}')
		{
		    braces--;
		    src++;
		    if(braces==0)
			break;
		}
		else if(*src=='\'' || *src=='"')
		{
		    ReadString(src);
		}
		else
		    src++;
	    }
	}
	else // single statement
	{
	    while(1)
	    {
		if(*src==0)
		    throw LangErr("ReadStatement(const char*&)","Missing ';'");
				
		if(*src==';')
		{
		    src++;
		    break;
		}
		else if(*src=='\'' || *src=='"')
		{
		    ReadString(src);
		}
		else
		    src++;
	    }
	}

	_src=src;
    }

    const char* CompressCode(const string& code)
    {
	static char* buffer=0;
	if(buffer)
	    delete buffer;
	buffer=new char[code.length()+1];

	if(!buffer)
	    throw Error::Memory("CompressCode(const string&)");

	register const char* src=code.c_str();
	register const char* tmp;
	register char* dst=buffer;
		
	while(*src)
	{
	    if(isspace(*src))
		src++;
	    else if(*src=='\'' || *src=='"')
	    {
		tmp=src;
		ReadString(src);
		while(tmp!=src)
		    *dst++ = *tmp++;
	    }
	    else if(*src=='d')
	    {
		*dst++=*src++;
		if(*src == 'e')
		{
		    *dst++=*src++;
		    if(*src == 'f')
		    {
			*dst++=*src++;
			if(*src == ' ')
			    *dst++=*src++;
		    }
		}	
	    }
	    else
		*dst++ = *src++;
	}

	*dst=0;
		
	return buffer;
    }

    void Triggers::ReadFile(const string& filename)
    {
	string content,line;
		
	ifstream f(filename.c_str());
	if(!f)
	    throw Error::IO("Triggers::ReadFile","Unable to open file '"+filename+"'");

	// Read file to string 'content' stripping comments.
	while(f)
	{
            line=readline(f);
            if(line[0]!='#')
            {
                content+=line;
                content+="\n";
            }
	}

	const char* sp=content.c_str();
	string key1,key2,type,code;

	// Parse string.
	while(1)
	{
	    EatWhiteSpace(sp);
	    if(*sp==0)
		break;
			
	    GetVariable(sp,key1);

	    if(key1=="trigger")
	    {
		EatWhiteSpace(sp);

		key1="";
		ReadString(sp,key1);

		EatWhiteSpace(sp);
				
		key2="";
		ReadString(sp,key2);
				
		EatWhiteSpace(sp);
				
		GetVariable(sp,type);
				
		if(type=="code")
		{
		    ReadStatement(sp,code);
		    AddTrigger(key1,key2,CompressCode(code));
		}
		else if(type=="as")
		{
		    string askey1,askey2;

		    EatWhiteSpace(sp);
		    ReadString(sp,askey1);
		    EatWhiteSpace(sp);
		    ReadString(sp,askey2);
		    EatWhiteSpace(sp);
					
		    AddTrigger(key1,key2,(*this)(askey1,askey2));
		}				
		else
		    throw LangErr("Triggers::ReadFile","unrecognized definition type '"+type+"'");
	    }
	    else
		throw LangErr("Triggers::ReadFile","Malformed trigger file '"+filename+"' (keyword '"+key1+"' unknown)");
			
	} // while(1)
    }

    string Substitute(const string& s,const Data& arg)
    {
	string ret;
	string value=tostr(arg).String();

	for(size_t i=0; i<s.length(); i++)
	    if(s[i]=='#')
		ret+=value;
	    else
		ret+=s[i];

	return ret;
    }
}
