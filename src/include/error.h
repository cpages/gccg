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

#ifndef __ERROR_H
#define __ERROR_H

#include <string>

/// Namespace for errorhandling.
namespace Error
{
	/// Base class of errors.
	class General
	{
		/// Class name of exception.
		std::string type;
		/// Function, where exception was thrown.
		std::string function;
		/// Explanation.
		std::string reason;

	  protected:
		
		/// Error of type \f$t\f$ in function \f$fn\f$ with message \f$r\f$.
		General(const std::string& t,const std::string& fn,const std::string& r)
			: type(t), function(fn), reason(r) {}

	  public:

		/// Error message to print in "catch all" exit.
		std::string Message() const
		{
			std::string msg = type + " exception at " + function;
			if (!reason.empty())
				msg += ": " + reason;
			return msg;
		}
		/// Explanation for exception.
		std::string Reason() const
			{return reason;}
	};

	/// Syntax error in input data.
	class Syntax:public General
	{
	  public:
		
		/// Error in function \f$fn\f$ with message \f$r\f$.
		Syntax(const std::string& fn,const std::string& r="") : General("Syntax",fn,r) {}
	};

	/// Cannot perform required action.
	class Invalid:public General
	{
	  public:
		
		/// Error in function \f$fn\f$ with message \f$r\f$.
		Invalid(const std::string& fn,const std::string& r="") : General("Invalid",fn,r) {}
	};

	/// Error in I/O.
	class IO:public General
	{
	  public:
		
		/// Error in function \f$fn\f$ with message \f$r\f$.
		IO(const std::string& fn,const std::string& r="") : General("I/O",fn,r) {}
	};

	/// Illegal argument.
	class Range:public General
	{
	  public:
		
		/// Error in function \f$fn\f$ with message \f$r\f$.
		Range(const std::string& fn,const std::string& r="") : General("Range",fn,r) {}
	};

	/// Out of memory.
	class Memory:public General
	{
	  public:
		
		/// Out of memory error in function \f$fn\f$.
		Memory(const std::string& fn) : General("Memory",fn,"Out of memory.") {}
		/// Error in function \f$fn\f$ with message \f$r\f$.
		Memory(const std::string& fn,const std::string& r) : General("Memory",fn,r) {}
	};
	
	/// Not yet implemented.
	class NotYetImplemented:public General
	{
	  public:
		
		/// Not yet implemented error in function \f$fn\f$.
		NotYetImplemented(const std::string& fn) : General("NotYetImplemented",fn,"Not yet implemented.") {}
		/// Error in function \f$fn\f$ with message \f$r\f$.
		NotYetImplemented(const std::string& fn,const std::string& r) : General("NotYetImplemented",fn,r) {}
	};

	/// Quit the program.
	class Quit: public General
	{
		int exitcode;
		
	  public:
		/// Ask for quitting the application.
		Quit() : General("Quit","","Normal exit") {exitcode=0;}
		Quit(int code) : General("Quit","","Normal exit") {exitcode=code;}

		/// Exit code.
		int ExitCode() const
			{return exitcode;}
	};
}

#endif
