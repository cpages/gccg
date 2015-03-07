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
#include "parser_functions.h"

namespace Evaluator
{
    Data* Data::InsertAt(int pos,const Data& object)
    {
	if(!IsList())
	    throw LangErr("Data::InsertAt","object is not a list");

	size_t i = size_t(pos);

	// note size_t is unsigned, so no need to check if i < 0
	if(i > vec.size())
	    throw LangErr("Data::InsertAt","invalid position");
			
	if(i==vec.size())
	{
	    vec.push_back(object);
	    return &vec[vec.size()-1];
	}
	else
	{
	    vec.insert(i,object);
	    return &vec[i];
	}
    }

    size_t Data::KeyLookup(const Data& key,bool& already_exist) const
    {
	size_t min=0;
	size_t max=vec.size();
	size_t i=0;

	already_exist=true;
		
	while(min != max)
	{
	    i=(max+min)/2;

	    if(!vec[i].IsList(2))
		throw LangErr("Data::KeyLookup","dictionary contains invalid entry '"+tostr(vec[i]).String()+"'");

	    if(key==vec[i][0])
		return i;

	    if(key < vec[i][0])
		max=i;
	    else
		min=i+1;
	}

	if(i < vec.size() && vec[i][0]==key)
	    return i;

	already_exist=false;

	return min;
    }

    Data& Data::operator[](int i)
    {
	if(!IsList())
	    throw Error::Invalid("Data::operator[](int)","Not a list");

	if(i < 0 || i >= int(vec.size()))
	    throw Error::Range("Data::operator[](int)","Index out of range");

	return vec[size_t(i)];
    }

    const Data& Data::operator[](int i) const
    {
	if(!IsList())
	    throw Error::Invalid("Data::operator[](int)","Not a list");

	if(i < 0 || i >= int(vec.size()))
	    throw Error::Range("Data::operator[](int)","Index "+ToString(i)+" out of range");

	return ((const cow_vector<Data>&)(vec))[size_t(i)];
    }

    const Data& Data::operator[](const Data& key) const
    {
	if(!IsList())
	    throw Error::Invalid("Data::operator[](const Data&)","Not a dictionary.");

	if(!HasKey(key))
	    return Null;

	bool is_old;
	size_t i;

	i=KeyLookup(key,is_old);
		
	return ((const cow_vector<Data>&)(vec))[size_t(i)][1];
    }
	
    Data* Data::FindKey(const Data& key)
    {
	if(!IsList())
	    throw LangErr("Data::FindKey","object is not a list");

	bool is_old;
	size_t i;

	i=KeyLookup(key,is_old);

	if(!is_old)
	{
	    Data pair;

	    pair.MakeList(2);
	    pair.vec[0]=key;
			
	    return InsertAt(i,pair);
	}
		
	return &vec[i];
    }

    bool Data::HasKey(const Data& key) const
    {
	if(!IsList())
	    throw LangErr("Data::HasKey","object is not a list");

	bool is_old;

	KeyLookup(key,is_old);

	return is_old;
    }

    size_t Data::Size() const
    {
	if(!IsList())
	    throw Error::Invalid("Size(const Data& )","Not a list");

	return vec.size();
    }
	
    void Data::MakeList(size_t size,const Data& init)
    {
	type=ListType;
	vec=cow_vector<Data >(size,init);
    }

    void Data::AddList(const Data& item)
    {
	if(!IsList())
	    throw Error::Invalid("AddList(const Data& )","Not a list");
	vec.push_back(item);
    }

    void Data::DelList(int index)
    {
	if(!IsList())
	    throw Error::Invalid("DelList(int)","Not a list");
	if(index < 0 || index >= (int)vec.size())
	    throw Error::Invalid("DelList(int)","Index out of range");

	vec.erase(index);
    }

    void Data::Sort()
    {
	if(!IsList())
	    throw Error::Invalid("Sort()","Not a list");

	std::sort(vec.begin(),vec.end());
    }

    Data Data::Keys() const
    {
	if(!IsList())
	    return Null;

	Data ret;		
	ret.MakeList(Size());
		
	for(size_t i=0; i<Size(); i++)
	    if(!(*this)[i].IsList(2))
		return Null;
	    else
		ret[i]=(*this)[i][0];

	return ret;
    }

    bool Data::DelEntry(const Data& entry)
    {
	if(!IsList())
	    throw Error::Invalid("DelEntry(int)","Not a list");

	bool already_exist;
	size_t pos=KeyLookup(entry,already_exist);

	if(!already_exist)
	    return false;

	DelList(pos);

	return true;
    }
	
    Data& Data::operator=(const Data& z)
    {
	if(this==&z)
	    return *this;

	type=z.type; n=z.n; str=z.str; r=z.r;
		
	if(z.IsDatabase() && z.type==ListType)
	{
	    MakeList(z.vec.size());
	    for(size_t i=0; i<z.vec.size(); i++)
		vec[i]=z[i];
	}
	else
	    vec=z.vec;
		
	return *this;
    }
	
    Data Data::operator-() const
    {
	Data ret;

	switch(type)
	{
	  case NullType:
	      break;
	  case IntegerType:
	      ret=-n;
	      break;
	  case StringType:
	      throw LangErr("Data::operator-()","Cannot negate string");
	  case RealType:
	      ret=-r;
	      break;
	  case ListType:
	      throw LangErr("Data::operator-()","Cannot negate list");
	}

	return ret;
    }

    Data Data::operator*(const Data& arg) const
    {
	if(type==NullType || arg.type==NullType)
	    return Null;
	if(type==IntegerType && arg.type==IntegerType)
	    return Data(n * arg.n);
	if(type==RealType && arg.type==RealType)
	    return Data(r * arg.r);
	if(type==IntegerType && arg.type==RealType)
	    return Data(double(n) * arg.r);
	if(arg.type==IntegerType && type==RealType)
	    return Data(r * double(arg.n));
	if(type==IntegerType && arg.type==StringType)
	{
	    string r,s;
	    s=arg.String();
	    for(int n=Integer(); n>0; n--)
		r+=s;
	    return Data(r);
	}
	if(type==StringType && arg.type==IntegerType)
	{
	    return arg*(*this);
	}
	if(type==IntegerType && arg.type==ListType)
	{
	    Data ret;
	    int n=(*this).Integer();
	    if(n < 0)
		return Null;
			
	    const Data& L=arg;
	    ret.MakeList(L.Size()*n);
	    int k=0;
	    for(int j=n; j > 0; j--)
		for(size_t i=0; i<L.Size(); i++)
		    ret[k++]=L[i];

	    return ret;
	}
	if(type==ListType && arg.type==IntegerType)
	{
	    return arg*(*this);
	}
	if(type==ListType && arg.type==ListType)
	{
	    const Data& L1=*this;
	    const Data& L2=arg;
	    if(L1.Size() != L2.Size())
		throw LangErr("Data::operator*(const Data& )","lists must have the same size");

	    Data ret;
	    ret.MakeList(L1.Size());
	    for(size_t i=0; i<L1.Size(); i++)
		ret[i]=L1[i]*L2[i];
					
	    return ret;
	}

	throw LangErr("Data::operator*(const Data& )","invalid operands");
    }
	
    Data Data::operator/(const Data& arg) const
    {
	if(arg==Data(0))
	    throw LangErr("Data::operator/(const Data& )","Cannot divide by zero");
	if(type==NullType || arg.type==NullType)
	    return Null;
	if(type==IntegerType && arg.type==IntegerType)
	    return Data(n / arg.n);
	if(type==RealType && arg.type==RealType)
	    return Data(r / arg.r);
	if(type==IntegerType && arg.type==RealType)
	    return Data(double(n) / arg.r);
	if(arg.type==IntegerType && type==RealType)
	    return Data(r / double(arg.n));

	throw LangErr("Data::operator/(const Data& )","Cannot divide non-numbers");
    }

    Data Data::operator%(const Data& arg) const
    {
	if(arg==Data(0))
	    throw LangErr("Data::operator%(const Data& )","Cannot divide by zero");
	if(type==NullType || arg.type==NullType)
	    return Null;
	if(type==IntegerType && arg.type==IntegerType)
	    return Data(n % arg.n);

	throw LangErr("Data::operator%(const Data& )","Cannot divide non-integers");
    }

    Data Data::operator&(const Data& arg) const
    {
	if(type==NullType || arg.type==NullType)
	    return Null;
	if(type==IntegerType && arg.type==IntegerType)
	    return Data(n & arg.n);

	throw LangErr("Data::operator&(const Data& )","Cannot compute bitmasks of non-integers");
    }

    Data Data::operator|(const Data& arg) const
    {
	if(type==NullType || arg.type==NullType)
	    return Null;
	if(type==IntegerType && arg.type==IntegerType)
	    return Data(n | arg.n);

	throw LangErr("Data::operator|(const Data& )","Cannot compute bitmasks of non-integers");
    }

    Data Data::operator^(const Data& arg) const
    {
	if(type==NullType || arg.type==NullType)
	    return Null;
	if(type==IntegerType && arg.type==IntegerType)
	    return Data(n ^ arg.n);

	throw LangErr("Data::operator^(const Data& )","Cannot compute bitmasks of non-integers");
    }
    
    Data Data::operator+(const Data& arg) const
    {
	if(type==NullType || arg.type==NullType)
	    return Null;
	if(type==IntegerType && arg.type==IntegerType)
	    return Data(n + arg.n);
	if(type==RealType && arg.type==RealType)
	    return Data(r + arg.r);
	if(type==IntegerType && arg.type==RealType)
	    return Data(double(n) + arg.r);
	if(arg.type==IntegerType && type==RealType)
	    return Data(r + double(arg.n));
	if(type==ListType)
	{
	    Data ret;

	    if(arg.type!=ListType)
		throw LangErr("Data::operator+(const Data& )","Cannot add list and non-list");
	    ret.MakeList(vec.size()+arg.vec.size());

	    size_t i=0;
	    for(size_t j=0; j<vec.size(); j++)
		ret.vec[i++]=vec[j];
	    for(size_t j=0; j<arg.vec.size(); j++)
		ret.vec[i++]=arg.vec[j];

	    return ret;
	}
	if(type==StringType || arg.type==StringType)
	{
	    if(type==StringType && arg.type==StringType)
		return Data(str+arg.str);
	    if(type==StringType)
		return Data(str+tostr(arg).String());

	    return Data(tostr(*this).String()+arg.str);
	}
		
	throw LangErr("Data::operator+(const Data& )","Incompatible operands");
    }

    Data Data::operator-(const Data& arg) const
    {
	if(type==NullType || arg.type==NullType)
	    return Null;
	if(type==IntegerType && arg.type==IntegerType)
	    return Data(n - arg.n);
	if(type==RealType && arg.type==RealType)
	    return Data(r - arg.r);
	if(type==IntegerType && arg.type==RealType)
	    return Data(double(n) - arg.r);
	if(arg.type==IntegerType && type==RealType)
	    return Data(r - double(arg.n));
	if(type==ListType)
	{
	    set<Data> remove;
	    Data ret;

	    if(arg.type!=ListType)
		throw LangErr("Data::operator-(const Data& )","Cannot subtract list and non-list");
	    ret.MakeList();

	    if(IsDatabase() || arg.IsDatabase())
	    {
		for(size_t i=0; i<arg.Size(); i++)
		    remove.insert(arg[i]);
		for(size_t i=0; i<Size(); i++)
		    if(remove.find((*this)[i])==remove.end())
			ret.AddList((*this)[i]);
	    }
	    else
	    {
		vector<Data>::const_iterator i;
		for(i=arg.vec.begin(); i!=arg.vec.end(); i++)
		    remove.insert(*i);
		for(i=vec.begin(); i!=vec.end(); i++)
		    if(remove.find(*i)==remove.end())
			ret.AddList(*i);
	    }

	    return ret;
	}
		
	throw LangErr("Data::operator-const Data& )","Incompatible operands");
    }

    bool Data::operator<(const Data& arg) const
    {

	if(type==IntegerType && arg.type==IntegerType)
	    return (n < arg.n);
	if(type==RealType && arg.type==RealType)
	    return (r < arg.r);
	if(type==IntegerType && arg.type==RealType)
	    return (double(n) < arg.r);
	if(arg.type==IntegerType && type==RealType)
	    return (r < double(arg.n));

	if(type==StringType && arg.type==StringType)
	    return (str < arg.str);

	if(type==ListType && arg.type==ListType)
	{
	    for(size_t i=0; i<Size() && i<arg.Size(); i++)
		if(vec[i] < arg.vec[i])
		    return true;
		else if(arg.vec[i] < vec[i])
		    return false;

	    if(arg.Size() > Size())
		return true;

	    return false;
	}

	return (type < arg.type);
    }

    bool Data::operator==(const Data& arg) const
    {
	if(type==IntegerType && arg.type==IntegerType)
	    return (n == arg.n);
	if(type==RealType && arg.type==RealType)
	    return (r == arg.r);
	if(type==IntegerType && arg.type==RealType)
	    return (double(n) == arg.r);
	if(arg.type==IntegerType && type==RealType)
	    return (r == double(arg.n));
	if(type==StringType && arg.type==StringType)
	    return (str == arg.str);
	if(type==NullType && arg.type==NullType) // Both NULL
	    return true;
	if(type==NullType || arg.type==NullType) // no, other is not
	    return false;
	if(type==ListType && arg.type==ListType)
	{
	    if(Size() != arg.Size())
		return false;
				
	    if(IsDatabase() || arg.IsDatabase())
	    {
		for(size_t i=0; i<Size(); i++)
		    if((*this)[i]!=arg[i])
			return false;
	    }
	    else
	    {
		vector<Data>::const_iterator i,j;
		for(i=vec.begin(),j=arg.vec.begin(); i!=vec.end(); i++,j++)
		    if(*i!=*j)
			return false;
	    }
		   
	    return true;
	}

	return false;
    }

    Data Data::operator||(const Data& arg) const
    {
	if(type==IntegerType && arg.type==IntegerType)
	    return Data((int)(n || arg.n));
	if(type==RealType && arg.type==RealType)
	    return Data((double)(r || arg.r));
	if(type==IntegerType && arg.type==RealType)
	    return Data((double)(double(n) || arg.r));
	if(arg.type==IntegerType && type==RealType)
	    return Data((double)(r || double(arg.n)));
	if(type==NullType || arg.type==NullType)
	    return Null;

	throw LangErr("Data::operator||(const Data& )","Invalid argument type");
    }

    Data Data::operator&&(const Data& arg) const
    {
	if(type==IntegerType && arg.type==IntegerType)
	    return Data((int)(n && arg.n));
	if(type==RealType && arg.type==RealType)
	    return Data((double)(r && arg.r));
	if(type==IntegerType && arg.type==RealType)
	    return Data((double)(double(n) && arg.r));
	if(arg.type==IntegerType && type==RealType)
	    return Data((double)(r && double(arg.n)));
	if(type==NullType || arg.type==NullType)
	    return Null;

	throw LangErr("Data::operator&&(const Data& )","Invalid argument type");
    }

    ostream& operator<<(ostream& O,const Data& D)
    {
	if(D.IsNull())
	    O << "NULL";
	else if(D.IsInteger())
	    O << D.Integer();
	else if(D.IsString())
	    O << D.String();
	else if(D.IsReal())
	    O << D.Real();
	else if(D.IsList())
	{
	    for(size_t i=0; i<D.Size(); i++)
	    {
		if(i)
		    O << ' ';
		O << D[i];
	    }
	}

	return O;
    }

    void ArgumentError(const string& fn,const Data& arg)
    {
	throw LangErr(fn+"(const Data& )","Invalid argument "+tostr(arg).String()+" for function "+fn+"()");
    }

    static int pretty_save_indent;
	
    void DoPrettySave(ostream& O,const Data& D)
    {
	if(D.IsList())
	{
	    const Data& L=D;

	    if(L.Size()==0)
		O << "(,)";
	    else
	    {
		if(pretty_save_indent)
		    O << endl;
		for(int i=pretty_save_indent; i>0; i--)
		    O << ' ';
		O << '(';
		pretty_save_indent+=2;
		for(size_t i=0; i<L.Size(); i++)
		{
		    DoPrettySave(O,L[i]);
		    O << ',';
		}
		pretty_save_indent-=2;
		O << ')';
	    }
	}
	else
	    O << tostr(D).String();
    }
	
    void PrettySave(ostream& O,const Data& D)
    {
	pretty_save_indent=0;
	DoPrettySave(O,D);
	O << endl;
    }

    Data ReadValue(const string& filename)
    {
	ifstream F(filename.c_str());
	if(!F)
	    throw Error::IO("ReadValue(const string&)","file "+string(filename)+" not found");

	string buffer;
	while(F)
	    buffer+=readline(F);

	return toval(buffer);
    }

}
