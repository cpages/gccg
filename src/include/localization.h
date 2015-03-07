#ifndef LOCALIZATION_H
#define LOCALIZATION_H
/*
    Very simple localization library.
	
    Copyright (C) 2004 Tommi Ronkainen

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
#include <string>
#include <map>

using namespace std;

namespace Localization
{
	/// When set, output all failed message lookups to stderr.
	extern bool debug;
	
	/// Return ISO 639 mapping from language codes to language names.
	extern const map<string,string>& Languages();

	/// Return true of a string is valid language code.
	extern bool IsLanguage(const string& code);

	/// Return name of the language.
	extern string LanguageName(const string& code);
	
	/// Set current language (defaul 'en').
	extern void SetLanguage(const string& code);

	/// Return current language.
	extern string GetLanguage();

	/// Return the name of the localized version of the file, which is filename added
	/// by the dot and code if the filename does not contain
	/// dot itself. Oherwise add dot and code before the extension.
	extern string Filename(const string& filename, const string& code);
	
	/// Return localized filename if it exist or filename itself if not.
	extern string File(const string& filename);

	/// Return localized version of the message.
	/// If no translation is found, the message is splitted at "/" or
	/// ", " and each part is then translated separately. If that does
	/// not help, then original message is returned.
	extern string Message(const string& message);
	extern string Message(const string& message,const string& arg1);
	extern string Message(const string& message,const string& arg1,const string& arg2);

	/// Read in a message dictionary file.
	extern void ReadDictionary(const string& filename);
}

#endif
