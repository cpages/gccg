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

#ifndef DATA_FILEDB_H
#define DATA_FILEDB_H

#include <time.h>
#include <string>
#include <set>
#include <map>
#include "data.h"

//#define FILEDB_DEBUG

namespace Evaluator
{
	enum FileDBType
	{
		DBNone,DBSingleFile,DBStringKeys
	};

	struct DBEntryStatus
	{
		/// True if entry is changed in memory.
		bool dirty;
		/// Last access time.
		time_t access;
		/// True if entry is stored on disk, not in memory.
		bool ondisk;

		DBEntryStatus()
			{dirty=false; ::time(&access); ondisk=false;}
	};
	
	class DataFileDB : public Data
	{		
		/// Type of the database.
		FileDBType dbtype;
		/// Full pathname of the database directory.
		string dir;
		/// Status of each vector database entry.
		mutable vector<DBEntryStatus> status;
		/// Status of single value database.
		DBEntryStatus single_status;

		void Dump(const string& function,const string& description,const Data& data) const;
		void Dump(const string& function,const string& description) const
			{Dump(function,description,Null);}
		void Dump(const string& function) const
			{Dump(function,"",Null);}
		
		/// Read content of the file in the database directory and return it's contents.
		string ReadFile(const string& filename) const;
		/// Write a string s to the file in the database directory.
		void WriteFile(const string& filename,const string& s) const;
		
		/// Return type of database if the database exist in the disk.
		FileDBType DatabaseExist() const;

		/// Return true if there is something to save on disk.
		bool IsDirty() const;
		/// Mark all entries as dirty.
		void MarkAllDirty();
		/// Mark all entries as clean.
		void MarkAllClean();
		/// Mark one vector entry ss dirty.
		void MarkDirty(int index);
		
		/// Initialize empty file structures to hold database.
		void CreateEmpty();
		/// Load content of the database to the empty instance.
		void LoadContent();
		/// Save everything to the disk if dirty.
		void SaveContent();
		/// Remove all disk files associated to the database.
		void DestroyContent();

		/// Update last access time.
		void Touch(int index) const;
		/// Convert a string to the cache entry filename.
		string FileName(const string& entry) const;
		/// Load vector element from disk if not already loaded.
		void LoadCache(int index) const;
		/// Save vector element to the disk and remove from memory if
		/// not already removed. Return 1, if the element was written to the disk.
		bool SaveCache(int index) const;
                /// Return the index of the oldest entry or -1 if empty.
		int Oldest() const;
		/// Check if enough time has passed since the last update. Search
		/// old entries and store them to the disk.
		void CacheUpdate() const;
		
		/// Return insertion position for key in hash and whether or not key was found.
		size_t KeyLookup(const Data& key,bool& already_exist) const;
		
	  public:
		
		/// Minimum delay between cache refresh in seconds.
		time_t CACHE_REFRESH_RATE;
		/// Minumum age of an entry to be written on disk in seconds.
		time_t CACHE_MIN_AGE;
		/// Maximum number of entries to write during single cache update.
		int CACHE_MAX_SINGLE_WRITE;
		/// Maximum number of entries in RAM.
		int CACHE_MAX_RESIDENT;

		static const time_t MINUTE=60;
		static const time_t HOUR=60*MINUTE;
		static const time_t DAY=24*HOUR;
		
		/// Default constructor.
		DataFileDB();
		/// Copy constructor.
		DataFileDB(const DataFileDB& data);

		virtual ~DataFileDB();

		/// Associate 'variable' with database and initialize it to the value 'init' if database does not exist already.
		void Attach(const Data& init, const string& variablename,FileDBType type);
		/// Replace the whole database changing it's type.
		DataFileDB& operator=(const DataFileDB& z);
		/// Set the current value for database.
		virtual Data& operator=(const Data& z);
		/// Indexed access to vectors.
		virtual Data& operator[](int i);
		/// Indexed access to vectors.
		virtual const Data& operator[](int i) const;

		virtual bool IsDatabase() const
			{return 1;}
                /// Return the number of entries currently loaded.
		int Loaded() const;

		/// Find a pair from the sorted list with first component as 'key'. Insert pair (key,NULL) if not found. Throw an error if this object is not a dictionary list.
		virtual Data* FindKey(const Data& key);
		/// Ensure that object is a list and insert object to the given position if valid.
		virtual Data* InsertAt(int pos,const Data& object);
		/// Return keys of a dictionary or NULL if not a dictionary.
		virtual Data Keys() const;
		/// Add an object to the list.
		virtual void AddList(const Data& item);
		/// Delete an object from the list.
		virtual void DelList(int index);

		/// Save all data to the disk.
		void SaveToDisk();

		/// Convert a string to the database type.
		static FileDBType StringToType(const string& s);
		/// Convert a database type to the string.
		static string TypeToString(FileDBType t);
	};
}
#endif
