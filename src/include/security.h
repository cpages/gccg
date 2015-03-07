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
#ifndef SECURITY_H
#define SECURITY_H

#include <string>
#include <list>
#include "error.h"

using namespace std;

namespace Evaluator
{
	/// Port number constant to refer any port.
	const int ANY_PORT=0;

	/// Security violation encountered.
	class SecurityViolationError  : public Error::General
	{
	  public:
		
		/// Security violation encountered.
		SecurityViolationError(const std::string& fn,const std::string& msg) : Error::General("SecurityViolationError",fn,msg) {}
	};

	/// Simple access control manager to keep track of file r/w access and networking permissions.
	class Security
	{
		/// Set if security manager is disabled.
		bool disabled;
		
		list<string> file_read;
		list<string> file_write;
		list<string> execute;
		list<string> open_dir;
		list<pair<string,int> > net_connect;
		list<int> create_socket;

		/// Return true if a filename does not contain '..'.
		bool ValidFilename(const string& filename);
		/// Return true if a string matches a pattern.
		bool MatchString(const string& pattern,const string& s);
		/// Return true if a port number matches a pattern.
		bool MatchPort(int pattern, int port);
			
	  public:

		/// Deny everything by default.
		Security()
			{Enable();}

		/// Deactivate security manager.
		void Disable()
			{disabled=true;}
		/// Activate security manager.
		void Enable()
			{disabled=false;}
			
		/// Allow file reading.
		void AllowReadFile(const string& pattern);
		/// Allow file writing.
		void AllowWriteFile(const string& pattern);
		/// Allow file reading and writing.
		void AllowReadWriteFile(const string& pattern);
		/// Allow executing of a script file.
		void AllowExecute(const string& pattern);
		/// Allow access to a directory listing.
		void AllowOpenDir(const string& pattern);
		/// Allow connecting to the specified server/port.
		void AllowConnect(const string& pattern,int port);
		/// Allow creation of the server socket.
		void AllowCreateSocket(int port);

		/// True if a file is readable.
		bool CanReadFile(const string& file);
		/// True if a file is writeable.
		bool CanWriteFile(const string& file);
		/// True if a directory can be browsed.
		bool CanOpenDir(const string& dir);
		/// True if a file is executable.
		bool CanExecute(const string& file);
		/// True if can connect to the server/port.
		bool CanConnect(const string& server,int port);
		/// True if can create a server socket at a port.
		bool CanCreateSocket(int port);

		/// Check if a file is readable. Throw an exception if not.
		void ReadFile(const string& file);
		/// Check if a file is writeable. Throw an exception if not.
		void WriteFile(const string& file);
		/// Check if a directory can be browsed. Throw an exception if not.
		void OpenDir(const string& dir);
		/// Check if a file is executable. Throw an exception if not.
		void Execute(const string& file);
		/// Check if can connect to the server/port. Throw an exception if not.
		void Connect(const string& server,int port);
		/// Check if can create a server socket at a port. Throw an exception if not.
		void CreateSocket(int port);
	};

	/// Shared security manager for scripting language.
	extern Security security;
}

#endif
