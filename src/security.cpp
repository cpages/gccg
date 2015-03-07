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
#include "security.h"
#include "tools.h"
#include "localization.h"

namespace Evaluator
{
	Security security;
	
	bool Security::ValidFilename(const string& file)
	{
		if(file=="" || file=="." || file=="..")
			return false;

                if(file.find("/../")!=string::npos)
                    return false;

                if(file.find("\\..\\")!=string::npos)
                    return false;

		return true;
	}
	
	bool Security::MatchString(const string& pattern,const string& s)
	{
		return JokerMatch(pattern,s);
	}

	bool Security::MatchPort(int pattern, int port)
	{
		return pattern==ANY_PORT || pattern==port;
	}

	void Security::AllowReadFile(const string& pattern)
	{
		cout << Localization::Message("Security manager: allow read file: %s",pattern) << endl;
		if(find(file_read.begin(),file_read.end(),pattern)==file_read.end())
			file_read.push_back(pattern);
	}
	void Security::AllowWriteFile(const string& pattern)
	{
		cout << Localization::Message("Security manager: allow write file: %s",pattern) << endl;
		if(find(file_write.begin(),file_write.end(),pattern)==file_write.end())
			file_write.push_back(pattern);
	}
	void Security::AllowReadWriteFile(const string& pattern)
	{
		AllowReadFile(pattern);
		AllowWriteFile(pattern);
	}
	void Security::AllowOpenDir(const string& pattern)
	{
		cout << Localization::Message("Security manager: allow open dir: %s",pattern) << endl;
		if(find(open_dir.begin(),open_dir.end(),pattern)==open_dir.end())
			open_dir.push_back(pattern);
	}
	void Security::AllowExecute(const string& pattern)
	{
		cout << Localization::Message("Security manager: allow execute: %s",pattern) << endl;
		if(find(execute.begin(),execute.end(),pattern)==execute.end())
			execute.push_back(pattern);
	}
	void Security::AllowConnect(const string& pattern,int port)
	{
		cout << Localization::Message("Security manager: allow connect: %s",pattern+"/"+ToString(port)) << endl;
		if(find(net_connect.begin(),net_connect.end(),pair<string,int>(pattern,port))==net_connect.end())
			net_connect.push_back(pair<string,int>(pattern,port));
	}
	void Security::AllowCreateSocket(int port)
	{
		cout << Localization::Message("Security manager: allow create socket: %s",ToString(port)) << endl;
		if(find(create_socket.begin(),create_socket.end(),port)==create_socket.end())
			create_socket.push_back(port);
	}

	bool Security::CanReadFile(const string& file)
	{
		if(disabled)
			return true;
		
		if(!ValidFilename(file))
			return false;
		
		list<string>::const_iterator i;
		for(i=file_read.begin(); i!=file_read.end(); i++)
			if(MatchString(*i,file))
				return true;
		
		return false;
	}
	bool Security::CanWriteFile(const string& file)
	{
		if(disabled)
			return true;
		if(!ValidFilename(file))
			return false;

		list<string>::const_iterator i;
		for(i=file_write.begin(); i!=file_write.end(); i++)
			if(MatchString(*i,file))
				return true;

		return false;
	}
	bool Security::CanOpenDir(const string& _dir)
	{
		if(disabled)
			return true;
		if(!ValidFilename(_dir))
			return false;

		string dir=_dir;
		if(dir.length() && dir[dir.length()-1]=='/')
			dir=dir.substr(0,dir.length()-1);
		
		list<string>::const_iterator i;
		for(i=open_dir.begin(); i!=open_dir.end(); i++)
			if(MatchString(*i,dir))
				return true;

		return false;
	}
	bool Security::CanExecute(const string& file)
	{
		if(disabled)
			return true;
		if(!ValidFilename(file))
			return false;

		list<string>::const_iterator i;
		for(i=execute.begin(); i!=execute.end(); i++)
			if(MatchString(*i,file))
				return true;

		return false;
	}
	bool Security::CanConnect(const string& host,int port)
	{
		if(disabled)
			return true;
		list<pair<string,int> >::const_iterator i;
		for(i=net_connect.begin(); i!=net_connect.end(); i++)
			if(MatchString((*i).first,host) && MatchPort((*i).second,port))
				return true;

		return false;
	}
	bool Security::CanCreateSocket(int port)
	{
		if(disabled)
			return true;
		list<int>::const_iterator i;
		for(i=create_socket.begin(); i!=create_socket.end(); i++)
			if(MatchPort(*i,port))
				return true;

		return false;
	}

	void Security::ReadFile(const string& file)
	{
		if(!CanReadFile(file))
			throw SecurityViolationError("Security::ReadFile(const string&)","access denied: '"+file+"'");
	}
	
	void Security::WriteFile(const string& file)
	{
		if(!CanWriteFile(file))
			throw SecurityViolationError("Security::WriteFile(const string&)","access denied: '"+file+"'");
	}
	
	void Security::OpenDir(const string& dir)
	{
		if(!CanOpenDir(dir))
			throw SecurityViolationError("Security::OpenDir(const string&)","access denied: '"+dir+"'");
	}
	
	void Security::Execute(const string& file)
	{
		if(!CanExecute(file))
			throw SecurityViolationError("Security::Execute(const string&)","access denied: '"+file+"'");
	}

	void Security::Connect(const string& server,int port)
	{
		if(!CanConnect(server,port))
			throw SecurityViolationError("Security::Connect(const string&,int)","access denied: "+server+":"+ToString(port));
	}
	
	void Security::CreateSocket(int port)
	{
		if(!CanCreateSocket(port))
			throw SecurityViolationError("Security::CreateSocket(int)","access denied: port "+ToString(port));
	}
}
