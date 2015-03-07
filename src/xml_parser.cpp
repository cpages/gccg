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
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(_MSC_VER)
#include <io.h>
#include "compat.h"
#else
#include <unistd.h>
#endif

#include <cstdlib> 
#include "xml_parser.h"
#include "error.h"

using namespace XML;
using namespace std;

//
// Class: Element
// ==============

Element::Element(const Element& e)
{
	name=e.name;
	attribute=e.attribute;
	data=e.data;
	list<Element*>::const_iterator i;
	for(i=e.sub.begin(); i!=e.sub.end(); i++)
		sub.push_back(new Element(*(*i)));
}

Element::Element(const Element* e)
{
	name=e->name;
	attribute=e->attribute;
	data=e->data;
	list<Element*>::const_iterator i;
	for(i=e->sub.begin(); i!=e->sub.end(); i++)
		sub.push_back(new Element(*(*i)));
}

Element& Element::operator=(const Element* e)
{
	name=e->name;
	attribute=e->attribute;
	data=e->data;
	list<Element*>::const_iterator i;

	for(i=sub.begin(); i!=sub.end(); i++)
	    delete *i;
	sub.clear();
	
	for(i=e->sub.begin(); i!=e->sub.end(); i++)
	    sub.push_back(new Element(**i));

	return *this;
}

Element::Element(const string& s)
{
    if(s=="")
	throw Error::Range("Element::Element(const string&)","Element cannot have empty name");
    name=s;
}

Element::~Element()
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	delete (*i);
}

list<string> Element::AttributeNames() const
{
    list<string> ret;
	
    map<string,string>::const_iterator i;
    for(i=attribute.begin(); i!=attribute.end(); i++)
	ret.push_back(i->first);

    return ret;
}

size_t Element::SubElements(const string& n) const
{
    size_t count=0;

    list<Element*>::const_iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()==n)
	    count++;

    return count;
}

list<Element*> Element::Subs(const string& n) const
{
    list<Element*> ret;

    list<Element*>::const_iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()==n)
	    ret.push_back(*i);

    return ret;
}

list<Element*> Element::SubsWithAttr(const string& s,const string& k,const string& v) const
{
    list<Element*> ret;

    list<Element*>::const_iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()==s && (**i)[k]==v)
	    ret.push_back(*i);

    return ret;
}

Element* Element::NthSubWithAttr(int n,const string& s,const string& k,const string& v) const
{
    if(n < 0)
	throw Error::Range("Element::NthSubWithAttr(int,const string&,const string&,const string&)","Index below zero ("+s+","+k+","+v+")");
	
    list<XML::Element*> L=SubsWithAttr(s,k,v);
    list<XML::Element*>::iterator i=L.begin();

    while(1)
    {
	if(i==L.end())
	    throw Error::Range("Element::NthSubWithAttr(int,const string&,const string&,const string&)","Index too big ("+s+","+k+","+v+")");
	if(n==0)
	    return *i;
	i++;
	n--;
    }
}

Element* Element::NthSub(int n,const string& s) const
{
    if(n < 0)
	throw Error::Range("Element::NthSub(size_t,const string&)","Index below zero");

    list<XML::Element*> L=Subs(s);
    list<XML::Element*>::iterator i=L.begin();

    while(1)
    {
	if(i==L.end())
	    throw Error::Range("Element::NthSub(size_t,const string&)","Index too big");
	if(n==0)
	    return *i;
	i++;
	n--;
    }
}

list<Element*> Element::Subs(const string& s1,const string& s2) const
{
    list<Element*> ret;

    list<Element*>::const_iterator i,j;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()==s1)
	    for(j=(*i)->sub.begin(); j!=(*i)->sub.end(); j++)
		if((*j)->Name()==s2)
		    ret.push_back(*j);

    return ret;
}

void Element::AddSubelements(const list<Element*>& E)
{
    list<Element*>::const_iterator i;
    for(i=E.begin(); i!=E.end(); i++)
	sub.push_back(new Element(*i));
}

void Element::DropSubElements(const string& n)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()==n)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

void Element::DropSubsWithAttr(const string& n,const string& k,const string& v)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()==n && (**i)[k]==v)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

void Element::SelectSubElements(const string& n)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()!=n)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

void Element::SelectSubElements(const string& n1,const string& n2)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()!=n1 && (*i)->Name()!=n2)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

void Element::SelectSubElements(const string& n1,const string& n2,const string& n3)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()!=n1 && (*i)->Name()!=n2 && (*i)->Name()!=n3)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

void Element::SelectSubElements(const string& n1,const string& n2,const string& n3,const string& n4)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()!=n1 && (*i)->Name()!=n2 && (*i)->Name()!=n3 && (*i)->Name()!=n4)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

void Element::SelectSubElements(const string& n1,const string& n2,const string& n3,const string& n4,const string& n5)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()!=n1 && (*i)->Name()!=n2 && (*i)->Name()!=n3 && (*i)->Name()!=n4 && (*i)->Name()!=n5)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

void Element::SelectSubElements(const string& n1,const string& n2,const string& n3,const string& n4,const string& n5,const string& n6)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()!=n1 && (*i)->Name()!=n2 && (*i)->Name()!=n3 && (*i)->Name()!=n4 && (*i)->Name()!=n5 && (*i)->Name()!=n6)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

void Element::SelectSubElements(const string& n1,const string& n2,const string& n3,const string& n4,const string& n5,const string& n6,const string& n7,const string& n8,const string& n9)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()!=n1 && (*i)->Name()!=n2 && (*i)->Name()!=n3 && (*i)->Name()!=n4 && (*i)->Name()!=n5 && (*i)->Name()!=n6 && (*i)->Name()!=n7 && (*i)->Name()!=n8 && (*i)->Name()!=n9)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

void Element::SelectSubElements(const string& n1,const string& n2,const string& n3,const string& n4,const string& n5,const string& n6,const string& n7,const string& n8,const string& n9,const string& n10,const string& n11,const string& n12)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()!=n1 && (*i)->Name()!=n2 && (*i)->Name()!=n3 && (*i)->Name()!=n4 && (*i)->Name()!=n5 && (*i)->Name()!=n6 && (*i)->Name()!=n7 && (*i)->Name()!=n8 && (*i)->Name()!=n9 && (*i)->Name()!=n10 && (*i)->Name()!=n11 && (*i)->Name()!=n12)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

void Element::SelectSubElements(const string& n1,const string& n2,const string& n3,const string& n4,const string& n5,const string& n6,const string& n7,const string& n8,const string& n9,const string& n10,const string& n11,const string& n12,const string& n13)
{
    list<Element*>::iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()!=n1 && (*i)->Name()!=n2 && (*i)->Name()!=n3 && (*i)->Name()!=n4 && (*i)->Name()!=n5 && (*i)->Name()!=n6 && (*i)->Name()!=n7 && (*i)->Name()!=n8 && (*i)->Name()!=n9 && (*i)->Name()!=n10 && (*i)->Name()!=n11 && (*i)->Name()!=n12 && (*i)->Name()!=n13)
	{
	    delete *i;
	    *i=0;
	}

    sub.remove(0);
}

bool Element::HasSubAttr(const string& s,const string& k,const string& v) const
{
    list<Element*>::const_iterator i;
    for(i=sub.begin(); i!=sub.end(); i++)
	if((*i)->Name()==s && (**i)[k]==v)
	    return 1;

    return 0;
}

static string ToString(int i)
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
	digit[0]=char(i % 10)+'0';
	i/=10;
	s=(digit+s);
		
    }while(i);
	
    return sign+s;
}

void Element::AddToAttribute(const string& a,int n)
{
    attribute[a]=ToString(atoi(attribute[a].c_str())+n);
}

int Element::Val(const string& a)
{
    return atoi(attribute[a].c_str());
}

void Element::JoinTo(const string& n,const list<Element*>& L)
{
    list<Element*> S=Subs(n);

    if(S.size() > 1)
	throw Error::Invalid("Element::JoinTo(const string&,const list<Element*>&)","There are more than one "+n+" subelement.");

    if(S.size()==0)
	AddSubelement(new Element(n));

    AddSubelements(L);
}

//
// Class: Document
// ===============

Document::Document()
{
    base=0;
    prolog="<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>";
    buffer=0;
    size=0;
    next=0;
}

Document& Document::operator=(const Document& D)
{
    prolog=D.prolog;
    if(base)
	delete base;
    base=new Element(D.base);
    buffer=0;
    size=0;
    next=0;

    return *this;
}

Document::Document(const Document& D)
{
    prolog=D.prolog;
    base=new Element(D.base);
}

Document::~Document()
{
    close();
    if(base)
	delete base;
}

void Document::SetBase(Element* e)
{
    if(base)
	delete base;
    base=e;
}

// LOW LEVEL FILE HANDLING
// =======================

void Document::open(const string& filename)
{
    struct stat file;

    if(stat(filename.c_str(),&file))
	throw Error::IO("Document::open(const string&)","cannot open file '"+filename+"'");
    if(buffer)
	delete buffer;

    if(base)
        delete base;
    base=0;
    size=file.st_size;
    buffer=new char[size];
    if(!buffer)
	throw Error::Memory("Document::open(const string&)");

    next=0;

    FILE* fd=::fopen(filename.c_str(),"rb");
    if(fd==NULL)
	throw Error::IO("Document::open(const string&)","cannot open file '"+filename+"'");

    if(size_t(::fread(buffer,sizeof(char),size,fd))!=size)
	throw Error::IO("Document::open(const string&)","short read on '"+filename+"'");

	::fclose(fd);
}

int Document::peek()
{
    if(!buffer)
	throw Error::Invalid("Document::peek()","file not yet opened");
    if(next==size)
	return EOF;

    return buffer[next];
}

int Document::get()
{
    if(!buffer)
	throw Error::Invalid("Document::get()","file not yet opened");
    if(next==size)
	return EOF;

    return buffer[next++];
}

void Document::putback()
{
    if(!buffer)
	throw Error::Invalid("Document::putback()","file not yet opened");
    if(next==0)
	throw Error::Invalid("Document::putback()","buffer underflow");
    next--;
}

void Document::close()
{
    if(buffer)
	delete buffer;

    buffer=0;
    size=0;
    next=0;
}

// READING TOOLS
// =============

/// Strip away next white spaces.
void Document::SkipWhite()
{
    while(isspace(peek()))
	get();
}

/// Read characters until mark is found. Return all characters read including mark.
string Document::ReadUntil(const string& mark)
{
    if(mark=="")
	return "";

    string data;
    int c;
    while(1)
    {
	c=get();
	if(c==EOF)
	    throw Error::Syntax("ReadUntil(istream&,const string&)","EOF before '"+mark+"'");
	data+=char(c);
	if(data.length() >= mark.length() && data.substr(data.length()-mark.length())==mark)
	    break;
    }

    return data;
}

/// Check if the next characters are str. Throw Error::Syntax if they are not.
string Document::CheckFor(const string& str)
{
    int c;
    for(size_t i=0; i<str.length(); i++)
    {
	c=get();
	if(c==EOF)
	    throw Error::Syntax("CheckFor(istream&,const string&)","EOF before '"+str+"'");
	if(char(c) != str[i])
	    throw Error::Syntax("CheckFor(istream&,const string&)","'"+str+"' expected");
    }

    return str;
}

/// Read valid XML element or attribute name.
string Document::GetName()
{
    int c;
    string name;
    SkipWhite();
    while(1)
    {
	c=peek();
	if(c==EOF)
	    break;
	if(!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'z') || (c >= '0' && c <= '9') || c=='.' || c=='-' || c=='_' || c==':'))
	    break;

	name+=char(c);
	get();
    }
    if(name=="")
	throw Error::Syntax("GetName(istream&)","Empty name");

    return name;
}

/// Read escaped character sequence.
char Document::ReadEscape()
{
    string sym;
    CheckFor("&");
    sym=ReadUntil(";");
    if(sym=="amp;")
	return '&';
    if(sym=="lt;")
	return '<';
    if(sym=="gt;")
	return '>';
    if(sym=="quot;")
	return '"';

    throw Error::Syntax("ReadEscape(istream&)","Invalid escape sequence &"+sym);
}

/// Read value enclosed in "'s
string Document::ReadValue()
{
    string val;
    int c;

    CheckFor("\"");
    while(1)
    {
	c=get();
	if(c==EOF)
	    throw Error::Syntax("CheckFor(istream&,const string&)","EOF before \"");
	if(c=='"')
	    break;
	if(c=='&')
	{
	    putback();
	    c=ReadEscape();
	}

	val+=char(c);
    }

    return val;
}

/// Read in attribute name and it's value.
void Document::ReadAttribute(string& key,string& value)
{
    key=GetName();
    SkipWhite();
    CheckFor("=");
    SkipWhite();
    value=ReadValue();
}

/// Escape special characters.
string Document::Escape(const string& str)
{
    string ret;

    for(size_t i=0; i<str.length(); i++)
    {
	if(str[i]=='&')
	    ret+="&amp;";
	else if(str[i]=='<')
	    ret+="&lt;";
	else if(str[i]=='>')
	    ret+="&gt;";
	else if(str[i]=='"')
	    ret+="&quot;";
	else
	    ret+=str[i];
    }

    return ret;
}

/// Allocate dynamically an element and fill it according to stream input.
Element* Document::ReadElement()
{
    string name;
    Element* e;
    int c;
	
    SkipWhite();
    CheckFor("<");

    name=GetName();
    e=new Element(name);

    // Read element attributes if any.
    while(1)
    {
	SkipWhite();

	c=peek();
	if(c=='>')
	{
	    get();
	    break;
	}
	if(c=='/') // Empty element
	{
	    get();
	    CheckFor(">");
	    return e;
	}
		
	string key,val;

	ReadAttribute(key,val);
	e->AddAttribute(key,val);
    }
	
    // Non-empty element.
    Element* s;
	
    while(1)
    {
	SkipWhite();
	if(peek() != '<')
	    throw Error::Syntax("ReadElement(istream&)","Character data not yet implemented");
	get();
	if(peek()=='/')
	{
	    get();
	    if(GetName() != name)
		throw Error::Syntax("ReadElement(istream&)","End tag </"+name+"> missing");
	    SkipWhite();
	    CheckFor(">");
			
	    break;
	}
		
	putback();
	s=ReadElement();
	e->AddSubelement(s);
    }
	
    return e;
}

void Document::ReadFile(const string& filename)
{
    open(filename);
    SkipWhite();
    CheckFor("<?xml");
    SetProlog("<?xml"+ReadUntil("?>"));
    while(isspace(peek()))
	get();
    int c=get();
    if(c==EOF)
	throw Error::Syntax("Document::ReadFile(const string&)","EOF before data");

    if(c=='<' && peek()=='!')
    {
	AddProlog("\n<");
	AddProlog(ReadUntil(">"));
    }
    else
	putback();

    SetBase(ReadElement());
    close();
}

void Document::WriteElement(ostream& O,size_t tab,Element* e)
{
    if(tab==2)
	cout << endl;

    for(size_t i=0; i<tab; i++)
	O << ' ';
    O << '<' << e->Name();
    if(e->Attributes())
    {
	list<string> A=e->AttributeNames();
	list<string>::iterator i;
	for(i=A.begin(); i!=A.end(); i++)
	{
	    O << ' ';
	    O << *i << "=\"" << Escape((*e)[*i]) << "\"";
	}
    }
    if(e->SubElements())
    {
	O << '>';
	O << endl;
	list<Element*>::iterator i;
	for(i=e->Subs().begin(); i!=e->Subs().end(); i++)
	    WriteElement(O,tab+2,*i);
	for(size_t i=0; i<tab; i++)
	    O << ' ';
	if(tab==0)
	    O << endl;
	O << "</" << e->Name() << ">" << endl;
    }
    else
	O << "/>" << endl;
}

void Document::WriteFile(const string& filename)
{
    ofstream F(filename.c_str());
    if(!F)
	throw Error::IO("Document::WriteFile(const string&)","Cannot open '"+filename+"' for writing");
    F << Prolog() << endl << endl;
    WriteElement(F,0,Base());
}
