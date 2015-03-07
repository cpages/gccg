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

#include <ctype.h>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#if !defined(__BCPLUSPLUS__) && !defined(_MSC_VER)
# include <sys/time.h>
# include <unistd.h>
#endif
#include <math.h>
#include "parser.h"
#include "parser_functions.h"
#include "localization.h"
#ifdef WIN32
#include <windows.h>
#undef max
#undef min
#endif

// binary element types
#define GCCG_NULL 0x01
#define GCCG_STRING 0x02
#define GCCG_INT 0x03
#define GCCG_DOUBLE 0x04
#define GCCG_LIST 0x05

// binary buff size
#define GCCG_RBBUF 1024


using namespace std;

//
// Internal functions
// ------------------

namespace Evaluator
{
	map<string,Data (*)(const Data& )> external_function;

	/// typeof(e) - Return the type of $e$ as a string {\tt null},
	/// {\tt integer}, {\tt string}, {\tt list} or {\tt real}.
	Data type_of(const Data& arg)
	{
		if(arg.IsInteger())
			return Data("integer");
		if(arg.IsString())
			return Data("string");
		if(arg.IsList())
			return Data("list");
		if(arg.IsReal())
			return Data("real");

		return Data("null");
	}

	/// print(e) - Print the value of expression $e$ to the standard
	///   output. List members are separeated by spaces.
	Data print(const Data& arg)
	{
		cout << arg << flush;
		return Data();
	}

	/// println(e) - Same as {\tt print}, but a new line is appended.
	Data println(const Data& arg)
	{
		cout << arg << endl << flush;
		return Data();
	}

    /// copy(e,n) - Create a list with element $e$ repeated $n$ times. If
    /// $n\le 0$, return empty list. Return {\tt NULL}, if arguments are invalid.
	Data copy(const Data& arg)
	{
		if(!arg.IsList(2) || !arg[1].IsInteger())
			return Null;

		Data ret;

		int n=arg[1].Integer();
		
		if(n <= 0)
			ret.MakeList(0);
		else
		{
			ret.MakeList(n);
			for(size_t i=0; i<ret.Size(); i++)
				ret[i]=arg[0];
		}

		return ret;
	}

	/// first(L) - If $L$ is a non-empty list, return the first element of
	/// the list. If it is a string, return the first character as a
	/// string. Otherwise {\tt NULL}.
	Data first(const Data& arg)
	{
		if(arg.IsList())
		{
			const Data& l=arg;
			if(l.Size())
				return l[0];
		}
		else if(arg.IsString())
		{
			string s=arg.String();
				
			if(s.length())
				return s.substr(0,1);
			else
				return string("");
		}
		else
			return Null;

		return Data();
	}

	/// last(L) - If $L$ is a non-empty list, return the last element of
	/// the list. If it is a string, return the last character as a a string.
	Data last(const Data& arg)
	{
		if(arg.IsList())
		{
			const Data& l=arg;
			if(l.Size())
				return l[l.Size()-1];
		}
		else if(arg.IsString())
		{
			string s=arg.String();
				
			if(s.length())
				return s.substr(s.length()-1,1);
			else
				return string("");
		}
		else
			return Null;

		return Data();
	}

	/// random(n) - Return random number between $0$ and $n-1$. If $n$
	/// is not an integer, return {\tt NULL}.
	Data random(const Data& arg)
	{
		if(arg.IsList())
		{
			const Data& l=arg;
			if(l.Size())
				return l[better_rand(l.Size())];
		}
		else if(arg.IsInteger())
		{
			return better_rand(arg.Integer());
		}
		else
			return Null;

		return Data();
	}

	/// randomize(e) - Initialize random number generator with seed
	/// $e$ if $e$ is integer. Otherwise initialze it with microsecond
	/// time value taken from system clock. Return value is the seed.
	Data randomize(const Data& arg)
	{
		int seed;
			
		if(arg.IsInteger())
			seed=arg.Integer();
		else
		{
#if defined(WIN32) || defined(__WIN32__)
		   seed=(int)clock();
#else
			timeval t;
			gettimeofday(&t,0);
			seed=t.tv_usec;
#endif
		}
		srand((unsigned) seed);

		return Data(seed);
	}

	/// shuffle(L) - Return a randomly shuffled version of list $L$.
	Data shuffle(const Data& arg)
	{
		if(arg.IsList())
		{
			Data ret;
			ret.MakeList(arg.Size());

			for(size_t i=0; i<arg.Size(); i++)
				ret[i]=arg[i];
			
			if(ret.Size() > 1)
			{
				for(size_t i=0; i<ret.Size(); i++)
					swap(ret[i],ret[i + better_rand(ret.Size() - i)]);

			}
			return ret;
		}
		else
			return Null;

		return Data();
	}

	/// tail(L) - Return all members of the list $L$ except the
	/// first. If $L$ have one or zero members, return empty list. If
	/// $L$ is not a list, return {\tt NULL}.
	Data tail(const Data& arg)
	{
		Data ret;

		if(arg.IsList())
		{
			const Data& l=arg;
			if(l.Size() <= 1)
			{
				ret.MakeList();
			}
			else
			{
				ret.MakeList(l.Size()-1);
				for(size_t i=0; i<l.Size()-1; i++)
					ret[i]=l[i+1];
			}
		}
		else
			return Null;

		return ret;
	}

	/// head(L) - Return all member of the list $L$ except the
	/// last. If $L$ have one or zero members, return empty list. If
	/// $L$ is not a list, return {\tt NULL}.
	Data head(const Data& arg)
	{
		Data ret;

		if(arg.IsList())
		{
			const Data& l=arg;
			if(l.Size() <= 1)
			{
				ret.MakeList();
			}
			else
			{
				ret.MakeList(l.Size()-1);
				for(size_t i=0; i<l.Size()-1; i++)
					ret[i]=l[i];
			}
		}
		else
			return Null;

		return ret;
	}

	/// index(L_1,L_2) - Select members of list $L_1$ using list $L_2$
	/// as index. If any member $i$ of $L_2$ is not integer or $i < 0$
	/// or $i \ge |L_1|$, error is thrown. The resulting list has the
	/// same number of elements as $L_2$, each element corresponding
	/// to indexing member in $L_2$.
	Data index(const Data& arg)
	{
		Data ret;

		if(!arg.IsList(2) || !arg[0].IsList() || !arg[1].IsList())
			ArgumentError("index",arg);

		ret.MakeList();
		const Data &k1=arg[0];
		for(size_t i=0; i<arg[1].Size(); i++)
		{
			const Data &k2=arg[1][i];
			if(!k2.IsInteger())
				throw LangErr("index(const Data& )","Bad index "+tostr(k2).String());
			size_t n=k2.Integer();
			if(n >= k1.Size())
				throw LangErr("index(const Data& )","Index "+tostr(k2).String()+" out of range");

			ret.AddList(k1[n]);
		}
		
		return ret;
	}
	
	/// tobinary(e) - Convert any element to binary stream.
	void tobinary(const Data& arg, ofstream & F)
	{
		int i;
		int sz;
		unsigned char ch;
		double d;
		if(arg.IsString()) {
			const string& s=arg.String();
			ch=GCCG_STRING;
			sz=s.size()+1;
			F.write((char*)&ch, 1);
			F.write((char*)&sz, sizeof(int));
			F.write((char*)s.c_str(), sz);
		}
		else if(arg.IsNull()) {
			ch=GCCG_NULL;
			F.write((char*)&ch, 1);
		}
		else if(arg.IsReal()) {
			ch=GCCG_DOUBLE;
			d=arg.Real();
			F.write((char*)&ch, 1);
			F.write((char*)&d, sizeof(double));
		}
		else if(arg.IsInteger()) {
			ch=GCCG_INT;
			i=arg.Integer();
			F.write((char*)&ch, 1);
			F.write((char*)&i, sizeof(int));
		}
		else if(arg.IsList()) {
			const Data& L=arg;
			ch=GCCG_LIST;
			sz=L.Size();
			F.write((char*)&ch, 1);
			F.write((char*)&sz, sizeof(int));

			Data elem;
			for(size_t i=0; i<L.Size(); i++) {
				tobinary(L[i], F);
			}
		}

		if ( !F )
			throw LangErr("tobinary", "Write error");
	}

	/// tostr(e) - Convert any element to string. This string is in
	/// such format that it produces element $e$ when evaluated.
	Data tostr(const Data& arg)
	{
		if(arg.IsString())
		{
			const string& s=arg.String();
			string ret;

			for(size_t i=0; i<s.length(); i++)
			{
				if(s[i]=='\\')
					ret+="\\\\";
				else if(s[i]=='\n')
					ret+="\\n";
				else if(s[i]=='"')
					ret+="\\\"";
				else
					ret+=s[i];
			}
			
			return Data("\""+ret+"\"");
		}
		if(arg.IsNull())
			return Data("NULL");		
		if(arg.IsReal())
		{
			static char parser_buffer[128];
#ifdef WIN32
			sprintf(parser_buffer,"%.10f",arg.Real());
#else
			snprintf(parser_buffer,127,"%.10f",arg.Real());
#endif
			return Data(parser_buffer);
		}
		if(arg.IsInteger())
			return Data(ToString(arg.Integer()));
		
		if(arg.IsList())
		{
			string s="(";
			const Data& L=arg;

			Data elem;
			for(size_t i=0; i<L.Size(); i++)
			{
				if(i)
					s+=",";
				s+=tostr(L[i]).String();
			}
			if(L.Size() <= 1)
				s+=",";
			s+=")";

			return s;
		}
		
		return Data();
	}

	/// seq(n_1,n_2) - Generate sequence $(n_1,n_1+1,...,n_2)$. Return
	/// {\tt NULL} if $n_1 > n_2$ or if arguments are not integers.
	Data seq(const Data& arg)
	{
		if(!arg.IsList(2) || !arg[0].IsInteger() || !arg[0].IsInteger())
			return Null;

		int n1=arg[0].Integer();
		int n2=arg[1].Integer();

		if(n1 > n2)
			return Null;

		Data ret;
		ret.MakeList(size_t(n2-n1+1));
		for(int i=n1; i<=n2; i++)
			ret[i-n1]=i;

		return ret;
	}

	/// array(d_1,...,d_n) - This function creates list of lists
	/// structure which is suitable for array like indexing. Dimension
	/// of the array is $n$ and each $d_i$ denotes the size of the
	/// $i$th dimension of the array. Initial values of the list are
	/// {\tt NULL}.
	Data array(const Data& arg)
	{
		Data ret;

		if(arg.IsInteger())
		{
			int sz=arg.Integer();
			if(sz<=0)
				throw LangErr("array(const Data& )","Bad dimension "+tostr(arg).String());

			ret.MakeList(sz);
				
			return ret;
		}

		if(arg.IsList())
		{
			const Data& dim=arg;
			if(dim.Size()==0)
				ArgumentError("array",arg);

			Data a;

			for(int i=dim.Size()-1; i>=0; i--)
			{
				if(dim[i].Integer() <= 0)
					ArgumentError("array",arg);

				ret.MakeList(dim[i].Integer(),a);
				a=ret;
			}				
		}
		else
			ArgumentError("array",arg);

		return ret;
	}

	/// length(e) - If $e$ is a string or a list, return length of
	/// it. Otherwise return -1.
	Data length(const Data& arg)
	{
		if(arg.IsList())
			return int(arg.Size());
		else if(arg.IsString())
			return int(arg.String().length());

		return -1;
	}
	
	/// substr(s,p,z) - Return substring of $s$ starting at position
	/// $p$ and having a length of $z$. If $z$ is not given, return
	/// substring of $s$ starting at position $p$ until end of the
	/// string.
	Data substr(const Data& arg)
	{
		string str;
		int pos=0;
		int size=0;

		if(arg.IsList(2))
		{
			if(!arg[0].IsString() || !arg[1].IsInteger())
				ArgumentError("substr",arg);

			str=arg[0].String();
			pos=arg[1].Integer();
			size=str.length();
		}
		else if(arg.IsList(3))
		{
			if(!arg[0].IsString() || !arg[1].IsInteger() || !arg[2].IsInteger())
				ArgumentError("substr",arg);

			str=arg[0].String();
			pos=arg[1].Integer();
			size=arg[2].Integer();
		}
		else
			ArgumentError("substr",arg);
			
		if(pos < 0 || size < 0)
			ArgumentError("substr",arg);
			
		if(size_t(pos) > str.length())
			return string();
			  
		return str.substr(pos,size);
	}

	/// sort(L) - Return list as sorted according to {\tt <}
	/// operator.
	Data sort(const Data& arg)
	{
		if(!arg.IsList())
			ArgumentError("sort",arg);

		Data ret=arg;
		ret.Sort();

		return ret;
	}

	/// flatten(L) - If $L$ is not a list, return a list containing
	/// $L$ as an element. Otherwise destroy sublist structures and
	/// leave elements untouched.
	Data flatten(const Data& arg)
	{
		bool reflatten=false;
		Data ret;

		ret.MakeList();
		if(!arg.IsList())
			ret.AddList(arg);
		else
		{
			for(size_t i=0; i<arg.Size(); i++)
			{
				if(arg[i].IsList())
				{
					const Data& sublist=arg[i];
					
					for(size_t j=0; j<sublist.Size(); j++)
					{
						ret.AddList(sublist[j]);
						if(sublist[j].IsList())
							reflatten=true;
					}
				}
				else
					ret.AddList(arg[i]);
			}
		}

		if(reflatten)
			return flatten(ret);

		return ret;
	}

	/// quit(n) - Throw Quit exception with optional exit code $n$.
	Data quit(const Data& arg)
	{
		if(!arg.IsNull() && !arg.IsInteger())
			ArgumentError("quit",arg);
		if(arg.IsInteger())
			throw Error::Quit(arg.Integer());
			
		throw Error::Quit();
	}

	/// count(e,L) - Count number of elements $e$ in list $L$.
	Data count(const Data& arg)
	{
		if(!arg.IsList(2) || !arg[1].IsList())
			return Null;

		const Data& L=arg[1];
		const Data& e=arg[0];

		int n=0;
		for(int i=L.Size()-1; i>=0; i--)
		{
			if(e==L[i])
				n++;
		}

		return n;
	}

	/// reverse(L) - Return a list $L$ reversed or throw an exception
	/// if $L$ is not a list.
	Data reverse(const Data& arg)
	{
		if(!arg.IsList())
			ArgumentError("reverse",arg);

		Data ret;
		ret.MakeList(arg.Size());
		for(int j=0,i=arg.Size()-1; i >= 0; i--,j++)
			ret[j]=arg[i];

		return ret;
	}
	
	/// has_entry(e,L) - Return 1 if dictionary $L$ has member $e$, 0
	/// otherwise. Throw exception, if $L$ is not a dictionary.
	Data has_entry(const Data& arg)
	{
		if(!arg.IsList(2) || !arg[1].IsList())
			ArgumentError("has_entry",arg);

		const Data& L=arg[1];
		int n=L.Size();
		const Data& search=arg[0];

		for(int i=0; i<n; i++)
			if(!L[i].IsList(2))
				throw LangErr("has_entry","invalid dictionary entry "+tostr(L[i]).String()+" in dictionary "+tostr(L).String());
			else if(L[i][0]==search)
				return 1;

		return 0;
	}

	/// max(L) - Return maximum member of the list $L$ with respect to
	/// < operator.
	Data max(const Data& arg)
	{
		if(!arg.IsList())
			return Null;

		const Data& L=arg;

		if(L.Size()==0)
			return Null;

		Data n;
		for(size_t i=0; i<L.Size(); i++)
			if(i==0 || L[i] > n)
				n=L[i];

		return n;
	}

	/// min(L) - Return minimum member of the list $L$ with respect to
	/// < operator.
	Data min(const Data& arg)
	{
		if(!arg.IsList())
			return Null;

		const Data& L=arg;

		if(L.Size()==0)
			return Null;

		Data n;
		for(size_t i=0; i<L.Size(); i++)
			if(i==0 || L[i] < n)
				n=L[i];

		return n;
	}

	/// left(s,n) - Return $n$ characters from the beginning of string
	/// $s$ or empty string if $n \le 0$. If $s$ is a list, then
	/// return $n$ members from the beginning of the list.
	Data left(const Data& arg)
	{
		if(!arg.IsList(2) || !(arg[0].IsString() || arg[0].IsList()) ||!arg[1].IsInteger())
			return Null;

		int n=arg[1].Integer();

		if(arg[0].IsString())
		{
			const string& s=arg[0].String();

			if(n <= 0)
				return string("");

			if(size_t(n) >= s.length())
				return s;

			return s.substr(0,n);
		}
		
		const Data& L=arg[0];
		if(n < 0)
			n=0;
		if((size_t)n > L.Size())
			n=L.Size();

		Data ret;
		ret.MakeList(n);

		for(size_t i=0; i<(size_t)n; i++)
			ret[i]=L[i];

		return ret;
	}

	/// right(s,n) - Return $n$ characters from the end of string $s$
	/// or empty string if $n \le 0$. If $s$ is a list, then return
	/// $n$ members from the end of the list.
	Data right(const Data& arg)
	{
		if(!arg.IsList(2) || !(arg[0].IsString() || arg[0].IsList()) ||!arg[1].IsInteger())
			return Null;

		int n=arg[1].Integer();

		if(arg[0].IsString())
		{

			const string& s=arg[0].String();

			if(n <= 0)
				return string("");

			if(size_t(n) >= s.length())
				return s;

			return s.substr(s.length()-n,n);
		}

		const Data& L=arg[0];
		if(n < 0)
			n=0;
		if((size_t)n > L.Size())
			n=L.Size();

		Data ret;
		ret.MakeList(n);

		for(size_t i=0,j=L.Size()-n; i<(size_t)n; i++,j++)
			ret[i]=L[j];

		return ret;
	}

	Data ReadBinary(ifstream & F)
	{
		Data ret;
		int i=0;
		double d=0;
		unsigned char ch;
		char buf[GCCG_RBBUF+1];
		string str;
		F.read((char*)&ch, 1);
		if ( !F ) throw LangErr("ReadBinary", "Cannot read element type");
		switch(ch) {
			case GCCG_NULL:
				return Null;

			case GCCG_STRING:
				F.read((char*)&i, sizeof(int)); if ( !F ) throw LangErr("ReadBinary", "Cannot read string length");
				buf[GCCG_RBBUF]=0;
				while ( i ) {
					F.read(buf, ::min(i, GCCG_RBBUF)); if ( !F ) throw LangErr("ReadBinary", "Cannot read string");
					i-=::min(i, GCCG_RBBUF);
					str+=buf;
				}
				return str;

			case GCCG_INT:
				F.read((char*)&i, sizeof(int));
				return i;

			case GCCG_DOUBLE:
				F.read((char*)&d, sizeof(double));
				return d;

			case GCCG_LIST:
				F.read((char*)&i, sizeof(int));
				ret.MakeList();

				if ( i < 0 ) {
					throw LangErr("ReadBinary","invalid count");
				}

				while ( i ) {
					ret.AddList(ReadBinary(F));
					i--;
				}

				return ret;

			default:
				throw LangErr("ReadBinary", "Invalid element type");
		}

	}

	Data ReadLiteral(const char*& src)
	{		
		while(*src && isspace(*src))
			src++;
		
		if(!*src)
		{
			return Null;
		}
		
		if(CheckFor("NULL",src))
		{
			return Null;
		}

		if(*src=='"' || *src=='\'')
		{
			string ret;
			ReadString(src,ret);

			return ret;
		}

		bool neg=false;
		bool dbl=false;
		
		if(*src=='-')
		{
			src++;
			neg=true;
		}

		if(*src >= '0' && *src <= '9')
		{
			const char* num;
			num=src;
			while((*src>='0' && *src<='9') || *src=='.')
			{
				if(*src=='.')
					dbl=true;
				src++;
			}

			if(dbl)
			{
				double ret;
				
				if(neg)
					ret=-1.0*atof(num);
				else
					ret=atof(num);

				return ret;
			}
			else
			{
				int ret;
				if(neg)
					ret = -1 * atoi(num);
				else
					ret =  atoi(num);

				return ret;
			}
		}

		if(*src=='(')
		{
			Data ret;
			ret.MakeList();

			while(*src && isspace(*src))
				src++;
			
			if(CheckFor("(,)",src))
				return ret;

			src++;
			
			ret.AddList(ReadLiteral(src));

			while(*src && isspace(*src))
				src++;
			
			if(CheckFor(",)",src))
				return ret;

		  again:

			while(*src && isspace(*src))
				src++;
			
			if(*src!=',')
				throw LangErr("ReadLiteral","missing , in "+string(src)+"'");
			src++;
			
			while(*src && isspace(*src))
				src++;

			if(*src==')')
			{
				src++;
				return ret;
			}
			
			ret.AddList(ReadLiteral(src));

			while(*src && isspace(*src))
				src++;
			
			if(*src==')')
			{
				src++;
				return ret;
			}
			
			goto again;
		}

		throw LangErr("ReadLiteral","invalid string: '"+string(src)+"'");
	}

	/// toval(e) - Convert string converted by {\tt tostr} back to
	/// appropriate value. If $e$ is not a string, return $e$.
	Data toval(const Data& arg)
	{
		if(!arg.IsString())
			return arg;

		string s=arg.String();
		const char *src=s.c_str();
		Data ret=ReadLiteral(src);

                while(*src && isspace(*src))
                    src++;

		if(*src!=0)
			ArgumentError("toval",arg);

		return ret;
	}
	
	/// values(D) - Return list of value elements of dictionary $D$ or
	/// throw error if $D$ is not a dictionary.  This functions also
	/// returns list of first elements of list containing only pairs.
	Data values(const Data& arg)
	{
		if(!arg.IsList())
			ArgumentError("values",arg);

		const Data& L=arg;
		Data ret;
		
		ret.MakeList(L.Size());
		for(size_t i=0; i<L.Size(); i++)
			if(!L[i].IsList(2))
				throw LangErr("values","argument is not a dictionary");
			else
				ret[i]=L[i][1];

		return ret;
	}
	
	/// split(s_1,s_2,d) - If $d$ is not givent Split the string $s_1$
	/// to pieces separated by the string $s_2$ and return pieces as a
	/// list of strings. If $s_2$ is empty string, split the string to
	/// single characters. If $d=1$ is given, add delimiters too in the
	/// resulting list.
	Data split(const Data& arg)
	{
		if(!(arg.IsList(2) || arg.IsList(3))  || !arg[0].IsString() || !arg[1].IsString())
			ArgumentError("split",arg);

		bool add_delim=false;
		if(arg.IsList(3))
		{
			if(!arg[2].IsInteger())
				ArgumentError("split",arg);
			else
				add_delim=(arg[2].Integer() == 1);
		}

		string s1=arg[0].String();
		const string& s2=arg[1].String();
		Data ret;

		if(s2=="")
		{
			char e[2];
			e[1]=0;
			ret.MakeList(s1.length());
			for(size_t i=0; i<s1.length(); i++)
			{
				e[0]=s1[i];
				ret[i]=e;
			}

			return ret;
		}
	   
		
		size_t n=0,f;

		ret.MakeList();
		while(1)
		{
			f=s1.find(s2,n);
			if(f > s1.length())
			{
				if(n < s1.length())
					ret.AddList(s1.substr(n,s1.length()));
				break;
			}
			if(f != n)
				ret.AddList(s1.substr(n,f-n));
			n=f+s2.length();
			if(add_delim)
				ret.AddList(s2);
		}
		
		return ret;
	}

	/// join(e,s) - If $e$ is a list of strings and $s$ is not given,
	/// concatenate the strings together and return the resulting
	/// string. With $s$, add a string
	/// $s$ between list members.
	Data join(const Data& arg)
	{
		if(!arg.IsList())
			ArgumentError("join",arg);

		const Data* L;
		string j;
		if(arg.IsList(2) && arg[0].IsList() && arg[1].IsString())
		{
			j=arg[1].String();
			L=&arg[0];
		}
		else
			L=&arg;

		string s;
		for(size_t i=0; i<L->Size(); i++)
		{
			if(i)
				s+=j;
			if((*L)[i].IsString())
				s+=(*L)[i].String();
			else if((*L)[i].IsInteger() || (*L)[i].IsReal())
				s+=(tostr((*L)[i])).String();
			else
				ArgumentError("join",arg);
		}

		return s;
	}

	/// toint(e) - If $e$ is an integer return it unchanged. If $e$ is
	/// a string, convert value to integer. If $e$ is a real, round to
	/// nearest integer and return it. Otherwise, return 0.
	Data toint(const Data& arg)
	{
		if(arg.IsInteger())
			return arg;
		else if(arg.IsString())
			return atoi(arg.String().c_str());
		else if(arg.IsReal())
			return int(arg.Real()+0.5);

		return 0;
	}

	/// toreal(e) - If $e$ is an integer or a real return it as a
	/// real. If $e$ is a string, convert value to real. Otherwise,
	/// return 0.0.
	Data toreal(const Data& arg)
	{
		if(arg.IsInteger())
			return double(arg.Integer());
		else if(arg.IsString())
			return atof(arg.String().c_str());
		else if(arg.IsReal())
			return arg;

		return 0.0;
	}
	
	/// format(f,e) - Return C-printf formatted string of a real
	/// number $f$.
	Data format(const Data& arg)
	{
		if(!arg.IsList(2) || !arg[0].IsString() || (!arg[1].IsReal() && !arg[1].IsInteger()))
			ArgumentError("format",arg);

		string format=arg[0].String();
		const Data& d=arg[1];
		double num;
		
		if(d.IsReal())
			num=d.Real();
		else
			num=double(d.Integer());
		
		static char parser_buffer[128];
#ifdef WIN32
		sprintf(parser_buffer,format.c_str(),num);
#else
		snprintf(parser_buffer,127,format.c_str(),num);
#endif
		return Data(parser_buffer);
	}
	
	/// lc(s) - Convert string $s$ to lower case.
	Data lc(const Data& args)
	{
		VECTORIZE(lc);

		if(!args.IsString())
			return Null;

		string s=args.String();
                bool convert=true;
		for(size_t i=0; i<s.length(); i++)
                {
                    if(s[i]=='{')
                        convert=false;
                    if(s[i]=='}')
                        convert=true;

                    if(convert)
			s[i]=tolower(s[i]);
                }

		return s;
	}

	/// uc(s) - Convert string $s$ to upper case.
	Data uc(const Data& args)
	{
		VECTORIZE(uc);

		if(!args.IsString())
			return Null;

		string s=args.String();
                bool convert=true;
		for(size_t i=0; i<s.length(); i++)
                {
                    if(s[i]=='{')
                        convert=false;
                    if(s[i]=='}')
                        convert=true;

                    if(convert)
                        s[i]=toupper(s[i]);
                }

		return s;
	}

	/// ucfirst(s) - Convert the first letter of the string $s$ to
	/// upper case.
	Data ucfirst(const Data& args)
	{
		VECTORIZE(ucfirst);

		if(!args.IsString())
			return Null;

		string s=args.String();
		if(s.length())
			s[0]=toupper(s[0]);

		return s;
	}

	/// sleep(t) - Wait for $t$ seconds.
	Data sleep(const Data& args)
	{
		if(!args.IsInteger())
			ArgumentError("sleep",args);
#if defined(WIN32) || defined(__WIN32__)
		Sleep(args.Integer()*1000);
#else
		::sleep(args.Integer());
#endif
		return Null;
	}
	
	/// find(e,L) - Return index of the first instance of element $e$
	/// in a list $L$ or {\tt NULL} if not found. Alternatively find the
	/// starting position of the first instance of substring $e$ in a
	/// string $L$.
	Data find(const Data& args)
	{
		if(args.IsList(2) && args[0].IsString() && args[1].IsString())
		{
		    const string& f=args[0].String();
		    const string& s=args[1].String();

		    if(f=="")
			return 0;

		    size_t i=s.find(f);
		    if(i < s.length())
			return (int)i;
		    else
			return Null;
		}
		
		if(!args.IsList(2) || !args[1].IsList())
			return Null;

		const Data& e=args[0];
		const Data& L=args[1];
		for(size_t i=0; i<L.Size(); i++)
			if(L[i]==e)
				return int(i);
		
		return Null;
	}

	/// unique(L) - Return unique elements of sorted list $L$.
	Data unique(const Data& args)
	{
		if(!args.IsList())
			return Null;

		Data ret,last;
		ret.MakeList();
		
		for(size_t i=0; i<args.Size(); i++)
		{
			if(i==0 || args[i]!=last)
				ret.AddList(args[i]);
			
			last=args[i];
		}
		
		return ret;
	}

	/// date() - Return current day in {\tt \%d.\%m.\%y} format.
	Data date(const Data& args)
	{
		if(!args.IsNull())
			ArgumentError("date",args);

		char buf[15];
		time_t t=::time(0);
		strftime(buf,14,"%d.%m.%Y", localtime(&t));

		return string(buf);
	}

	/// time() - Return current time in 24H clock format {\tt \%h:\%m:\%s}.
	Data time(const Data& args)
	{
		if(!args.IsNull())
			ArgumentError("time",args);

		char buf[50];
		time_t t=::time(0);
		strftime(buf,49,"%H:%M:%S", localtime(&t));

		return string(buf);
	}

	/// replace(L,s,d) - Replace object L or members of list L having value s with value d.
	Data replace(const Data& args)
	{
		if(!args.IsList(3))
			ArgumentError("replace",args);
		
		if(args[0].IsList())
		{
			Data ret(args[0]);

			for(size_t i=0; i<ret.Size(); i++)
			{
				if(ret[i]==args[1])
					ret[i]=args[2];
			}

			return ret;
		}
		else
		{
			if(args[0]==args[1])
				return args[2];
			else
				return args[0];
		}
	}

	/// joker_match(p,s) - Return 1 if string s matches to joker
	/// pattern p. If a list of strings is given instead of a single
	/// string, then each maching string is returned.
	Data joker_match(const Data& args)
	{
		if(!args.IsList(2) || !args[0].IsString())
			ArgumentError("joker_match",args);

		if(args[1].IsList())
		{
			Data ret;
			ret.MakeList();
			for(size_t i=0; i<args[1].Size(); i++)
				if(joker_match(Data(args[0],args[1][i])).Integer())
					ret.AddList(args[1][i]);

			return ret;
		}

		if(!args[1].IsString())
			ArgumentError("joker_match",args);
			
		return (int)JokerMatch(args[0].String(),args[1].String());
	}

	/// fuzzy(s) - Remove all accents from letters and drop spaces and special characters.
	Data fuzzy(const Data& args)
	{
		VECTORIZE(fuzzy);
		
		if(!args.IsString())
			return Null;

		return Fuzzify(args.String());
	}

	/// trim(s) - Remove spaces from the beginning and from the end string s.
	Data trim(const Data& args)
	{
		VECTORIZE(trim);
		
		if(!args.IsString())
			return Null;
		
		return Trim(args.String());
	}
	
	/// read_file(f) - Read a text file f and return list of strings
	/// representing lines of the text file. Return NULL if reading fails.
	Data read_file(const Data& args)
	{
		VECTORIZE(read_file);
			
		if(!args.IsString())
			return Null;
			
		string file=args.String();

		security.ReadFile(file);
			
		ifstream f(file.c_str());
		if(!f)
			return Null;
		
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

	/// read_file_raw(f) - Read a text file f and return content as a string.
    /// Return NULL if reading fails.
	Data read_file_raw(const Data& args)
	{
		VECTORIZE(read_file_raw);
			
		if(!args.IsString())
			return Null;
			
		string file=args.String();

		security.ReadFile(file);
			
		ifstream f(file.c_str(), ios_base::binary);
		if(!f)
			return Null;
		
		string s;
		string ret;
        char chr;

		while(!f.eof())
		{
            f.get(chr);
            if(f.eof())
                break;

            ret.push_back(chr);
		}

		return Data(ret);
	}

	/// write_file(file,lines) - Write text lines to the file.
	/// Return 1 if successful and NULL if fails.
	Data write_file(const Data& args)
	{
		if(!args.IsList(2) || !args[0].IsString() || !args[1].IsList())
			ArgumentError("write_file",args);
			
		string file=args[0].String();

		security.WriteFile(file);

 		for(size_t i=0; i<args[1].Size(); i++)
 			if(!args[1][i].IsString())
 				throw LangErr("write_file","invalid text line "+tostr(args[1][i]).String());
		
		ofstream f(file.c_str());
		if(!f)
			return Null;

 		for(size_t i=0; i<args[1].Size(); i++)
			f << args[1][i].String() << endl;

		f.close();
		
		return 1;
	}

	/// write_file_raw(file,data) - Write raw data to the file.
	/// Return 1 if successful and NULL if fails.
	Data write_file_raw(const Data& args)
	{
		if(!args.IsList(2) || !args[0].IsString() || !args[1].IsString())
			ArgumentError("write_file_raw",args);
			
		string file=args[0].String();

		security.WriteFile(file);
		
		ofstream f(file.c_str(), ios_base::binary);
		if(!f)
			return Null;

        f << args[1].String();

		f.close();
		
		return 1;
	}

	/// current_time() - Return the time in seconds since the Epoch as real number.
	Data current_time(const Data& args)
	{
		if(!args.IsNull())
			ArgumentError("current_time",args);
		
		return (double)::time(0);
	}

	/// diff_time(t1,t2) - Return the time difference from the moment
	/// $t2$ to the moment $t1$ given as seconds (since the Epoch).
	/// The return value is a quadruple ($d$,$h$,$m$,$s$), where $d$ is the
	/// number of days, $h$ is the number of hours, $m$ is the number of
	/// minutes and $s$ is the number of seconds. All return values are
	/// negative if the moment $t2$ is after the moment $t1$.
	Data diff_time(const Data& args)
	{
		if(!args.IsList(2) || (!args[0].IsReal() && !args[0].IsInteger()) || (!args[1].IsReal()  && !args[0].IsInteger()))
			ArgumentError("diff_time",args);

		bool neg=false;

		long t1,t2;
		
		if(args[0].IsReal())
			t1=(long)args[0].Real();
		else
			t1=(long)args[0].Integer();

		if(args[1].IsReal())
			t2=(long)args[1].Real();
		else
			t2=(long)args[1].Integer();
			
		long t=t1-t2;

		if(t < 0)
		{
			neg=true;
			t=-t;
		}
		
		int s,m,h,d;

		d=(int)t/(60*60*24);
		t=t-d*(60*60*24);
		h=(int)t/(60*60);
		t=t-h*(60*60);
		m=(int)t/60;
		t=t-m*60;
		s=(int)t;
			
		Data ret;
		ret.MakeList(4);
		ret[0]=s;
		ret[1]=m;
		ret[2]=h;
		ret[3]=d;

		if(neg)
		{
			ret[0]=-ret[0];
			ret[1]=-ret[1];
			ret[2]=-ret[2];
			ret[3]=-ret[3];
		}
		
		return ret;
	}

	/// languages(code) - Return a dictionary mapping ISO639 language
	/// codes to language names.
	Data languages(const Data& args)
	{
		map<string,string> codes=Localization::Languages();
		
		Data ret;
		ret.MakeList(codes.size());

		map<string,string>::const_iterator i;
		size_t j=0;
		
		for(i=codes.begin(); i!=codes.end(); i++)
		{
			ret[j].MakeList(2);
			ret[j][0]=(*i).first;
			ret[j][1]=(*i).second;
			j++;
		}
		
		return ret;
	}

	/// get_lang() - Return current localization language.
	Data get_lang(const Data& args)
	{
		if(!args.IsNull())
			ArgumentError("get_lang",args);

		return Localization::GetLanguage();
	}
	
	/// set_lang(code) - Select current language for localization.
	Data set_lang(const Data& args)
	{
		if(!args.IsString())
			ArgumentError("set_lang",args);

		string old=Localization::GetLanguage();
		string code=args.String();
		
		if(!Localization::IsLanguage(code))
			throw LangErr("set_lang()","invalid language code "+code);
		
		Localization::SetLanguage(code);

		return old;
	}
	
	/// L(msg,[arg1],[arg2]) - Localize a message.
	Data L(const Data& args)
	{
		if(args.IsString())
			return Localization::Message(args.String());
		if(args.IsList(2) && args[0].IsString() && args[1].IsString())
			return Localization::Message(args[0].String(),args[1].String());
		if(args.IsList(3) && args[0].IsString() && args[1].IsString() && args[2].IsString())
			return Localization::Message(args[0].String(),args[1].String(),args[2].String());

		return Null;
	}

	/// isdict(L) - Return 1 if L is a dictionary.
	Data isdict(const Data& args)
	{
		if(!args.IsList())
			return 0;

		for(size_t i=0; i<args.Size(); i++)
		{
			if(!args[i].IsList(2))
				return 0;
			if(i)
				if(args[i]<args[i-1] || args[i]==args[i-1])
					return 0;
		}

		return 1;
	}
	
	/// tr(s,a,b) - Replace every character in string a found from string s with b.
	Data tr(const Data& args)
	{
	    if(!args.IsList(3) || !args[0].IsString() 
	      || !args[1].IsString() || !args[2].IsString())
		ArgumentError("tr",args);

	    const string& s=args[0].String();
	    const string& a=args[1].String();
	    const string& b=args[2].String();
	    
	    string ret;

	    for(size_t i=0; i<s.length(); i++)
		if(a.find(s[i])!=string::npos)
		    ret=ret+b;
		else
		    ret=ret+s[i];

	    return ret;
	}

	/// strreplace(s,a,b) - Replace every instance of string a
	/// found from string s with b. Return NULL on if infinite recursive replacement.
	Data strreplace(const Data& args)
	{
	    if(!args.IsList(3) || !args[0].IsString() 
	      || !args[1].IsString() || !args[2].IsString())
		ArgumentError("tr",args);

	    const string& a=args[1].String();
	    const string& b=args[2].String();
	    
	    string ret=args[0].String();

	    if(a=="" || ret=="")
		return ret;

	    if(b.find(a)!=string::npos)
		return Null;

	    size_t i;
	    while((i=ret.find(a))!=string::npos)
		ret.replace(i,a.length(),b);

	    return ret;
	}

	/// hexencode(s) - Encode non-ascii characters.
	Data hexencode(const Data& args)
	{
		VECTORIZE(hexencode);

	    if(!args.IsString())
            ArgumentError("hexencode",args);


	    string s=args.String();
        string ret;
	    for(size_t i=0; i<s.length(); i++)
            ret+=ToHex(s[i]);

	    return Data(ret);
	}

	/// hexdecode(s) - Decode hexencoded() string.
	Data hexdecode(const Data& args)
	{
		VECTORIZE(hexdecode);

	    if(!args.IsString())
            ArgumentError("hexdecode",args);

	    string s=args.String();
        
        string ret;
	    for(size_t i=0; i<s.length(); i+=2)
            ret.push_back(ToChar(s.substr(i,2)));

	    return Data(ret);
	}

// Initialization code

	namespace Lib
	{
		class LibraryInitializer
		{
		public:
			LibraryInitializer()
			{
				external_function["L"]=&L;
				external_function["array"]=&array; 
				external_function["copy"]=&copy; 
				external_function["count"]=&count; 
				external_function["current_time"]=&current_time;
				external_function["date"]=&date;
				external_function["diff_time"]=&diff_time;
				external_function["find"]=&find; 
				external_function["first"]=&first; 
				external_function["flatten"]=&flatten; 
				external_function["format"]=&format; 
				external_function["fuzzy"]=&fuzzy;
				external_function["get_lang"]=&get_lang;
				external_function["has_entry"]=&has_entry;
				external_function["head"]=&head;
				external_function["hexdecode"]=&hexdecode;
				external_function["hexencode"]=&hexencode;
				external_function["index"]=&index;
				external_function["isdict"]=&isdict;
				external_function["join"]=&join;
				external_function["joker_match"]=&joker_match;
				external_function["languages"]=&languages;
				external_function["last"]=&last;
				external_function["lc"]=&lc;
				external_function["left"]=&left;
				external_function["length"]=&length; 
				external_function["max"]=&max; 
				external_function["min"]=&min; 
				external_function["print"]=&print; 
				external_function["println"]=&println; 
				external_function["quit"]=&quit;
				external_function["random"]=&random; 
				external_function["randomize"]=&randomize; 
				external_function["read_file"]=&read_file;
				external_function["read_file_raw"]=&read_file_raw;
				external_function["replace"]=&replace;
				external_function["reverse"]=&reverse;
				external_function["right"]=&right; 
				external_function["seq"]=&seq;
				external_function["set_lang"]=&set_lang;
				external_function["shuffle"]=&shuffle; 
				external_function["sleep"]=&Evaluator::sleep; 
				external_function["sort"]=&sort;
				external_function["split"]=&split;
				external_function["strreplace"]=&strreplace;
				external_function["substr"]=&substr;
				external_function["tail"]=&tail; 
				external_function["time"]=&time;
				external_function["toint"]=&toint;
				external_function["toreal"]=&toreal;
				external_function["tostr"]=&tostr;
				external_function["toval"]=&toval;
				external_function["tr"]=&tr;
				external_function["trim"]=&trim;
				external_function["typeof"]=&type_of;
				external_function["uc"]=&uc;
				external_function["ucfirst"]=&ucfirst;
				external_function["unique"]=&unique;
				external_function["values"]=&values;
				external_function["write_file"]=&write_file;
				external_function["write_file_raw"]=&write_file_raw;
			}
		};

		static LibraryInitializer initializer;
	}

}
