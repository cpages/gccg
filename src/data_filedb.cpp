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

#include <sys/stat.h>
#include <sys/types.h>
#if !defined(__BCPLUSPLUS__) && !defined(_MSC_VER)
# include <unistd.h>
#elif !defined(_MSC_VER)
# include <dir.h>
#endif
#if defined(_MSC_VER)
# include "compat.h"
# include <direct.h>
#else
# include <dirent.h>
#endif
#include <fstream>
#include "security.h"
#include "carddata.h"
#include "data_filedb.h"
#include "parser_functions.h"

namespace Evaluator
{		
	// Debugging
	// =========

#ifdef FILEDB_DEBUG
	static inline string DbName(const DataFileDB* db)
	{
		static map<const DataFileDB*,string> debug_names;
		static int next_name;
		
		if(debug_names[db]=="")
			debug_names[db]="Db#"+ToString(++next_name);

		return debug_names[db];
	}
	
	inline void DataFileDB::Dump(const string& function,const string& description,const Data& data) const
	{
		cout << DbName(this) << " " << TypeToString(dbtype) << " [" << dir << "] " << function << endl;
//		cout << "   value now: " << tostr(*this).String().substr(0,80) << endl;
		cout << "   " << description;
		if(data!=Null)
			cout << " "<< tostr(data).String().substr(0,80) << endl;
		else
			cout << endl;
	}
#else
	inline void DataFileDB::Dump(const string& function,const string& description,const Data& data) const
	{
	}

	static inline string DbName(const DataFileDB* db)
	{
		return "";
	}
#endif
	
	// Construct & Destruct
	// ====================
	
	DataFileDB::DataFileDB()
	{
		CACHE_REFRESH_RATE=MINUTE;
		CACHE_MIN_AGE=MINUTE*15;
		CACHE_MAX_SINGLE_WRITE=1;
		CACHE_MAX_RESIDENT=100;
	
		dbtype=DBNone;
		dir="";

		Dump("DataFileDB()","create empty");
	}
	
	DataFileDB::DataFileDB(const DataFileDB& src)
	{
		CACHE_REFRESH_RATE=MINUTE;
		CACHE_MIN_AGE=MINUTE*15;
		CACHE_MAX_SINGLE_WRITE=1;
		CACHE_MAX_RESIDENT=100;
		
		dbtype=src.dbtype;
		dir="";
		
		Dump("DataFileDB(const DataFileDB&)","create from",src);

		if(src.dbtype!=DBNone)
			throw Error::Invalid("DataFileDB::DataFileDB(const DataFileDB&)","cannot create copy of non-empty database");			
	}

	DataFileDB::~DataFileDB()
	{
		SaveContent();
		Dump("~DataFileDB()","destruct");
	}

	// Utilities
	// =========
	
	FileDBType DataFileDB::StringToType(const string& s)
	{
		if(s=="DBNone")
			return DBNone;
		else if(s=="DBSingleFile")
			return DBSingleFile;
		else if(s=="DBStringKeys")
			return DBStringKeys;
		else
			throw Error::Invalid("DataFileDB::StringToType(FileDBType)","invalid type '"+s+"'");
	}

	string DataFileDB::TypeToString(FileDBType t)
	{
		if(t==DBNone)
			return "DBNone";
		else if(t==DBSingleFile)
			return "DBSingleFile";
		else if(t==DBStringKeys)
			return "DBStringKeys";
		else
			throw Error::Invalid("DataFileDB::TypeToString(FileDBType)","invalid type "+ToString(int(t)));
	}

	string DataFileDB::ReadFile(const string& filename) const
	{
		string f=dir+"/"+filename;
		
		ifstream F(f.c_str());
		if(!F)
			throw Error::IO("DataFileDB::ReadFile(const string&,const string&)","unable to read '"+f+"'");

		string line,content;
		
		while(F)
		{
			F >> line;
			if(F.eof())
				break;
			content+=line;
		}
		
		return content;
	}

	void DataFileDB::WriteFile(const string& filename,const string& s) const
	{
		string f=dir+"/"+filename;

		ofstream F(f.c_str());
		if(!F)
			throw Error::IO("DataFileDB::WriteFile(const string&,const string&)","unable to write '"+f+"'");

		F << s;
		F << endl;
	}
	
	// Database operations
	// ===================
	
	void DataFileDB::Attach(const Data& init, const string& variablename,FileDBType type)
	{
		dir=savedir+"/"+variablename+"-db";
		dbtype=DatabaseExist();

		security.WriteFile(dir);

		if(dbtype!=DBNone)
		{
			Dump("DataFileDB::Attach(const Data&, const string&, FileDBType)","load old",init);
			LoadContent();
		}
		else
		{
			dbtype=type;
			Dump("DataFileDB::Attach(const Data&, const string&, FileDBType)","create new",init);
			CreateEmpty();
			DataFileDB::operator=(init);
			MarkAllDirty();
		}
	}
	
	FileDBType DataFileDB::DatabaseExist() const
	{
		DIR* d;
		security.OpenDir(dir);
		d=opendir(dir.c_str());
		if(d)
		{
			closedir(d);
			if(FileExist((dir+"/type").c_str()))
				return StringToType(ReadFile("type"));
		}

		return DBNone;
	}

	void DataFileDB::DestroyContent()
	{
		Dump("DestroyContent()","remove content");

		if(dbtype==DBNone)
			return;
		else if(dbtype==DBSingleFile)
		{
			unlink((dir+"/value").c_str());
		}
		else if(dbtype==DBStringKeys)
		{
			unlink((dir+"/keys").c_str());
			for(size_t i=0; i<vec.size(); i++)
				unlink(FileName(vec[i][0].String()).c_str());
		}
		else
			throw Error::NotYetImplemented("DataFileDB::DestroyContent()");
	}
	
	void DataFileDB::CreateEmpty()
	{
		Dump("CreateEmpty()","initialize");

		if(DatabaseExist())
			throw Error::IO("DataFileDB::CreateEmpty()","database "+dir+" already created");

		if(dbtype==DBNone)
			throw Error::IO("DataFileDB::CreateEmpty()","cannot create database of type DBNone");
		else if(dbtype==DBSingleFile)
		{
#ifdef WIN32
			_mkdir(dir.c_str());
#else
			mkdir(dir.c_str(),0700);
#endif

			WriteFile("type",TypeToString(dbtype));
			WriteFile("value","NULL");
		}
		else if(dbtype==DBStringKeys)
		{
#ifdef WIN32
			_mkdir(dir.c_str());
			_mkdir((dir+"/data").c_str());
#else
			mkdir(dir.c_str(),0700);
			mkdir((dir+"/data").c_str(),0700);
#endif

			WriteFile("type",TypeToString(dbtype));
			WriteFile("keys","(,)");

			status=vector<DBEntryStatus>();
			MakeList();
		}
		else
			throw Error::NotYetImplemented("DataFileDB::CreateEmpty()");
	}

	void DataFileDB::LoadContent()
	{
		dbtype=StringToType(ReadFile("type"));

		if(dbtype==DBSingleFile)
		{
			Data::operator=(toval(ReadFile("value")));
		}
		else if(dbtype==DBStringKeys)
		{
			Data keys=toval(ReadFile("keys"));
			status=vector<DBEntryStatus>(keys.Size());
			MakeList(keys.Size());
			
			for(size_t i=0; i<vec.size(); i++)
			{
				vec[i]=keys[i];
				status[i].ondisk=true;
			}
		}
		else
			throw Error::NotYetImplemented("DataFileDB::LoadContent()");

		Dump("LoadContent()","load old data");
	}

	void DataFileDB::SaveContent()
	{
		if(IsDirty())
			SaveToDisk();
	}

	void DataFileDB::SaveToDisk()
	{
		Dump("SaveToDisk()","save all data");
		
		if(dbtype==DBSingleFile)
		{
			WriteFile("type",TypeToString(DBSingleFile));
			WriteFile("value",tostr(*this).String());
		}
		else if(dbtype==DBStringKeys)
		{
			WriteFile("type",TypeToString(DBStringKeys));
			Data keys;
			keys.MakeList(status.size());
			for(size_t i=0; i<status.size(); i++)
			{
				keys[i].MakeList(2);
				keys[i][0]=vec[i][0];
				SaveCache(i);
			}
			WriteFile("keys",tostr(keys).String());				
		}
		else
			throw Error::NotYetImplemented("DataToDisk::SaveContent()");

		MarkAllClean();
	}
	
	// Dirty flag
	// ==========
	
	bool DataFileDB::IsDirty() const
	{
		if(dbtype==DBNone)
			return false;
		else if(dbtype==DBSingleFile)
			return single_status.dirty;
		else if(dbtype==DBStringKeys)
		{
			if(status.size()==0)
				return true;
			
			for(size_t i=0; i<status.size(); i++)
				if(status[i].dirty)
					return true;

			return false;
		}
		else
			throw Error::NotYetImplemented("DataFileDB::IsDirty()");
	}

	void DataFileDB::MarkAllDirty()
	{
		if(dbtype==DBNone)
			return;
		else if(dbtype==DBSingleFile)
			single_status.dirty=true;
		else if(dbtype==DBStringKeys)
		{
			for(size_t i=0; i<status.size(); i++)
				status[i].dirty=true;
		}
		else
			throw Error::NotYetImplemented("DataFileDB::MarkAllDirty()");
		
	}

	void DataFileDB::MarkAllClean()
	{
		if(dbtype==DBNone)
			return;
		else if(dbtype==DBSingleFile)
			single_status.dirty=false;
		else if(dbtype==DBStringKeys)
		{
			for(size_t i=0; i<status.size(); i++)
				status[i].dirty=false;
		}
		else
			throw Error::NotYetImplemented("DataFileDB::MarkAllClean()");
		
	}

	void DataFileDB::MarkDirty(int index)
	{
		if(dbtype==DBNone)
			throw Error::IO("DataFileDB::MarkDirty(int)","cannot apply indices to database type DBNone");
		else if(dbtype==DBSingleFile)
			single_status.dirty=true;
		else if(dbtype==DBStringKeys)
			status[index].dirty=true;
		else
			throw Error::NotYetImplemented("DataFileDB::MarkDirty(int)");
	}

	// Cache handling
	// ==============
	void DataFileDB::Touch(int index) const
	{
		if(index < 0 || (size_t)index >= status.size())
			throw Error::Invalid("DataFileDB::Touch","invalid index "+ToString(index));
		
		::time(&status[index].access);
	}

	string DataFileDB::FileName(const string& entry) const
	{
		if(entry=="")
			return dir+"/data/[empty_string]";
		
		return dir+"/data/"+HexEncode(entry);
	}
	
	void DataFileDB::LoadCache(int index) const
	{
		if(index < 0 || (size_t)index >= status.size())
			throw Error::Invalid("DataFileDB::LoadCache","invalid index "+ToString(index));
		
		if(dbtype==DBStringKeys)
		{
			Touch(index);
		
			if(!status[index].ondisk)
				return;

			Dump("DataFileDB::LoadCache(int)","loading entry "+ToString(index)+": "+tostr(vec[index][0]).String());
				
			string filename=FileName(vec[index][0].String());
			security.ReadFile(filename);
			ifstream F(filename.c_str());
			if(!F)
				throw Error::IO("DataFileDB::LoadCache(int)","unable to read "+filename);
		
			string buffer;
			while(F)
				buffer+=readline(F);
			F.close();
		
			status[index].ondisk=false;
			vec[index][1]=toval(buffer);
		}
		else
			throw Error::NotYetImplemented("DataFileDB::LoadCache()");
	}

	bool DataFileDB::SaveCache(int index) const
	{
		if(index < 0 || (size_t)index >= status.size())
			throw Error::Invalid("DataFileDB::SaveCache","invalid index "+ToString(index));

		if(dbtype==DBStringKeys)
		{
			if(status[index].ondisk)
				return false;

			Dump("DataFileDB::SaveCache(int)","saving entry "+ToString(index)+": "+tostr(vec[index][0]).String());
		
			Touch(index);

			string filename=FileName(vec[index][0].String());
			security.WriteFile(filename);
		
			ofstream F(filename.c_str());
			if(!F)
				throw Error::IO("DataFileDB::SaveCache(int)","unable to write "+filename);
			PrettySave(F,vec[index][1]);
			F.close();

			status[index].ondisk=true;
			vec[index][1]=Null;
		}
		else
			throw Error::NotYetImplemented("DataFileDB::SaveCache()");
		
		return true;
	}

	int DataFileDB::Oldest() const
        {
            time_t oldest;
            int oldest_index=-1;

            ::time(&oldest);
            oldest++;

            for(size_t i=0; i<status.size(); i++)
            {
                if(!status[i].ondisk && status[i].access < oldest)
                {
                    oldest=status[i].access;
                    oldest_index=i;
                }
            }

            return oldest_index;
        }

	int DataFileDB::Loaded() const
        {
            int count=0;

            for(size_t i=0; i<status.size(); i++)
            {
                if(!status[i].ondisk)
                    count++;
            }

            return count;
        }

	void DataFileDB::CacheUpdate() const
	{
		static bool first_update=true;
		static time_t last_update;

		if(dbtype==DBStringKeys)
		{
			// Check if it is too early to update.
			if(first_update)
			{
				::time(&last_update);
				first_update=false;
				return;
			}
		
			time_t now;
			::time(&now);

			if(now-last_update < CACHE_REFRESH_RATE)
				return;

			last_update=now;

			if(status.size()==0)
				return;
			
			// Debug:
#ifdef FILEDB_DEBUG
// 			for(size_t i=0; i<status.size(); i++)
// 				cout << "   => Age of " << vec[i][0] << " (" << i << "): " << (now - status[i].access) << "s " << (status[i].ondisk ? "disk" : "memory") << endl;
#endif
			int save_count=0;
			int oldest_index;
                        time_t oldest;

                        // Check the maximum size for resident entries.
                        int overflow=Loaded() - CACHE_MAX_RESIDENT;
                        while(overflow > 0)
                        {
                            oldest_index=Oldest();
                            if(oldest_index==-1)
                                break;
                            SaveCache(oldest_index);
                            save_count++;
                            overflow--;
                        }
			
			// Find and save older entries.
			for(int count=CACHE_MAX_SINGLE_WRITE; count; count--)
			{
				oldest_index=Oldest();

				if(oldest_index==-1)
					break;

                                oldest=status[oldest_index].access;

				if(now - oldest <= CACHE_MIN_AGE)
					break;

				SaveCache(oldest_index);
				save_count++;
			}

			Dump("DataFileDB::CacheUpdate()","cache update of "+ToString(save_count)+" entries");
		}
		else
			throw Error::NotYetImplemented("DataFileDB::CacheUpdate()");
	}

	// Data access
	// ===========

	DataFileDB& DataFileDB::operator=(const DataFileDB& z)
	{
		Dump("operator=(const DataFileDB&)","copy from "+DbName(&z),z);

		if(z.dbtype!=DBNone)
			throw Error::Invalid("DataFileDB::operator=(const DataFileDB&)","cannot create copy of non-empty database");
		
		DestroyContent();
		
		dbtype=DBNone;
		dir="";

		return *this;
	}

	Data& DataFileDB::operator=(const Data& z)
	{
		Dump("operator=(const Data&)","copy from",z);
		
		DestroyContent();
		
		if(dbtype==DBNone)
			throw Error::IO("DataFileDB::operator=(const Data&)","cannot assign to database type DBNone");

		if(dbtype==DBSingleFile)
		{
			Data::operator=(z);
			MarkAllDirty();
			return *this;
		}
		else if(dbtype==DBStringKeys)
		{
			if(!z.IsList())
				throw LangErr("DataFileDB::operator=(const DataFileDB&)","only dictionaries can be stored into DBStringKeys");

			MakeList(z.Size());
			status=vector<DBEntryStatus>(z.Size());
			
			for(size_t i=0; i<z.Size(); i++)
			{
				if(!z[i].IsList(2))
					throw LangErr("DataFileDB::operator=(const DataFileDB&)","invalid dictionary entry "+tostr(z[i]).String());
				else if(!z[i][0].IsString())
					throw LangErr("DataFileDB::operator=(const DataFileDB&)","only string valued keys allowed: "+tostr(z[i][0]).String());

				vec[i]=z[i];
				Touch(i);
				MarkDirty(i);
			}
			
			return *this;
		}
		else
			throw Error::NotYetImplemented("DataFileDB::operator=(const Data&)");
	}
	
	Data& DataFileDB::operator[](int i)
	{
		if(i < 0 || i>=(int)vec.size())
			throw LangErr("DataFileDB::operator[](int)","invalid index "+ToString(i));
		
		if(dbtype==DBNone)
			throw LangErr("DataFileDB::operator[](int)","cannot use [] on database type DBNone");
		else if(dbtype==DBSingleFile)
		{
			MarkDirty(i);
			return Data::operator[](i);
		}
		else if(dbtype==DBStringKeys)
		{
			MarkDirty(i);
			LoadCache(i);
			CacheUpdate();
			return vec[i];
		}
		else
			throw Error::NotYetImplemented("DataFileDB::operator[](int)");
	}

	const Data& DataFileDB::operator[](int i) const
	{
		if(i < 0 || i>=(int)vec.size())
			throw LangErr("DataFileDB::operator[](int)","invalid index "+ToString(i));
		
		if(dbtype==DBNone)
			throw LangErr("DataFileDB::operator[](int)","cannot use [] on database type DBNone");
		else if(dbtype==DBSingleFile)
			return Data::operator[](i);
		else if(dbtype==DBStringKeys)
		{
			LoadCache(i);
			return vec[i];
		}
		else
			throw Error::NotYetImplemented("DataFileDB::operator[](int)");
	}

	// Key lookup
	// ==========
	
	size_t DataFileDB::KeyLookup(const Data& key,bool& already_exist) const
	{
//		Dump("DataFileDB::KeyLookup(const Data&,bool","key look up for ",key);
		
		if(dbtype==DBNone)
			throw LangErr("DataFileDB::KeyLookup(const Data&,bool&)","cannot do key lookup on database type DBNone");
		else if(dbtype==DBSingleFile)
		{
			return Data::KeyLookup(key,already_exist);
		}
		else if(dbtype==DBStringKeys)
		{
			if(!key.IsString())
				throw LangErr("DataFileDB::KeyLookup(const Data&,bool&)","invalid key type "+type_of(key).String()+" for DBStringKeys");

			size_t pos=Data::KeyLookup(key,already_exist);

			if(already_exist)
				Touch(pos);
			
			return pos;
		}
		else
			throw Error::NotYetImplemented("DataFileDB::KeyLookup(const Data&,bool&)");
			
	}

	Data* DataFileDB::FindKey(const Data& key)
	{
		if(dbtype==DBNone)
			throw LangErr("DataFileDB::FindKey(const Data&)","cannot do key lookup on database type DBNone");
		else if(dbtype==DBSingleFile)
		{
			MarkAllDirty();
			return Data::FindKey(key);
		}
		else if(dbtype==DBStringKeys)
		{
			bool is_old;
			size_t pos;
		
			pos=KeyLookup(key,is_old);

			if(is_old)
				LoadCache(pos);
			else
			{
				Data pair;

				pair.MakeList(2);
				pair[0]=key;
				InsertAt(pos,pair);
			}

			Touch(pos);
			MarkDirty(pos);
			CacheUpdate();
			
			return &vec[pos];
		}
		else
			throw Error::NotYetImplemented("DataFileDB::FindKey(const Data&)");
	}

	Data* DataFileDB::InsertAt(int pos,const Data& object)
	{
		if(dbtype==DBNone)
			throw LangErr("DataFileDB::InsertAt(int pos,const Data&)","cannot do key lookup on database type DBNone");
		else if(dbtype==DBSingleFile)
		{
			Data* ret=Data::InsertAt(pos,object);
			MarkDirty(pos);
			return ret;
		}
		else if(dbtype==DBStringKeys)
		{
			Data *ret=Data::InsertAt(pos,object);
			status.insert(status.begin()+pos,DBEntryStatus());
			MarkDirty(pos);
			
			return ret;
		}
		else
			throw Error::NotYetImplemented("DataFileDB::InsertAt(int pos,const Data&)");
	}

	Data DataFileDB::Keys() const
	{
		Data ret;		
		ret.MakeList(Size());
		
		for(size_t i=0; i<Size(); i++)
			ret[i]=Data::operator[](i)[0];

		return ret;
	}

	void DataFileDB::AddList(const Data& item)
	{
		if(dbtype==DBNone)
			throw LangErr("DataFileDB::AddList(const Data&)","cannot add list entries to type DBNone");
		else
			throw Error::NotYetImplemented("DataFileDB::AddList(const Data&)");
	}

	void DataFileDB::DelList(int index)
	{
		if(dbtype==DBNone)
			throw LangErr("DataFileDB::DelList(int)","cannot delete list entries from type DBNone");
		else if(dbtype==DBStringKeys)
		{
			if(index < 0 || index >= (int)vec.size())
				throw Error::Invalid("DataFileDB::DelList(int)","Index out of range");

			unlink(FileName(vec[index][0].String()).c_str());
			
			vec.erase(index);
			status.erase(status.begin()+index);
			SaveToDisk();
		}
		else
			throw Error::NotYetImplemented("DataFileDB::DelList(int)");
	}
}
