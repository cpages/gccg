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

#include <list>
#include <string>
#include <map>
#include <fstream>

/// XML-coding and encoding.
namespace XML
{
    /// Element of XML document.
    class Element
    {
	/// Name of the element.
	std::string name;
	/// List of attributes and their values.
	std::map<std::string,std::string> attribute;
	/// Character data.
	std::string data;
	/// Subelements.
	std::list<Element*> sub;

    public:

	/// Allocate copies of each subelement.
	Element(const Element& e);
	/// Allocate copies of each subelement.
	Element(const Element* e);
	/// Create an element with name s.
	Element(const std::string& s);
	/// Delete all sub elements.
	~Element();

	/// Allocate copies.
	Element& operator=(const Element* e);
	/// Allocate copies.
	Element& operator=(const Element& e)
	{operator=(&e); return *this;}
		
	/// Add an attribute named n with value v.
	void AddAttribute(const std::string& n, const std::string& v)
	{attribute[n]=v;}
	/// Delete an attribute named n if it exist.
	void DelAttribute(const std::string& n)
	{attribute.erase(n);}
	/// Return the list of attribute names.
	std::list<std::string> AttributeNames() const;
	/// Name of the element.
	const std::string& Name() const
	{return name;}
	/// Number of attributes.
	size_t Attributes() const
	{return attribute.size();}
	/// Return attribute value if defined. Otherwise empty std::string.
	std::string operator[](const std::string& key)
	{return attribute[key];}
	/// Add subelement.
	void AddSubelement(Element* e)
	{sub.push_back(e);}
	/// Allocate copies of list members and add them to this element as subelements.
	void AddSubelements(const std::list<Element*>& e);
	/// Drop all subelements who has a name n.
	void DropSubElements(const std::string& n);
	/// Drop subelements with name n and attribute k with value v.
	void DropSubsWithAttr(const std::string& n,const std::string& k,const std::string& v);
	/// Drop all subelements except those who has a name n.
	void SelectSubElements(const std::string& n);
	/// Drop all subelements except those who has a name n1 or n2.
	void SelectSubElements(const std::string& n1,const std::string& n2);
	/// Drop all subelements except those who has a name n1,n2 or n3.
	void SelectSubElements(const std::string& n1,const std::string& n2,const std::string& n3);
	/// Drop all subelements except those who has a name n1,n2,n3 or n4.
	void SelectSubElements(const std::string& n1,const std::string& n2,const std::string& n3,const std::string& n4);
	/// Drop all subelements except those who has a name n1,n2,n3,n4 or n5.
	void SelectSubElements(const std::string& n1,const std::string& n2,const std::string& n3,const std::string& n4,const std::string& n5);
	/// Drop all subelements except those who has a name n1,...,n6.
	void SelectSubElements(const std::string& n1,const std::string& n2,const std::string& n3,const std::string& n4,const std::string& n5,const std::string& n6);
	/// Drop all subelements except those who has a name n1,...,n9.
	void SelectSubElements(const std::string& n1,const std::string& n2,const std::string& n3,const std::string& n4,const std::string& n5,const std::string& n6,const std::string& n7,const std::string& n8,const std::string& n9);
	/// Drop all subelements except those who has a name n1,...,n12.
	void SelectSubElements(const std::string& n1,const std::string& n2,const std::string& n3,const std::string& n4,const std::string& n5,const std::string& n6,const std::string& n7,const std::string& n8,const std::string& n9,const std::string& n10,const std::string& n11,const std::string& n12);
	/// Drop all subelements except those who has a name n1,...,n13.
	void SelectSubElements(const std::string& n1,const std::string& n2,const std::string& n3,const std::string& n4,const std::string& n5,const std::string& n6,const std::string& n7,const std::string& n8,const std::string& n9,const std::string& n10,const std::string& n11,const std::string& n12,const std::string& n13);
	/// Number of subelements.
	size_t SubElements() const
	{return sub.size();}
	/// Number of subelements named n.
	size_t SubElements(const std::string& n) const;
	/// List of subelements.
	std::list<Element*>& Subs()
	{return sub;}
	/// List of the subelements named s.
	std::list<Element*> Subs(const std::string& s) const;
	/// List of the subelements with name s and which have an attribute k with value v.
	std::list<Element*> SubsWithAttr(const std::string& s,const std::string& k,const std::string& v) const;
	/// Find Nth subelement with name s and which have an attribute k with value v.
	Element* NthSubWithAttr(int n,const std::string& s,const std::string& k,const std::string& v) const;
	/// Nth subelement which have a name s.
	Element* NthSub(int n,const std::string& s) const;
	/// List of the subsubelements of subelement s1 whos name is s2.
	std::list<Element*> Subs(const std::string& s1,const std::string& s2) const;
	/// 1 if element have a subelement s with attribute k which has a value v.
	bool HasSubAttr(const std::string& s,const std::string& k,const std::string& v) const;
	/// Increment numeric value of attribute a by n. (Non-numeric or empty attribute is considered as a zero.)
	void AddToAttribute(const std::string& a,int n);
	/// Return numerical value of attribute a.
	int Val(const std::string& a);
	/// Create new subelement named n if it does not exist. After that append all elements of L as subelements.
	void JoinTo(const std::string& n,const std::list<Element*>& L);
    };

    /// Print formatted XML element to the stream.
    std::ostream& operator<<(std::ostream& O,Element* E);

    /// XML-document.
    class Document
    {
	/// Prolog code of XML-document.
	std::string prolog;
	/// Base element if any.
	Element* base;
	/// Content of the file during read or zero if not yet read in.
	char* buffer;
	/// Size of the file during read.
	size_t size;
	/// Pointer to the next character during file read.
	size_t next;

	/// Initialize read buffer by reading in the file.
	void open(const std::string& filename);
	/// Return next character or EOF.
	int peek();
	/// Read next characer from the buffer or EOF.
	int get();
	/// Cancel reading of the previous character.
	void putback();
	/// Clear the buffer.
	void close();

	/// Strip away next white spaces.
	void SkipWhite();
	/// Read characters until mark is found. Return all characters read including mark.
	std::string ReadUntil(const std::string& mark);
	/// Check if the next characters are str. Throw Error::Syntax if they are not.
	std::string CheckFor(const std::string& str);
	/// Read valid XML element or attribute name.
	std::string GetName();
	/// Read escaped character sequence.
	char ReadEscape();
	/// Read value enclosed in "'s
	std::string ReadValue();
	/// Read in attribute name and it's value.
	void ReadAttribute(std::string& key,std::string& value);
	/// Escape special characters.
	std::string Escape(const std::string& str);
	/// Allocate dynamically an element and fill from the input buffer.
	Element* ReadElement();

	/// Helper function to write an element to the stream.
	void WriteElement(std::ostream& O,size_t tab,Element* e);

    public:

	/// Create an empty document.
	Document();
	/// Allocate copies of all elements.
	Document(const Document& D);
	/// Free all elements.
	~Document();

	/// Allocate copise of all elements.
	Document& operator=(const Document& D);
		
	/// Read XML-file and constuct a document.
	void ReadFile(const std::string& filename);
	/// Write current document to XML-file.
	void WriteFile(const std::string& filename);
	/// Set prolog string.
	void SetProlog(const std::string& str)
	{prolog=str;}
	/// Add to prolog string.
	void AddProlog(const std::string& str)
	{prolog+=str;}
	/// Get prolog std::string.
	const std::string& Prolog() const
	{return prolog;}
	/// Delete document tree and set new base element e.
	void SetBase(Element* e);
	/// Base element.
	Element* Base() const
	{return base;}
	/// Return the list of first level elements (subelements of base) whos name is s.
	std::list<Element*> operator()(const std::string& s) const
	{return base->Subs(s);}
	/// Return the list of second level elements whos names are s2 and which are the sub elements of elements named s1.
	std::list<Element*> operator()(const std::string& s1,const std::string& s2) const
	{return base->Subs(s1,s2);}
    };
}
