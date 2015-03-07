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
#ifndef DATA_H
#define DATA_H

#include <string>
#include <set>
#include "cow_vector.h"
#include "error.h"
#include "tools.h"

namespace Evaluator
{
	class Data;
	
	/// NULL value.
	extern Data Null;
	/// Flag which is set when a library function is interrupted by signal.
	extern bool quitsignal;
	/// Directory to use as variable storage.
	extern string savedir;
	/// Directory containing additional executables if any.
	extern string execdir;
	/// Print all function calls, return values and variable substitutions.
	extern bool debug;
		
	/// Scan string as long legal variable name found as possible and store it to 'var'.
	void GetVariable(const char*& _src,string& var);
	/// Skip string for as long legal variable.
	void GetVariable(const char*& _src);
	/// Return true, if argument is valid variable.
	bool IsVariable(const string& s);
	/// Take next '- or "-separated string and append it to 'var'. Replace \c notations with c if "-separated string.
	void ReadString(const char*& _src,string& var);
	/// Take next '- or "-separated string.
	void ReadString(const char*& _src);
	/// Take next expression in parenthesis counting left ( and right ) correctly and put it to 'var'.
	void ReadParenthesis(const char*& _src,string& var);
	/// Discard next parenthesis expression.
	void ReadParenthesis(const char*& _src);
	/// Take next literal atom, i.e. NULL, number,string or list of those.
	Data ReadLiteral(string& s);
	/// Take next block or statement from the string and put it to var.
	void ReadStatement(const char*& _src,string& var);
	/// Take next block or statement from the string.
	void ReadStatement(const char*& _src);
	/// Strip extra whitespaces away. Note: return buffer is overwritten during each call.
	const char* CompressCode(const string& str);
	/// Write value 'D' to output stream enhanching it's readability by using indentation.
	void PrettySave(ostream& O,const Data& D);
	/// Read a value from a file.
	Data ReadValue(const string& filename);

	/// Error in language syntax.
	class LangErr:public Error::General
	{
	  public:
		
		/// Language syntax error in function \f$fn\f$ with message \f$r\f$.
		LangErr(const std::string& fn,const std::string& r="") : Error::General("LangErr",fn,r) {}
	};
	
	/// Data types supported by evaluator.
	enum Type {NullType,IntegerType,RealType,StringType,ListType};
	
	/// Data object.
	class Data
	{
		
	  protected:
		
		/// Type of the object.
		Type type;

		/// Integer storage.
		int n;
		/// String storage.
		string str;
		/// Real number storage.
		double r;
		/// List storage.
		mutable cow_vector<Data> vec; // DataFileDB::LoadCache() requires mutable

		/// Return insertion position for key in hash and whether or not key was already there.
		virtual size_t KeyLookup(const Data& key,bool& already_exist) const;
		
	public:

		/// Construct a NULL object.
		Data()
			{type=NullType;}
		/// Copyconstructor.
		Data(const Data& z)
			{type=z.type; n=z.n; str=z.str; r=z.r; vec=z.vec;}
		/// Construct an integer object.
		Data(int z)
			{type=IntegerType; n=z;}
		/// Construct a real number.
		Data(double z)
			{type=RealType; r=z;}
		/// Construct a string object.
		Data(const string& z)
			{type=StringType; str=z;}
		/// Construct a list object.
		Data(const vector<Data>& l)
			{
				MakeList(l.size());
				for(size_t i=0; i<l.size(); i++)
					vec[i]=l[i];
			}
		/// Construct a list with two members.
		Data(const Data& m0,const Data& m1) {MakeList(2); vec[0]=m0; vec[1]=m1;}
		/// Construct a list with three members.
		Data(const Data& m0,const Data& m1,const Data& m2) {MakeList(3); vec[0]=m0; vec[1]=m1; vec[2]=m2;}
		/// Construct a list with four members.
		Data(const Data& m0,const Data& m1,const Data& m2,const Data& m3) {MakeList(4); vec[0]=m0; vec[1]=m1; vec[2]=m2; vec[3]=m3;}
		/// Construct a list with five members.
		Data(const Data& m0,const Data& m1,const Data& m2,const Data& m3,const Data& m4) {MakeList(5); vec[0]=m0; vec[1]=m1; vec[2]=m2; vec[3]=m3; vec[4]=m4;}


		virtual ~Data()
			{}
		
		/// Copy assignment.
		virtual Data& operator=(const Data& z);
		/// Integer assignment.
		Data& operator=(int z)
			{type=IntegerType; n=z; vec=cow_vector<Data>(); return *this;}
		/// String assignment.
		Data& operator=(const string& z)
			{type=StringType; str=z; vec=cow_vector<Data>(); return *this;}
		/// Real assignment.
		Data& operator=(double z)
			{type=RealType; r=z; vec=cow_vector<Data>(); return *this;}

		/// Return 1 if this element uses database.
		virtual bool IsDatabase() const
			{return 0;}
		
		/// Total ordering on Data objects.
		bool operator<(const Data& ) const;
		bool operator>(const Data& d) const
			{return d.operator<(*this);}
		/// Equality oprator returns true when both operands are numerically equal or have same members.
		bool operator==(const Data& ) const;
		/// 
		bool operator!=(const Data& d) const
			{return !operator==(d);}

		/// Change object to list.
		void MakeList(size_t size=0)
			{MakeList(size,Null);}
		/// Change object to list.
		virtual void MakeList(size_t size,const Data& init);
		/// Add an object to the list.
		virtual void AddList(const Data& item);
		/// Delete an object from the list.
		virtual void DelList(int index);
		/// Sort the list or throw an error if not a list.
		virtual void Sort();
		/// Return size of the list or throw error if this is not a list.
		virtual size_t Size() const;
		/// Return reference to i:th member of this list or throw an exception if this is not a list.
		virtual Data& operator[](int i);
		/// Return value of the i:th member of this list or throw an exception if this is not a list.
		virtual const Data& operator[](int i) const;
		/// Return a value of an dictionary entry or Null if not found. If the object is not a dictionary, throw an exception.
		virtual const Data& operator[](const Data& key) const;

		/// Find a pair from the sorted list with first component as 'key'. Return true if found. Throw Error::Syntax if this object is not a dictionary list.
		virtual bool HasKey(const Data& key) const;
		/// Find a pair from the sorted list with first component as 'key'. Insert pair (key,NULL) if not found. Throw an error if this object is not a dictionary list. Return also index postion, if pointer not null.
		virtual Data* FindKey(const Data& key);
		/// Ensure that object is a list and insert object to the given position if valid.
		virtual Data* InsertAt(int pos,const Data& object);
		/// Return keys of a dictionary or NULL if not a dictionary.
		virtual Data Keys() const;
		/// Delete an dictionary entry. Return 1 if found.
		virtual bool DelEntry(const Data& d);
		
		/// True if object is NULL.
		bool IsNull() const
			{return type==NullType;}
		/// True if object is an integer.
		bool IsInteger() const
			{return type==IntegerType;}
		/// True if object is a string.
		bool IsString() const
			{return type==StringType;}
		/// True if object is a list.
		bool IsList() const
			{return type==ListType;}
		/// True if object is a list and it's size 'exact_size'.
		bool IsList(size_t exact_size) const
			{return type==ListType && vec.size()==exact_size ;}
		/// True if object is a real number.
		bool IsReal() const
			{return type==RealType;}

		/// Return integer value of the object or zero if the object is not an integer.
		int Integer() const
			{return IsInteger() ? n : 0;}
		/// Return string value of the object or "" if the object is not a string.
		string String() const
			{return IsString() ? str : string("");}
		/// Return real value of the object or zero if the object is not an real.
		double Real() const
			{return IsReal() ? r : 0.0;}
		/// Return a constant list reference if the object is a list. Otherwise throw Error::Invalid.
/* 		virtual const vector<Data>& List() const */
/* 			{if(!IsList()) throw Error::Invalid("Data::List()","Not a list"); return vec.const_ref();} */

		/// Negate integer or throw LangErr. Operation on NULL object gives always NULL.
		Data operator-() const;
		/// Add two objects. Lists and strings are catenated, integers are summed together.
		Data operator+(const Data& ) const;
		/// Subtract two objects. If args are lists, remove elements from first belonging second.
		Data operator-(const Data& ) const;
		/// Multiply two numbers or throw LangErr.
		Data operator*(const Data& ) const;
		/// Divide a number by other number. Other objects throws LangErr.
		Data operator/(const Data& ) const;
		/// Return remainder for integers. Other objects throws LangErr.
		Data operator%(const Data& ) const;
		/// Bitwise OR.
		Data operator|(const Data& ) const;
		/// Bitwise AND.
		Data operator&(const Data& ) const;
		/// Bitwise XOR.
		Data operator^(const Data& ) const;
		/// Logical OR.
		Data operator||(const Data& ) const;
		/// Logical AND.
		Data operator&&(const Data& ) const;
	};

	/// Dump value of the object D.
	ostream& operator<<(ostream& O,const Data& D);

}

#endif
