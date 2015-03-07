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

#include <fstream>
#include "localization.h"
#include "error.h"
#include "tools.h"

namespace Localization
{
	// Debug flag.
	bool debug;
	
	// Default language.
	static string default_language="en";

	// Current language.
	static string current_language="en";

	// Pointer to the dictionary of the current language.
	static map<string,map<string,string> >::iterator lang;
	
	// Dictionaries mapping (language code,message) pairs to translated versions.
	static map<string,map<string,string> > dictionary;
	
	// Language ISO 639 code table.
	static const char* iso639[]={
		"aa","Afar",
		"ab","Abkhazian",
		"af","Afrikaans",
		"am","Amharic",
		"ar","Arabic",
		"as","Assamese",
		"ay","Aymara",
		"az","Azerbaijani",
		"ba","Bashkir",
		"be","Byelorussian",
		"bg","Bulgarian",
		"bh","Bihari",
		"bi","Bislama",
		"bn","Bengali",
		"bo","Tibetan",
		"br","Breton",
		"ca","Catalan",
		"co","Corsican",
		"cs","Czech",
		"cy","Welsh",
		"da","Danish",
		"de","German",
		"dz","Bhutani",
		"el","Greek",
		"en","English",
		"eo","Esperanto",
		"es","Spanish",
		"et","Estonian",
		"eu","Basque",
		"fa","Persian",
		"fi","Finnish",
		"fj","Fiji",
		"fo","Faroese",
		"fr","French",
		"fy","Frisian",
		"ga","Irish",
		"gd","Scots Gaelic",
		"gl","Galician",
		"gn","Guarani",
		"gu","Gujarati",
		"ha","Hausa",
		"he","Hebrew",
		"hi","Hindi",
		"hr","Croatian",
		"hu","Hungarian",
		"hy","Armenian",
		"ia","Interlingua",
		"id","Indonesian",
		"ie","Interlingue",
		"ik","Inupiak",
		"is","Icelandic",
		"it","Italian",
		"iu","Inuktitut",
		"ja","Japanese",
		"jw","Javanese",
		"ka","Georgian",
		"kk","Kazakh",
		"kl","Greenlandic",
		"km","Cambodian",
		"kn","Kannada",
		"ko","Korean",
		"ks","Kashmiri",
		"ku","Kurdish",
		"ky","Kirghiz",
		"la","Latin",
		"ln","Lingala",
		"lo","Laothian",
		"lt","Lithuanian",
		"lv","Latvian, Lettish",
		"mg","Malagasy",
		"mi","Maori",
		"mk","Macedonian",
		"ml","Malayalam",
		"mn","Mongolian",
		"mo","Moldavian",
		"mr","Marathi",
		"ms","Malay",
		"mt","Maltese",
		"my","Burmese",
		"na","Nauru",
		"ne","Nepali",
		"nl","Dutch",
		"no","Norwegian",
		"oc","Occitan",
		"om","Oromo",
		"or","Oriya",
		"pa","Punjabi",
		"pl","Polish",
		"ps","Pashto, Pushto",
		"pt","Portuguese",
		"qu","Quechua",
		"rm","Rhaeto-Romance",
		"rn","Kirundi",
		"ro","Romanian",
		"ru","Russian",
		"rw","Kinyarwanda",
		"sa","Sanskrit",
		"sd","Sindhi",
		"sg","Sangho",
		"sh","Serbo-Croatian",
		"si","Sinhalese",
		"sk","Slovak",
		"sl","Slovenian",
		"sm","Samoan",
		"sn","Shona",
		"so","Somali",
		"sq","Albanian",
		"sr","Serbian",
		"ss","Siswati",
		"st","Sesotho",
		"su","Sundanese",
		"sv","Swedish",
		"sw","Swahili",
		"ta","Tamil",
		"te","Telugu",
		"tg","Tajik",
		"th","Thai",
		"ti","Tigrinya",
		"tk","Turkmen",
		"tl","Tagalog",
		"tn","Setswana",
		"to","Tonga",
		"tr","Turkish",
		"ts","Tsonga",
		"tt","Tatar",
		"tw","Twi",
		"ug","Uighur",
		"uk","Ukrainian",
		"ur","Urdu",
		"uz","Uzbek",
		"vi","Vietnamese",
		"vo","Volapuk",
		"wo","Wolof",
		"xh","Xhosa",
		"yi","Yiddish",
		"yo","Yoruba",
		"za","Zhuang",
		"zh","Chinese",
		"zu","Zulu",
		"",""
	};
	
	void SetLanguage(const string& code);
	
	const map<string,string>& Languages()
	{
		static map<string,string> ret;

		if(ret.size()==0)
			for(size_t j=0; iso639[j][0]; j+=2)
				ret[iso639[j]]=iso639[j+1];
		
		return ret;
	}

	string LanguageName(const string& code)
	{
		if(!IsLanguage(code))
			throw Error::Range("Localization::LanguageName(const string&)","invalid language code '"+code+"'");

		return (*Languages().find(code)).second;
	}
	
	bool IsLanguage(const string& code)
	{
		const map<string,string>& lang=Languages();

		return lang.find(code)!=lang.end();
	}

	void SetLanguage(const string& code)
	{
		if(!IsLanguage(code))
			throw Error::Range("Localization::SetLanguage(const string&)","invalid language code '"+code+"'");

		current_language=code;
		lang=dictionary.find(current_language);
		if(lang==dictionary.end())
			dictionary[current_language]=map<string,string>();
		lang=dictionary.find(current_language);
	}

	string GetLanguage()
	{
		return current_language;
	}

	string Filename(const string& filename, const string& code)
	{
		if(!IsLanguage(code))
			throw Error::Range("Localization::Filename(const string&,const string&)","invalid language code '"+code+"'");

		if(filename=="")
			throw Error::Invalid("Localization::Filename(const string&,const string&)","empty filename");

		if(code==default_language)
			return filename;

		int pos=filename.length()-1;

		while(pos>0)
		{			
			if(filename[pos]=='/')
				break;

			pos--;
		}
		
		return filename.substr(0,pos+1)+"/"+code+"/"+filename.substr(pos+1);
	}
	
	string File(const string& filename)
	{
		string localizedfile=Filename(filename,current_language);
		
		// avoid an IO if possible
		if(localizedfile!=filename && FileExist(localizedfile))
			return localizedfile;

		return filename;
	}

	void ReadDictionary(const string& filename)
	{
		ifstream F(filename.c_str());
		
		if(!F)
			throw Error::IO("Localization::ReadDictionary(const string&)","unable to open '"+filename+"'.");

		int linecount=0;
		
		string line;
		string code;
		
		string original;
		
		while(F)
		{                  
			line=Trim(readline(F));
                        if(F.eof())
                            break;
			linecount++;

			if(line=="" || line[0]=='#')
			{
				original="";
				continue;
			}
			
			code=line.substr(0,2);
			
			if(!IsLanguage(code))
				throw Error::Syntax("Localization::ReadDictionary(const string&)","invalid language code: '"+code+"'.");
				
			if(line.length() < 3 || line[2]!=':')
				throw Error::Syntax("Localization::ReadDictionary(const string&)","invalid line '"+line+"'");
			line=Trim(line.substr(3));
			if(line=="")
				continue;
			
			if(code==default_language)
			{
				if(original!="")
					throw Error::Syntax("Localization::ReadDictionary(const string&)","translation expected on line: "+ToString(linecount));
				original=line;
			}
			else
			{
				if(original=="")
					throw Error::Syntax("Localization::ReadDictionary(const string&)","original languge expected on line: "+ToString(linecount));
				dictionary[code][original]=line;
			}
		}
	}

	// Helper functions for Message() try split the string if there are
	// matches in dictionary.
	static bool TranslateMessage(string& message,bool split);
	
	static bool TrySplitting(string& message,const string& delimiter)
	{
		size_t i=0;

		if((i=message.find(delimiter,0))==string::npos)
			return false;

		string left,right;

		left=message.substr(0,i);
		right=message.substr(i+delimiter.length());

		if(!TranslateMessage(left,false))
			return false;

		if(!TranslateMessage(right,true))
			return false;

		message=left+delimiter+right;
		
		return true;
	}
	
	static bool TrySplitting(string& message)
	{
		if(TrySplitting(message,"/"))
			return true;
		if(TrySplitting(message,", "))
			return true;

		return false;
	}
	
	static bool TranslateMessage(string& message,bool split)
	{
		if(message=="")
			return true;
		
		if(default_language==current_language)
		{
			if(debug)
				cerr << "en: " << message << endl;
			return true;
		}

 		map<string,string>::const_iterator entry;

		entry=(*lang).second.find(message);
 		if(entry==(*lang).second.end())
		{
			bool splitted=false;
			
			if(split)
				splitted=TrySplitting(message);
			if(debug && split)
				cerr << "en: " << message << endl;
			
 			return splitted;
		}
		
		message=(*entry).second;

		return true;
	}

	// Store for %d,%e,...,%r notation value pairs.
	static map<string,string> message_numbers;

	// Remove numbers and replace with %d,%e,...,%r
	static bool CollectNumbers(string& message)
	{
		string ret;
		
		message_numbers.clear();
		char next[]="%d";

		for(size_t i=0; i<message.length();)
		{
			if((message[i]>='0' && message[i]<='9') || (message[i]=='-' && i+1<message.length() && message[i+1]>'0' && message[i+1]<='9'))
			{
				int i0=i;
				
				ret+=next;
				
				if(message[i]=='-')
					i++;
				while(i<message.length() && message[i]>='0' && message[i]<='9')
					i++;
 				message_numbers[next]=message.substr(i0,i-i0).c_str();
				next[1]++;
				if(next[1]=='s')
					return false;
			}
			else
			{
				ret=ret+message[i];
				i++;
			}
		}

		message=ret;
		return true;
	}

	// Restore removed numbers.
	static void ResubstituteNumbers(string& message)
	{
		map<string,string>::iterator i;
		size_t pos;
		
		for(i=message_numbers.begin(); i!=message_numbers.end(); i++)
		{
			pos=message.find((*i).first);
			if(pos!=string::npos)
				message.replace(pos,(*i).first.length(),(*i).second);
		}
	}
	
	string Message(const string& message)
	{
		string m=message;
		
		size_t spaces=0;
		while(spaces < m.length() && m[spaces]==' ')
			spaces++;
		if(spaces)
			m=m.substr(spaces,m.length()-spaces);
		
		bool resubst=CollectNumbers(m);
		
		TranslateMessage(m,true);

		if(resubst)
			ResubstituteNumbers(m);
		
		while(spaces--)
			m=" "+m;
		
		return m;
	}
	
	string Message(const string& message,const string& arg1)
	{
		string m=Message(message);

		size_t i=m.find("%s",0);
		if(i!=string::npos)
			m=m.replace(i,2,arg1);
			
		return m;
	}

	string Message(const string& message,const string& arg1,const string& arg2)
	{
		string m=Message(message);

		size_t i=m.find("%s",0);
		if(i!=string::npos)
			m=m.replace(i,2,arg1);

		i=m.find("%t",0);
		if(i!=string::npos)
			m=m.replace(i,2,arg2);

		return m;
	}
}
