/*
  Gccg - Generic collectible card game.
  Copyright (C) 2001-2013 Tommi Ronkainen

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

/// Use formatted output, when saving variable values.
#define PRETTY_SAVE
/// Collect information about function calls.
//#define PERFORMANCE_ANALYSIS

#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <iostream>
#include <fstream>
#include <stack>
#include <vector>
#include <list>
#include <cstdlib>
#include <string.h>

#if !defined(__BCPLUSPLUS__) && !defined(_MSC_VER)
# include <unistd.h>
#endif
#if defined(_MSC_VER)
#include "compat.h"
#endif
#include "security.h"
#include "tools.h"
#include "version.h"
#include "data_filedb.h"
#include "parser_functions.h"
#ifdef PERFORMANCE_ANALYSIS
#include <sys/time.h>
#endif

#ifdef USE_SQUIRREL
#include <squirrel.h>

#endif
using namespace std;

/// Generic expression evaluator.
namespace Evaluator
{
    void InitializeLibcards();
    void InitializeLibnet();
#ifdef USE_SQUIRREL
    void InitializeLibsquirrel();
#endif

    /// Throw language exception complaining about arguments.
    void ArgumentError(const string& fn,const Data& arg);

    /// External shared functions. These functions simply maps Data object to Data object.
    extern map<string,Data (*)(const Data& )> external_function;

    /// Substitute stringized value to the string by replacing each '#' character.
    string Substitute(const string& string,const Data& value);

#ifdef USE_SQUIRREL
	// Prototypes for the Squirrel interface library implementations
	//
	// Note that these shouldn't be called directly
	// but only by the parser's internal *_sq
	// functions as those set the foreign pointer
	// to the parser instance, which is required
	// to call GCCG script functions.
	namespace Libsquirrel {
		HSQUIRRELVM start_sq_impl(const Data& args);
		Data stop_sq_impl(HSQUIRRELVM sq, const Data& args);
		Data call_sq_impl(HSQUIRRELVM sq, const Data& args);
		Data eval_sq_impl(HSQUIRRELVM sq, const Data& args);
		Data get_sq_impl(HSQUIRRELVM sq, const Data& args);
		Data load_sq_impl(HSQUIRRELVM sq, const Data& args);
        
        // Objects of subtypes of this type can be stored in the foreignptr
        // of the Squirrel VM and thus allow invoking Call().
        class SQProxyHelper {
            public:
                virtual Data call(Data &arg) = 0;
        };
        
        // Parent for application classes used in Parser so that
        // I can get an object to invoke Call() on the parser.
        class SQHelper {
            public:
                // Create a heap allocated proxy object.
                // The user must free this.
                virtual SQProxyHelper* make_sq_proxy_helper() = 0;
        };
	};

#endif
    /// Generic parser.
    template <class Application> class Parser
	{
	    /// Variable values.
	    map<string,Data> variable;
	    /// Database variables.
	    map<string,DataFileDB> database;
	    /// Application defined functions.
	    typedef Data (Application::*pf)(const Data&);
	    map<string, pf> function;

	    /// Internal functions. These functions require access to Parser internals.
	    typedef Data (Parser<Application>::*pif)(const Data&);
	    map<string, pif> internal_function;

	    /// Function argument and return value stack for user defined functions.
	    stack<Data> argument_stack;
#ifdef STACK_TRACE
	    /// Function call stack of function names called.
	    stack<string> function_call_stack;
#endif
#ifdef PERFORMANCE_ANALYSIS
	    map<string,long> perf_total_time;
	    map<string,int> perf_calls;
	    string perf_current_function;
	    long perf_last_stamp;

	    void PerfCall(const string& func,string& store_old)
		{
		    timeval t;
		    long usec;
			
		    gettimeofday(&t,0);
		    usec=t.tv_usec + t.tv_sec * 1000000L;
			
		    if(perf_current_function != "")
			perf_total_time[perf_current_function]+=(usec-perf_last_stamp);

		    store_old=perf_current_function;
		    perf_calls[func]++;
		    perf_last_stamp=usec;
		    perf_current_function=func;
		}
		
	    void PerfReturn(const string& old_func)
		{
		    timeval t;
		    long usec;
			
		    gettimeofday(&t,0);
		    usec=t.tv_usec + t.tv_sec * 1000000L;
			
		    perf_total_time[perf_current_function]+=(usec-perf_last_stamp);
		    perf_last_stamp=usec;
		    perf_current_function=old_func;
		}
		
#endif
	    /// Data stack for local variables etc.
	    stack<Data> data_stack;
	    /// User defined functions.
	    map<string,string> user_function;
	    /// Current indentation level of dump output.
	    int dump_indent;
	    /// Pointer to the object which uses this parser.
	    Application* user;
#ifdef USE_SQUIRREL
    
        /// Squirrel VM instance.
        HSQUIRRELVM squirrel_vm;
        /// If Squirrel has been started.
        bool squirrel_started;

#endif

	    /// Change indentation level and output spaces to stderr.
	    void Tab(int adjust)
		{
		    if(adjust < 0)
			dump_indent+=adjust;
				
		    for(int i=0; i<dump_indent; i++)
			cerr << ' ';

		    if(adjust > 0)
			dump_indent+=adjust;
		}
	    /// Read argument list with parenthesis. Empty parenthesis returns NULL, single argument is returned as it is and two or more arguments are converted to a list. String reference is updated.
	    void EvalParenthesis(const char*& _src,Data& ret);
	    /// Evaluate atom.
	    void EvalAtom(const char*& _src,Data& ret);
	    /// Evaluate term.
	    void EvalTerm(const char*& _src,Data& ret);
	    /// Evaluate sum.
	    void EvalSum(const char*& _src,Data& ret);
	    /// Evaluate bits.
	    void EvalBit(const char*& _src,Data& ret);
	    /// Evaluate relation.
	    void EvalRelation(const char*& _src,Data& ret);
	    /// Evaluate expression.
	    void EvalExpression(const char*& _src,Data& ret);
	    /// Evaluate single statement.
	    void EvalStatement(const char*& _src,Data& ret);
	    /// Evaluate block or single statement.
	    void EvalBlock(const char*& _src,Data& ret);
		
	public:

	    /// Public internal function: vardump
	    Data vardump(const Data& arg); 
	    /// Public internal function: stacktrace
	    Data stacktrace(const Data& arg);
		
	    /// Create a parser.
	    Parser(Application* base);
	    /// Make performance dump, if in use.
	    ~Parser();
	    /// Evaluate given expression.
	    Data operator()(const string& expr);
	    /// Get reference to variable.
	    Data& Variable(const string& var);
	    /// Set variable value.
	    void SetVariable(const string& var,const Data& val);
	    /// Unset variable value.
	    void UnsetVariable(const string& var);
	    /// Declare external function.
	    void SetFunction(const string& fn,Data (*)(const Data& ));
	    /// Declare application defined function.
	    void SetFunction(const string& fn,Data (Application::*)(const Data& ));
	    /// Declare user defined function.
	    void SetFunction(const string& fn,const string& code);
	    /// Clear both argument and data stacks.
	    void ClearStacks()
		{
		    while(data_stack.size())
			data_stack.pop();
		    while(argument_stack.size())
			argument_stack.pop();
#ifdef STACK_TRACE
		    while(function_call_stack.size())
			function_call_stack.pop();
#endif
		}

	    // Internal functions.		
	    Data call(const Data& arg); 
	    Data _return(const Data& arg); 
	    Data apply(const Data& arg); 
	    Data attach(const Data& arg); 
	    Data binary_load(const Data& arg); 
	    Data binary_save(const Data& arg); 
	    Data cache_parameters(const Data& arg); 
	    Data cache_size(const Data& arg); 
	    Data del_entry(const Data& arg); 
	    Data delsaved(const Data& arg); 
	    Data execute(const Data& arg); 
	    Data forall(const Data& arg); 
	    Data isfunction(const Data& arg); 
	    Data isvar(const Data& arg); 
	    Data keys(const Data& arg); 
	    Data load(const Data& arg); 
	    Data pop(const Data& arg); 
	    Data push(const Data& arg); 
	    Data repeat(const Data& arg); 
	    Data save(const Data& arg); 
	    Data select(const Data& arg); 
	    Data valueof(const Data& arg); 
	    Data sort_fn(const Data& arg); 
#ifdef USE_SQUIRREL

        // Internal Squirrel interface functions.       
	    Data start_sq(const Data& arg); 
	    Data stop_sq(const Data& arg); 
	    Data call_sq(const Data& arg); 
	    Data eval_sq(const Data& arg); 
	    Data load_sq(const Data& arg); 
	    Data get_sq(const Data& arg); 
        
        SQInteger call_gccg_from_sq(HSQUIRRELVM sq); // Callback
#endif
	};

    //
    // Parser implementation
    // =====================

    template <class Application> Parser<Application>::~Parser()
	{
#ifdef PERFORMANCE_ANALYSIS
	    vector<string> sorted(perf_total_time.size());
		
	    cout << "--------------------" << endl;
	    cout << "Performance analysis" << endl;
	    cout << "--------------------" << endl;
		
	    map<string,long>::iterator i;
	    int j=0;
	    for(i=perf_total_time.begin(); i!=perf_total_time.end(); i++)
		sorted[j++]=(*i).first;

	    string tmp;
		
	    cout << "Total time:" << endl;
	    for(size_t j=0; j<sorted.size()-1; j++)
		for(size_t k=j+1; k<sorted.size(); k++)
		    if(perf_total_time[sorted[j]] < perf_total_time[sorted[k]])
		    {
			tmp=sorted[j];
			sorted[j]=sorted[k];
			sorted[k]=tmp;
		    }

	    for(size_t j=0; j<sorted.size(); j++)
		cout << " " << sorted[j] << " " << double(perf_total_time[sorted[j]])/1000.0 << "ms" << endl;

	    cout << "Number of calls:" << endl;
	    for(size_t j=0; j<sorted.size()-1; j++)
		for(size_t k=j+1; k<sorted.size(); k++)
		    if(perf_calls[sorted[j]] < perf_calls[sorted[k]])
		    {
			tmp=sorted[j];
			sorted[j]=sorted[k];
			sorted[k]=tmp;
		    }

	    for(size_t j=0; j<sorted.size(); j++)
		cout << " " << sorted[j] << " " << perf_calls[sorted[j]] << " times" << endl;

	    cout << "Average time:" << endl;
	    for(size_t j=0; j<sorted.size()-1; j++)
		for(size_t k=j+1; k<sorted.size(); k++)
		    if(perf_total_time[sorted[j]]/perf_calls[sorted[j]] < perf_total_time[sorted[k]]/perf_calls[sorted[k]])
		    {
			tmp=sorted[j];
			sorted[j]=sorted[k];
			sorted[k]=tmp;
		    }

	    for(size_t j=0; j<sorted.size(); j++)
		cout << " " << sorted[j] << " " << double(perf_total_time[sorted[j]])/perf_calls[sorted[j]]/1000.0 << "ms" << endl;

	    cout << "Alphabetically:" << endl;
		
	    for(i=perf_total_time.begin(); i!=perf_total_time.end(); i++)
	    {
		cout << " " << (*i).first << " " << double((*i).second)/1000.0 << "ms (called " << perf_calls[(*i).first] << " times) avg:" <<  double((*i).second)/1000.0/perf_calls[(*i).first] << " ms"  << endl;
	    }

#endif
	}
	
    template <class Application> void Parser<Application>::EvalParenthesis(const char*& _src,Data& ret)
	{
	    register const char* src=_src;
	    Data elem;

	    ret=Null;
		
	    if(*src==0 || *src!='(')
		throw LangErr("Parser<Application>::EvalParenthesis(const char*& src)","Invalid function arguments '"+string(src)+"'");
	    src++;
		
	    // Null
	    if(*src==')')
	    {
		src++;
		_src=src;
		return;
	    }

	    // Empty list
	    if(CheckFor(",)",src))
	    {
		ret.MakeList();
		_src=src;
		return;
	    }
		
	    // Single object or list
	    EvalExpression(src,elem);
		
	    EatWhiteSpace(src);
		
	    if(*src!=',' && *src!=')')
		throw LangErr("Parser<Application>::EvalParenthesis(const char*& src)","Missing ',' or ')' in '"+string(_src)+"'");

	    if(*src==')')
	    {
		src++;
		_src=src;
		ret=elem;
		return;
	    }
		
	    // List

	    ret.MakeList();
	    ret.AddList(elem);

	    for(;;)
	    {
		if(*src!=',')
		    throw LangErr("Parser<Application>::EvalParenthesis(const char*& src)","Missing ',' in '"+string(_src)+"'");
			
		src++;			
		EatWhiteSpace(src);
		if(*src==')')
		    break;
			
		EvalExpression(src,elem);
		ret.AddList(elem);
			
		EatWhiteSpace(src);
		if(*src==')')
		    break;
	    }
		
	    src++;

	    _src=src;
	    return;
	}

    template <class Application> void Parser<Application>::EvalAtom(const char*& _src,Data& ret)
	{
	    Data parenth;
	    Data* var;
	    string name;
	    string debugname;
	    register const char* src=_src;
		
	    EatWhiteSpace(src);

	    if(*src==0)
		throw LangErr("Parser<Application>::EvalAtom(string&)","null expression");		

	    // Negation
	    if(*src=='-')
	    {
		src++;
		EvalAtom(src,ret);
		ret=-ret;
		_src=src;
		return;
	    }

	    // String constant
	    if(*src=='"' || *src=='\'')
	    {
		string str;
		ReadString(src,str);

		_src=src;
		ret=str;
		return;
	    }

	    // Integer or real constant
	    if((*src >= '0' && *src <= '9') || *src == '.')
	    {
		bool real = (*src == '.');

		src++;
		while(*src && ((*src >= '0' && *src <= '9') || (*src=='.' && !real)))
		{
		    if(*src=='.')
			real=true;
		    src++;
		}
			
		if(real)
		{
		    double r=atof(_src);
		    _src=src;
		    ret=r;
		    return; // note: this may accept more characters than prevoius scan
		}
		else
		{
		    int n=atoi(_src);
		    _src=src;
		    ret=n;
		    return;
		}
	    }

	    // Subexpression

	    if(*src == '(')
	    {
		EvalParenthesis(src,ret);
		if(*src=='[' || *src=='{')
		{
		    if(debug)
			debugname="(<exp>)";
				
		    parenth=ret;
		    var=&parenth;
		    goto indexing;
		}
			
		_src=src;
		return;
	    }
		
	    // NULL
		
	    if(CheckFor("NULL",src))
	    {
		_src=src;
		ret=Null;
		return;
	    }

	    // Symbolic name (function or variable)
		
	    GetVariable(src,name);
		
	    if(name != "")
	    {
		EatWhiteSpace(src);

		// Functions
		if(*src=='(')
		{
		    EvalParenthesis(src,ret);

		    // Special function eval
		    if(name=="eval")
		    {
			if(debug)
			{
			    Tab(2);
			    cerr << "Fnc: eval(" << tostr(ret) << ")" << endl;
			}
			if(ret.IsString())
			{
#ifdef STACK_TRACE
			    function_call_stack.push("eval");
			    function_call_stack.push(tostr(ret).String());
#endif
#ifdef PERFORMANCE_ANALYSIS
			    string old;
			    PerfCall("eval",old);
#endif
			    ret=(*this)(ret.String()+";");
#ifdef PERFORMANCE_ANALYSIS
			    PerfReturn(old);
#endif
#ifdef STACK_TRACE
			    if(function_call_stack.size())
				function_call_stack.pop();
			    if(function_call_stack.size())
				function_call_stack.pop();
#endif
			}
					
			_src=src;
			return;
		    }

		    if(debug)
		    {
			Tab(2);
			cerr << "Fnc: " << name << "(" << tostr(ret) << ")" << endl;					
		    }
				
		    // Application defined function
		    if(function.find(name)!=function.end())
		    {
#ifdef STACK_TRACE
			function_call_stack.push(name);
			function_call_stack.push(tostr(ret).String());
#endif
#ifdef PERFORMANCE_ANALYSIS
			string old;
			PerfCall(name,old);
#endif
			ret=(user->*(function[name]))(ret);
#ifdef PERFORMANCE_ANALYSIS
			PerfReturn(old);
#endif
#ifdef STACK_TRACE
			if(function_call_stack.size())
			    function_call_stack.pop();
			if(function_call_stack.size())
			    function_call_stack.pop();
#endif
		    }
		    // External function
		    else if(external_function.find(name)!=external_function.end())
		    {
#ifdef STACK_TRACE
			function_call_stack.push(name);
			function_call_stack.push(tostr(ret).String());
#endif
#ifdef PERFORMANCE_ANALYSIS
			string old;
			PerfCall(name,old);
#endif
			ret=(*external_function[name])(ret);
#ifdef PERFORMANCE_ANALYSIS
			PerfReturn(old);
#endif
#ifdef STACK_TRACE
			if(function_call_stack.size())
			    function_call_stack.pop();
			if(function_call_stack.size())
			    function_call_stack.pop();
#endif
		    }
		    // Internal function
		    else if(internal_function.find(name)!=internal_function.end())
		    {
#ifdef STACK_TRACE
			function_call_stack.push(name);
			function_call_stack.push(tostr(ret).String());
#endif
#ifdef PERFORMANCE_ANALYSIS
			string old;
			PerfCall(name,old);
#endif
			ret=(this->*internal_function[name])(ret);
#ifdef PERFORMANCE_ANALYSIS
			PerfReturn(old);
#endif
#ifdef STACK_TRACE
			if(function_call_stack.size())
			    function_call_stack.pop();
			if(function_call_stack.size())
			    function_call_stack.pop();
#endif
		    }
		    // User defined function
		    else if(user_function.find(name) != user_function.end())
		    {
			CHECK_STACK_OVERFLOW;
			argument_stack.push(Data());
			argument_stack.push(ret);
			string fn=user_function[name];
#ifdef STACK_TRACE
			function_call_stack.push(name);
			function_call_stack.push(tostr(ret).String());
#endif
#ifdef PERFORMANCE_ANALYSIS
			string old;
			PerfCall(name,old);
#endif
			(*this)(fn);
#ifdef PERFORMANCE_ANALYSIS
			PerfReturn(old);
#endif
#ifdef STACK_TRACE
			if(function_call_stack.size())
			    function_call_stack.pop();
			if(function_call_stack.size())
			    function_call_stack.pop();
#endif
			argument_stack.pop();
			ret=argument_stack.top();
			argument_stack.pop();
		    }
		    else
			throw LangErr("Parser<Application>::EvalAtom(string&)","unknown function '"+name+"' called");

		    if(debug)
		    {
			Tab(-2);
			cerr << "Out: " << tostr(ret) << endl;
		    }

		    _src=src;
		    return;
		}

		// Variables

		if(name=="ARG")
		{
		    if(!argument_stack.size())
			throw LangErr("Parser<Application>::EvalAtom(string&)","stack is empty");
		    var=&argument_stack.top();
		}
		else
		{
		    var=&Variable(name);
				
		    if(debug)
			debugname+=name;
		}

		// Update variable pointer if array indexing found.

	    indexing:
			
		while(*src=='[' || *src=='{') // Array or dictionary
		{
		    Data index;
		    bool dictionary=(*src=='{');
		    char delimiter=dictionary ? '}' : ']';

		    src++;
		    EatWhiteSpace(src);
				
		    EvalExpression(src,index);

		    EatWhiteSpace(src);

		    if(*src!=delimiter)
			throw LangErr("Parser<Application>::EvalAtom(string&)","Missing ']' or '}'");

		    if(debug)
		    {
			if(delimiter=='}')
			    debugname+="{"+tostr(index).String()+"}";
			else
			    debugname+="["+tostr(index).String()+"]";
		    }
					
		    src++;
				
		    if(!dictionary && !index.IsInteger())
			throw LangErr("Parser<Application>::EvalAtom(string&)","Array index '"+tostr(index).String()+"' is not integer");

		    if(!var->IsList())
		    {
			string d;
			if(delimiter=='}')
			    d="{"+tostr(index).String()+"}";
			else
			    d="["+tostr(index).String()+"]";
			throw LangErr("Parser<Application>::EvalAtom(string&)","Cannot use "+d+" on '"+tostr(*var).String()+"' which is not a list. Code: "+string(_src));
		    }

		    else if(dictionary) // Dictionary
		    {
			var=&(*var->FindKey(index))[1];
		    }
		    else // Array
		    {
			int i=index.Integer();
			if(i < 0 || size_t(i) >= var->Size())
			    throw LangErr("Parser<Application>::EvalAtom(string&)","Index '"+tostr(index).String()+"' out of range applied on "+tostr(*var).String());
			else
				var=&(*var)[i];
		    }
		}
			
		if(*src=='=' && *(src+1) != '=') // Substitution
		{
		    src++;
		    EatWhiteSpace(src);
		    EvalExpression(src,ret);
		    *var=ret;

		    if(debug)
		    {
			Tab(0);
			cerr << "Set: " << debugname << "=" << tostr(ret).String() << endl;
		    }
				
		    _src=src;
		    return;
		}

		// Variable value

		_src=src;
		ret=*var;
		return;

	    } // if((name=GetVariable(s)) != "")

	    throw LangErr("Parser<Application>::EvalAtom(string&)","Syntax error '"+string(_src)+"'");
	}

    template <class Application> void Parser<Application>::EvalTerm(const char*& _src,Data& ret)
	{
	    register const char* src=_src;
	    char op;
	    Data e;
		
	    EvalAtom(src,ret);
		
	    EatWhiteSpace(src);
		
	    while(*src=='*' || *src=='/' || *src=='%')
	    {
		op=*src;
		src++;
			
		EatWhiteSpace(src);
		EvalTerm(src,e);
		if(op=='*')
		    ret=(ret * e);
		else if(op=='%')
		    ret=(ret % e);
		else
		    ret=(ret / e);
		EatWhiteSpace(src);
	    }

	    _src=src;
	    return;
	}

    template <class Application> void Parser<Application>::EvalSum(const char*& _src,Data& ret)
	{
	    register const char* src=_src;
	    Data e;
	    char op;
		
	    EvalTerm(src,ret);
	    EatWhiteSpace(src);
		
	    while(*src=='+' || *src=='-')
	    {
		op=*src;
		src++;
		EatWhiteSpace(src);
		if(op=='+')
		{
		    EvalTerm(src,e);
		    ret=ret+e;
		}
		else
		{
		    EvalTerm(src,e);
		    ret=ret-e;
		}
		EatWhiteSpace(src);
	    }

	    _src=src;
	}

    template <class Application> void Parser<Application>::EvalBit(const char*& _src,Data& ret)
	{
	    register const char* src=_src;
	    Data e;
	    char op;
		
	    EvalSum(src,ret);
	    EatWhiteSpace(src);
		
	    while((*src=='|' && *(src+1) != '|') || (*src=='&' && *(src+1) != '&') || *src=='^')
	    {
		op=*src;
		src++;
		EatWhiteSpace(src);
		if(op=='|') //bitwise OR
		{
		    EvalSum(src,e);
		    ret=ret | e;
		}
		else if(op=='&') //bitwise AND
		{
		    EvalSum(src,e);
		    ret=ret & e;
		}
		else //bitwise XOR
		{
		    EvalSum(src,e);
		    ret=ret ^ e;
		}
		EatWhiteSpace(src);
	    }

	    _src=src;
	}

    template <class Application> void Parser<Application>::EvalRelation(const char*& _src,Data& ret)
	{
	    register const char* src=_src;
	    Data e;
	    char op;
	    bool eq=false;

	    EatWhiteSpace(src);
	    if(*src=='!')
	    {
		src++;
		EvalRelation(src,ret);
		_src=src;
		if(ret.Integer())
		    ret=0;
		else
		    ret=1;
		return;
	    }
			
	    EvalBit(src,ret);
	    EatWhiteSpace(src);
	    while(*src=='<' || *src=='>' || *src=='=' || *src=='!')
	    {
		op=*src;
		src++;
		if(*src=='=')
		{
		    eq=true;
		    src++;
		}
			
		EvalBit(src,e);
			
		if(op=='=' && eq)
		    ret=Data((int)(ret == e));
		else if(op=='!' && eq)
		    ret=Data((int)!(ret == e));
		else if(op=='<')
		{
		    if(eq)
			ret=Data((int)!(e < ret));
		    else
			ret=Data((int)(ret < e));
		}
		else if(op=='>')
		{
		    if(eq)
			ret=Data((int)!(ret < e));
		    else
			ret=Data((int)(e < ret));
		}
		else
		    throw LangErr("Parser<Application>::EvalRelation(string&)","Invalid relation "+string(_src));
			
		EatWhiteSpace(src);
	    }

	    _src=src;
	}
	
    template <class Application> void Parser<Application>::EvalExpression(const char*& _src,Data& ret)
	{
	    register const char* src=_src;
	    Data e;
		
	    EvalRelation(src,ret);
		
	    while(1)
	    {
		EatWhiteSpace(src);
		if(*src==')' || *src==';' || *src=='}' || *src==']' || *src==',')
		    break;

		if(CheckFor("||",src))
		{
			EvalRelation(src,e);
			ret=(ret || e);
		}
		else if(CheckFor("&&",src))
		{
			EvalRelation(src,e);
			ret=(ret && e);
		}
		else
		    throw LangErr("Parser<Application>::EvalExpression(const char*&)","Invalid expression '"+string(_src)+"'");
	    }

	    _src=src;
	}

    template <class Application> void Parser<Application>::EvalStatement(const char*& _src,Data& ret)
	{
	    register const char* src=_src;
	    register const char* tmp;
	    ret=Null;
		
	    EatWhiteSpace(src);
	    if(*src=='}' || *src==0)
	    {
		_src=src;
		return;
	    }
			
	    if(*src==';')
	    {
		src++;
		_src=src;
		return;
	    }

	    if(CheckFor("if",src,1))
	    {
		bool truth;
		string choice;
		Data truthtmp;
		EvalParenthesis(src,truthtmp);
		truth=truthtmp.Integer() != 0;
			
		if(truth)
		    ReadStatement(src,choice);
		else
		    ReadStatement(src);

		EatWhiteSpace(src);

		while(CheckFor("elseif",src))
		{
		    if(choice!="")
		    {
			ReadParenthesis(src);
			ReadStatement(src);
		    }
		    else
		    {
			EvalParenthesis(src,truthtmp);
			truth=truthtmp.Integer() != 0;
			if(truth)
			    ReadStatement(src,choice);
			else
			    ReadStatement(src);
		    }
				
		    EatWhiteSpace(src);
		}
		if(CheckFor("else",src))
		{
		    if(choice=="")
			ReadStatement(src,choice);
		    else
			ReadStatement(src);
		}
			
		_src=src;
		register const char* tmp;
		tmp=choice.c_str();
		EvalBlock(tmp,ret);
		return;
	    }

	    if(CheckFor("while",src))
	    {
		string exp,block;
		Data truthtmp;

		EatWhiteSpace(src);

		ReadParenthesis(src,exp);
		ReadStatement(src,block);
			
		EatWhiteSpace(src);

		tmp=exp.c_str();
		while(EvalParenthesis(tmp,truthtmp),truthtmp.Integer())
		{
		    tmp=block.c_str();
		    EvalBlock(tmp,ret);
		    tmp=exp.c_str();
		}
			
		_src=src;
		return;
	    }

	    tmp=src;
	    if(CheckFor("for",src))
	    {
		EatWhiteSpace(src);
		if(CheckFor("(",src))
		{
		    Data forlist;
		    string block;
		    string var;
				
		    EatWhiteSpace(src);
		    GetVariable(src,var);
		    EatWhiteSpace(src);
				
		    if(var=="")
			throw LangErr("Parser<Application>::EvalStatement(string&)","for: missing variable");
		    if(*src!=')')
			throw LangErr("Parser<Application>::EvalStatement(string&)","for: missing ')' after variable "+var);

		    src++;
				
		    EvalParenthesis(src,forlist);
		    ReadStatement(src,block);

		    // Integer loop 0..n
		    if(forlist.IsInteger())
		    {
			int n=forlist.Integer();
					
			data_stack.push(variable[var]);

			for(int i=0; i<n; i++)
			{
			    variable[var]=i;
			    tmp=block.c_str();
			    EvalBlock(tmp,ret);
			    i=variable[var].Integer();	
			}

			variable[var]=data_stack.top();
			data_stack.pop();

			_src=src;
			return;
		    }
				
		    if(!forlist.IsList())
			throw LangErr("Parser<Application>::EvalStatement(string&)","for: argument "+tostr(forlist).String()+" is not a list");

		    // Other loop.
		    data_stack.push(variable[var]);

		    const Data &L=forlist;
		    for(size_t i=0; i<L.Size(); i++)
		    {
			variable[var]=L[i];
			tmp=block.c_str();
			EvalBlock(tmp,ret);
		    }

		    variable[var]=data_stack.top();
		    data_stack.pop();
				
		    _src=src;
		    return;
		}
		else
		    src=tmp;
	    }
		
	    if(CheckFor("def ",src))
	    {
		string name;
			
		EatWhiteSpace(src);

		GetVariable(src,name);
		if(name=="")
		    throw LangErr("Parser<Application>::EvalStatement(string&)","def: missing function name");
		EatWhiteSpace(src);
			
		string code;
		ReadStatement(src,code);

#ifdef STACK_TRACE
		// This ugly part stores all functions in call stack to
		// avoid crash if the code under execution suddenly disappears
		// from the heap.
		static map<string,string> function_storage;
			
		stack<string> s=function_call_stack;
		while(s.size())
		{
		    s.pop();
			
		    if(name==s.top())
			function_storage[name]=user_function[name];
			
		    s.pop();
		}
#endif
		user_function[name]=CompressCode(code);

		_src=src;
		ret=name;
		return;
	    }
		
	    EvalExpression(src,ret);
	    EatWhiteSpace(src);

	    if(*src!=';')
		throw LangErr("Parser<Application>::EvalStatement(string&)","Missing ';'");

	    src++;
		
	    _src=src;
	    return;
	}

    template <class Application> void Parser<Application>::EvalBlock(const char*& _src,Data& ret)
	{
	    register const char* src=_src;
		
	    EatWhiteSpace(src);

	    if(*src==0)
	    {
		_src=src;
		ret=Null;
		return;
	    }

	    if(*src=='{')
	    {
		src++;
		while(1)
		{
		    EvalStatement(src,ret);
		    EatWhiteSpace(src);
				
		    if(*src==0)
			throw LangErr("Parser<Application>::EvalBlock(const char*&)","Missing '}'");
		    if(*src=='}')
			break;
		}
		src++;
	    }
	    else
	    {
		EvalStatement(src,ret);
	    }

	    _src=src;
	    return;
	}
	
    template <class Application> Data& Parser<Application>::Variable(const string& var)
	{
	    if(!IsVariable(var))
		throw Error::Range("Parser<Application>::Variable(const string&)","String '"+var+"' is not valid variable name");

	    if(database.find(var)!=database.end())
		return database[var];
				
	    return variable[var];
	}
	
    template <class Application> void Parser<Application>::SetVariable(const string& var,const Data& val)
	{
	    Variable(var)=val;
	}

    template <class Application> void Parser<Application>::UnsetVariable(const string& var)
	{
	    if(!IsVariable(var))
		throw Error::Range("Parser<Application>::SetVariable(const string&,const Data& )","String '"+var+"' is not valid variable name");

	    if(database.find(var)!=database.end())
		database.erase(var);
	    else
		variable.erase(var);
	}

    template <class Application> void Parser<Application>::SetFunction(const string& var,Data (Application::*fn)(const Data& ))
	{
	    if(!IsVariable(var))
		throw Error::Range("Parser<Application>::SetFunction(const string&,const Data& )","String '"+var+"' is not valid function name");

	    function[var]=fn;
	}

    template <class Application> void Parser<Application>::SetFunction(const string& var,const string& code)
	{
	    if(!IsVariable(var))
		throw Error::Range("Parser<Application>::SetFunction(const string&,const string&)","String '"+var+"' is not valid function name");

	    user_function[var]=CompressCode(code);
	}

    template <class Application> Data Parser<Application>::operator()(const string& expr)
	{
	    register const char* src=expr.c_str();
	    Data ret;
		
	    while(*src)
		EvalBlock(src,ret);
		
	    return ret;
	}

    /// forall(s,e_1,...,e_n) - Evaluate string $s$ once for each
    /// $e_1,...,e_n$ substituting value of each $e_i$ in the place of
    /// the first {\tt \#} character in $s$. Return the list of
    /// results. If $n$=1 and $e_1$ is a list, substitute each value
    /// of the list $e_1$.
    template <class Application> Data Parser<Application>::forall(const Data& arg)
	{
	    Data ret;

	    if(arg.IsList())
	    {
		const Data * L=&arg;
		if(L->Size() >= 1 && (*L)[0].IsString())
		{
		    if(L->Size()==1)
		    {
			ret.MakeList();
			return ret;
		    }
				
		    string subst=(*L)[0].String();
		    size_t first=1;
				
		    if((*L)[1].IsList() && L->Size()==2)
		    {
			L=&(*L)[1];
			first=0;
		    }

		    ret.MakeList(L->Size()-first);

		    for(size_t i=0; i<L->Size()-first; i++)
			ret[i]=(*this)(Substitute(subst,(*L)[i+first])+";");
					
		    return ret;
		}

		ArgumentError("forall",arg);
	    }

	    return ret;
	}

    /// select(s,L) - Substitute each member $x\in L$ into the string
    /// $s$ in place of {\tt \#} character(s). If expression evaluates
    /// to non-zero integer, add $x$ to the resulting list.
    template <class Application> Data Parser<Application>::select(const Data& arg)
	{
	    Data ret;

	    if(!arg.IsList(2) || !arg[0].IsString() || !arg[1].IsList())
		ArgumentError("select",arg);

	    const Data& L=arg[1];
	    const string exp=arg[0].String();
	    string s;

	    ret.MakeList();
	    for(size_t i=0; i<L.Size(); i++)
	    {
		s=Substitute(exp,L[i]);
		if((*this)(s+";").Integer())
		    ret.AddList(L[i]);
	    }
					
	    return ret;
	}

    /// call(f,e) - Call function $f$ given as a string with arguments
    /// $e$.
    template <class Application> Data Parser<Application>::call(const Data& arg)
	{
	    if(!arg.IsList(2) || !arg[0].IsString())
		ArgumentError("call",arg);

	    CHECK_STACK_OVERFLOW;
		
	    Data ret;
	    const Data& newarg=arg[1];
	    string name=arg[0].String();
		
#ifdef STACK_TRACE
	    function_call_stack.push(name);
	    function_call_stack.push(tostr(newarg).String());
#endif				
	    if(function.find(name)!=function.end())
		ret=(user->*(function[name]))(newarg);
	    else if(external_function.find(name) != external_function.end())
		ret=(*external_function[name])(newarg);
	    else if(internal_function.find(name) != internal_function.end())
		ret=(this->*internal_function[name])(newarg);
	    else if(user_function.find(name) != user_function.end())
	    {
		argument_stack.push(Data());
		argument_stack.push(newarg);
#ifdef PERFORMANCE_ANALYSIS
		string old;
		PerfCall(name,old);
#endif
		(*this)(user_function[name]);
#ifdef PERFORMANCE_ANALYSIS
		PerfReturn(old);
#endif
		argument_stack.pop();
		ret=argument_stack.top();
		argument_stack.pop();
	    }
	    else
		throw LangErr("call","undefined function "+name);

#ifdef STACK_TRACE
	    if(function_call_stack.size())
		function_call_stack.pop();
	    if(function_call_stack.size())
		function_call_stack.pop();
#endif

	    return ret;
	}

    /// apply(f,L) - Call a function $f$ once for each argument in the list $L$.
    /// Return list of return values of each call.
    template <class Application> Data Parser<Application>::apply(const Data& arg)
	{
	    if(!arg.IsList(2) || !arg[0].IsString() || !arg[1].IsList())
		ArgumentError("apply",arg);

	    Data ret;
	    ret.MakeList(arg[1].Size());
	    Data applyarg;
	    applyarg.MakeList(2);
	    applyarg[0]=arg[0];
		
	    for(size_t i=0; i<arg[1].Size(); i++)
	    {
		applyarg[1]=arg[1][i];
		ret[i]=call(applyarg);
	    }
		
	    return ret;
	}
	
    /// isfunction(s) - Return 1 if $s$ is user defined function, 2 if
    /// $s$ is library function, 3 if $s$ is internal parser function,
    /// 4 if $s$ is application defined function, 5 if $s$ is a
    /// special function and 0 if there is no such function.
    template <class Application> Data Parser<Application>::isfunction(const Data& arg)
	{
	    if(!arg.IsString())
		ArgumentError("isfunction",arg);

	    string name=arg.String();
		
	    if(user_function.find(name) != user_function.end())
		return 1;
	    else if(external_function.find(name) != external_function.end())
		return 2;
	    else if(internal_function.find(name) != internal_function.end())
		return 3;
	    else if(function.find(name)!=function.end())
		return 4;
	    else if(name=="eval")
		return 5;
	    else
		return 0;
	}

    /// isvar(s) - Return 1 if a string $s$ name of the defined varible.
    template <class Application> Data Parser<Application>::isvar(const Data& arg)
	{
	    if(!arg.IsString())
		ArgumentError("isvar",arg);

	    string name=arg.String();
		
	    if(variable.find(name) != variable.end())
		return 1;
	    else if(database.find(name) != database.end())
		return 1;
	    else
		return 0;
	}
	
    /// valueof(s) - Return value of the variable named $s$ or NULL if not defined.
    template <class Application> Data Parser<Application>::valueof(const Data& arg)
	{
	    if(!arg.IsString())
		ArgumentError("valueof",arg);

	    string name=arg.String();
		
	    if(variable.find(name) != variable.end())
		return variable[name];
	    else if(database.find(name) != database.end())
		return database[name];
	    else
		return Null;
	}
	
    /// repeat([n],s) - Evaluate a string $s$ $n$ times if $n >
    /// 0$. Return list of results. If $n$ is omitted, then evaluate
    /// string $s$ forever.
    template <class Application> Data Parser<Application>::repeat(const Data& arg)
	{
	    Data ret;

	    if(arg.IsString())
	    {
		string command=arg.String();
		while(1)
		{
		    (*this)(command+";");
		}
	    }
	    else if(arg.IsList(2) && arg[0].IsInteger() && arg[1].IsString())
	    {
		int times=arg[0].Integer();
		string command=arg[1].String();

		ret.MakeList();
		for(; times>0; times--)
		    ret.AddList((*this)(command+";"));
	    }
	    else
		ArgumentError("repeat",arg);
		
	    return ret;
	}

    /// vardump() - Print all variables and their values to the file named 'vardump'.
    template <class Application> Data Parser<Application>::vardump(const Data& arg)
	{
	    if(!arg.IsNull())
		ArgumentError("vardump",arg);

	    string file="./vardump";
	    security.WriteFile(file);
		
	    ofstream F(file.c_str());
	    if(!F)
		return Null;
		
	    map<string,Data >::const_iterator i;
	    for(i=variable.begin(); i!=variable.end(); i++)
		F << (*i).first << "=" << tostr((*i).second) << endl;
	    map<string,DataFileDB >::const_iterator j;
	    for(j=database.begin(); j!=database.end(); j++)
		F << (*j).first << "=<Database>" << endl;
		
	    return 1;
	}
	
    /// stacktrace() - Print function call stack with arguments.
    template <class Application> Data Parser<Application>::stacktrace(const Data& arg)
	{
	    if(!arg.IsNull())
		ArgumentError("stacktrace",arg);
#ifdef STACK_TRACE
	    stack<string> s=function_call_stack;
	    int i=0;
	    string a;
	    while(s.size())
	    {
		a=s.top();
		s.pop();
			
		cerr << "[ " << i << ". ] " << s.top() << "(" << a << ")" << endl;
			
		s.pop();
		i++;
	    }
#else
	    cout << "Stacktrace ability not compiled in parser." << endl;
#endif
	    return Null;
	}

    /// push(e_1,...,e_n) - Push values $e_1$,...,$e_n$ to the stack
    /// in that order. Return the current size of the stack.
    template <class Application> Data Parser<Application>::push(const Data& arg)
	{
	    data_stack.push(arg);

	    return int(data_stack.size());
	}

    /// pop() - Remove the topmost value of the stack and return it.
    template <class Application> Data Parser<Application>::pop(const Data& arg)
	{
	    if(!arg.IsNull())
		ArgumentError("pop",arg);

	    if(!data_stack.size())
		throw LangErr("Parser<Application>::pop(const Data&)","stack is empty");

	    Data ret=data_stack.top();
	    data_stack.pop();

	    return ret;
	}

    /// return(e) - Set the return value during execution of the user
    /// defined function. Note: this do not terminate the execution of
    /// the function.
    template <class Application> Data Parser<Application>::_return(const Data& arg)
	{		
	    if(argument_stack.size() < 2)
		throw LangErr("Parser<Application>::pop(const Data&)","stack underflow");

	    Data tmp=argument_stack.top();
	    argument_stack.pop();
	    argument_stack.pop();
	    argument_stack.push(arg);
	    argument_stack.push(tmp);

	    return Null;
	}

    /// save(s) - Save the variable with name $s$ to save
    /// directory or skip this if it is a database. If the variable $s$ is not declared, throw
    /// error. If the variable has two dots in it's name, throw fatal
    /// error. Return 1, if success, 0 otherwise.
    template <class Application> Data Parser<Application>::save(const Data& arg)
	{
	    if(!arg.IsString())
		ArgumentError("save",arg);

	    string s=arg.String();
	    if(!IsVariable(s))
		throw LangErr("save","invalid variable '"+s+"'");

	    if(database.find(s) != database.end())
	    {
		database[s].SaveToDisk();
		return 1;
	    }
		
	    if(variable.find(s) == variable.end())
		throw LangErr("save","variable "+s+" not defined");
		
	    string f=savedir+"/"+s;

	    security.WriteFile(f);
		
	    ofstream F(f.c_str());
	    if(!F)
		return 0;
#ifndef PRETTY_SAVE
	    F << tostr(variable[s]).String() << endl;
#else /* PRETTY_SAVE */
	    PrettySave(F,variable[s]);
#endif /* PRETTY_SAVE */
	    F.close();

	    return 1;
	}

    /// delsaved(s) - Remove a file where variable 's' is saved. Return
    /// 1 if file exist and is succesfully removed.
    template <class Application> Data Parser<Application>::delsaved(const Data& arg)
	{
	    if(!arg.IsString())
		ArgumentError("delsaved",arg);

	    string s=arg.String();
	    if(s=="")
		throw LangErr("load","empty variable name");
	    string f=savedir+"/"+s;
	    security.WriteFile(f);
	    security.WriteFile(savedir+"/");

	    return (int)(unlink(f.c_str())==0);
	}

    /// load(s) - Load the value of the variable with name $s$ from
    /// the save directory. Return 1, if success, 0 otherwise.
    template <class Application> Data Parser<Application>::load(const Data& arg)
	{
	    if(!arg.IsString())
		ArgumentError("load",arg);

	    string s=arg.String();
	    if(s=="")
		throw LangErr("load","empty variable name");
	    string f=savedir+"/"+s;

	    security.ReadFile(f);

	    ifstream F(f.c_str());
	    if(!F)
		return 0;
	    string buffer;
	    while(F)
		buffer+=readline(F);
	    variable[s]=toval(buffer);
	    F.close();

	    return 1;
	}

    /// execute(f) - Run script at a file $f$ and return 1, if the
    /// file exists. Throw fatal exception, if the security manager is enabled and
    /// the file is not declared as a safe in the program binary.
    template <class Application> Data Parser<Application>::execute(const Data& arg)
	{
	    if(!arg.IsString())
		ArgumentError("execute",arg);
		
	    string file;
	    file=CCG_DATADIR;
	    file+="/scripts/";
	    file+=arg.String();
		
	    if(!FileExist(file)) {
		 
		file=execdir;
		file+="/";
		file+=arg.String();

		if(!FileExist(file)) {
			file=CCG_DATADIR;
			file+="/scripts/global/";
			file+=arg.String();
		}

		if(execdir == "" || !FileExist(file)) {

		    file=getenv("HOME");
		    file+="/.gccg/";
		    file+=arg.String();
		}
	    }

	    if(!FileExist(file))
		return Null;

	    security.Execute(file);

	    ifstream F(file.c_str());
		
	    string line,code;
	    while(F)
	    {
		line=readline(F);
		if(line.length() && line[0]!='#')
		    code+=line;
	    }

	    (*this)(code);
	    return 1;
	}

    /// attach(v,t) - Attach a variable 'v' to the disk database. If the
    /// database does not exist yet, then current value of the
    /// variable 'v' is taken as initial content for the database having type t.
    /// Supportet types are now "DBSingleFile" and "DBStringKeys". After
    /// that, each change in the value of variable 'v' is stored to
    /// the disk automatically. If the database exist already, then
    /// old value of the variable 'v' is ignored and the database is
    /// used instead.
    template <class Application> Data Parser<Application>::attach(const Data& arg)
	{
	    if(!arg.IsList(2) || !arg[0].IsString() || !arg[1].IsString())
		ArgumentError("attach",arg);

	    string var=arg[0].String();
	    FileDBType type=DataFileDB::StringToType(arg[1].String());
		
	    Data init;

	    if(type==DBNone)
		throw Error::Range("attach","invalid type '"+arg[1].String()+"'.");
	    if(!IsVariable(var))
		throw Error::Range("attach","invalid variable name '"+var+"'.");
	    if(database.find(var)!=database.end())
		throw LangErr("attach","'"+var+"' already attached.");
				
	    if(variable.find(var)!=variable.end())
	    {
		init=variable[var];
		UnsetVariable(var);
	    }

	    database.insert(pair<string,DataFileDB>(var,DataFileDB()));
	    database[var].Attach(init,var,type);
		
	    return Null;
	}

    /// binary_load(s) - Load the value of the variable with name $s$ from
    /// the save directory. Return 1, if success, 0 otherwise.
    template <class Application> Data Parser<Application>::binary_load(const Data& arg)
	{
	    if(!arg.IsString())
		ArgumentError("binary_load",arg);

	    string s=arg.String();
	    if(s=="")
		throw LangErr("binary_load","empty variable name");
	    string f=savedir+"/"+s;

	    security.ReadFile(f);

		ifstream F(f.c_str(), ios::in | ios::binary);
		if(!F) return 0;

		char buf[4];
		F.read(buf, 3);

		// if not binary encoded, load as string
		if ( strncmp(buf, "CCG", 3) ) {
			F.close();
			return load(arg); 
		}

		// version, unused for now
		unsigned char ver;
		F.read((char*)&ver, 1);

		variable[s]=ReadBinary(F);
		F.close();
	    return 1;
	}

    /// binary_save(s) - Save the variable with name $s$ to save
    /// directory or skip this if it is a database. If the variable $s$ is not declared, throw
    /// error. If the variable has two dots in it's name, throw fatal
    /// error. Return 1, if success, 0 otherwise.
    template <class Application> Data Parser<Application>::binary_save(const Data& arg)
	{
	    if(!arg.IsString())
		ArgumentError("binary_save",arg);

	    string s=arg.String();
	    if(!IsVariable(s))
		throw LangErr("binary_save","invalid variable '"+s+"'");

	    if(database.find(s) != database.end())
	    {
			database[s].SaveToDisk();
			return 1;
	    }
		
	    if(variable.find(s) == variable.end())
		throw LangErr("binary_save","variable "+s+" not defined");
		
	    string f=savedir+"/"+s;
	    security.WriteFile(f);
		
		ofstream F(f.c_str(), ios::out | ios::binary);
		if ( !F ) return 0;

		// 4 bytes header, fourth byte not used
		unsigned char ver=0;
		F.write("CCG", 3);
		F.write((char*)&ver, 1);

		tobinary(variable[s], F);

		F.close();

	    return 1;
	}

    /// keys(D) - Return list of key elements of dictionary $D$ or
    /// throw error if $D$ is not a dictionary. This functions also
    /// returns list of first elements of list containing only pairs.
    /// If $D$ is given as a string, the parser variable with that name
    /// is accessed directly. This is much faster escpecially with
    /// file databases.
    template <class Application> Data Parser<Application>::keys(const Data& arg)
	{
	    Data ret=arg.Keys();
		
	    if(arg.IsList())
	    {
		ret=arg.Keys();
		
		if(ret.IsNull())
		    ArgumentError("keys",arg);
	    }
	    else if(arg.IsString())
	    {
		string var=arg.String();
			
		if(!IsVariable(var))
		    throw LangErr("keys","invalid variable "+var);

		if(variable.find(var)!=variable.end())
		    return variable[var].Keys();
		if(database.find(var)!=database.end())
		    return database[var].Keys();
			
		throw LangErr("keys","no such variable "+var);			
	    }
	    else
		ArgumentError("keys",arg);

	    return ret;
	}
	
    /// del_entry(e,L) - If $L$ is a list of pairs (i.e.dictionary),
    /// delete all elements whose first component is $e$. If $L$ is a string
    /// delete entry from directly from parser the variable and return 1 if successful.
    template <class Application> Data Parser<Application>::del_entry(const Data& arg)
	{
	    if(!arg.IsList(2))
		ArgumentError("del_entry",arg);

	    if(arg[1].IsList())
	    {
		Data ret=arg[1];

		ret.DelEntry(arg[0]);

		return ret;
	    }
	    else if(arg[1].IsString())
	    {
		Data& var=Variable(arg[1].String());

		if(!var.IsList())
		    throw LangErr("del_entry","not a dictionary");
			
		return (int)var.DelEntry(arg[0]);
	    }
	    else
		ArgumentError("del_entry",arg);

	    return Null;
	}

    /// cache_parameters(var,(p1,p2,p3,p4)) - If only a variable name is
    ///   given, return the list containing current cache parameters
    ///   for that database variable.
    ///   If a list of parameters is also given, set parameters and
    ///   return earlier settings. Parameters are p1 - cache refresh
    ///   rate in seconds, p2 - mininmum age before disk write in
    ///   seconds, p3 - maximum number of entries to write at one
    ///   update, p4 - maximum number of resident entries.
    ///   A negative value can be given in order to leave
    ///   a parameter unchanged.
    template <class Application> Data Parser<Application>::cache_parameters(const Data& arg)
	{
	    string var;
	    int p1=-1,p2=-1,p3=-1,p4=-1;
		
	    if(arg.IsString())
	    {
		var=arg.String();
	    }
	    else if(arg.IsList(2) && arg[0].IsString() && arg[1].IsList(4)
	      && arg[1][0].IsInteger() && arg[1][1].IsInteger() && arg[1][2].IsInteger() && arg[1][3].IsInteger())
	    {
		var=arg[0].String();
		p1=arg[1][0].Integer();
		p2=arg[1][1].Integer();
		p3=arg[1][2].Integer();
		p4=arg[1][3].Integer();
	    }
	    else
		ArgumentError("cache_parameters",arg);

	    if(database.find(var)==database.end())
		throw LangErr("cache_parameters","no such database as "+var);

	    Data ret;
	    ret.MakeList(4);
	    ret[0]=(int)database[var].CACHE_REFRESH_RATE;
	    ret[1]=(int)database[var].CACHE_MIN_AGE;
	    ret[2]=database[var].CACHE_MAX_SINGLE_WRITE;
	    ret[3]=database[var].CACHE_MAX_RESIDENT;

	    if(p1 >= 0)
		database[var].CACHE_REFRESH_RATE=(time_t)p1;
	    if(p2 >= 0)
		database[var].CACHE_MIN_AGE=(time_t)p2;
	    if(p3 >= 0)
		database[var].CACHE_MAX_SINGLE_WRITE=p3;
	    if(p4 > 0)
		database[var].CACHE_MAX_RESIDENT=p4;
		
	    return ret;
	}

    /// cache_size(var) - Return the number of entries currently
    ///    loaded for the database var.
    template <class Application> Data Parser<Application>::cache_size(const Data& arg)
	{
	    string var;
		
	    if(!arg.IsString())
		ArgumentError("cache_size",arg);

            var=arg.String();

	    if(database.find(var)==database.end())
		throw LangErr("cache_parameters","no such database as "+var);

	    return database[var].Loaded();
	}

    /// sort_fn(f,L) - Return the list L sorted using the function f as comparison function.
    /// Each list member is substituted in place of '#' in the string
    /// f and evaluated to produce coparison function value.
    template <class Application> Data Parser<Application>::sort_fn(const Data& arg)
	{
	    if(!arg.IsList(2) || !arg[0].IsString() || !arg[1].IsList())
		ArgumentError("sort_fn",arg);

	    const string& f=arg[0].String();
	    const Data& L=arg[1];
	    
	    int n=L.Size();
	    int* index = new int[n];
	    Data* fnval = new Data[n];
	    Data ret;
	    ret.MakeList(n);

	    for(int i=0; i<n; i++)
	    {
		index[i]=i;
		fnval[i]=(*this)(Substitute(f,L[i])+";");
	    }

	    int k;
	    for(int i=0; i<n-1; i++)
		for(int j=i+1; j<n; j++)
		    if(fnval[index[i]] > fnval[index[j]])
		    {
			k=index[i];
			index[i]=index[j];
			index[j]=k;
		    }

	    for(int i=0; i<n; i++)
		ret[i]=L[index[i]];

		delete [] index;
		delete [] fnval;
	    return ret;
	}
#ifdef USE_SQUIRREL
    
    // Start Squirrel.
    template <class Application> Data Parser<Application>::start_sq(const Data& arg)
    {
        if (squirrel_started)
            return Null;
        squirrel_vm = Libsquirrel::start_sq_impl(arg);
        sq_setforeignptr(squirrel_vm, user->make_sq_proxy_helper());
        squirrel_started = true;

        // Load the common GCCG-specific Squirrel functions.
        // Note: always call this after_ squirrel_started has been set
        //       to avoid recursion.        
        load_sq(Data(string("./scripts/common.nut")));

       return Null;
    }

    // Stop Squirrel. Not strictly an internal method, but kept here for
    // consistency.
    template <class Application> Data Parser<Application>::stop_sq(const Data& arg)
    {
        squirrel_started = false;
        Libsquirrel::SQProxyHelper* ph = static_cast<Libsquirrel::SQProxyHelper*>(sq_getforeignptr(squirrel_vm));
        delete ph;
        sq_setforeignptr(squirrel_vm, NULL);
        return Libsquirrel::stop_sq_impl(squirrel_vm, arg);
    }
    
    template <class Application> Data Parser<Application>::call_sq(const Data& arg)
    {
        start_sq(Null);
        return Libsquirrel::call_sq_impl(squirrel_vm, arg);
    }

    template <class Application> Data Parser<Application>::eval_sq(const Data& arg)
    {
        start_sq(Null);
        return Libsquirrel::eval_sq_impl(squirrel_vm, arg);
    }

    template <class Application> Data Parser<Application>::load_sq(const Data& arg)
    {
        start_sq(Null);
        return Libsquirrel::load_sq_impl(squirrel_vm, arg);
    }

    template <class Application> Data Parser<Application>::get_sq(const Data& arg)
    {
        start_sq(Null);
        return Libsquirrel::get_sq_impl(squirrel_vm, arg);
    }
#endif

    template <class Application> Parser<Application>::Parser(Application* base)
	{
	    user=base;
	    dump_indent=0;
#ifdef USE_SQUIRREL
		squirrel_started=false;
#endif
			
	    internal_function["apply"]=&Parser<Application>::apply;
	    internal_function["attach"]=&Parser<Application>::attach;
	    internal_function["binary_load"]=&Parser<Application>::binary_load;
	    internal_function["binary_save"]=&Parser<Application>::binary_save;
	    internal_function["cache_parameters"]=&Parser<Application>::cache_parameters;
	    internal_function["cache_size"]=&Parser<Application>::cache_size;
	    internal_function["call"]=&Parser<Application>::call;
	    internal_function["del_entry"]=&Parser<Application>::del_entry;
	    internal_function["delsaved"]=&Parser<Application>::delsaved;
	    internal_function["execute"]=&Parser<Application>::execute;
	    internal_function["forall"]=&Parser<Application>::forall; 
	    internal_function["isfunction"]=&Parser<Application>::isfunction;
	    internal_function["isvar"]=&Parser<Application>::isvar;
	    internal_function["keys"]=&Parser<Application>::keys;
	    internal_function["load"]=&Parser<Application>::load;
	    internal_function["pop"]=&Parser<Application>::pop; 
	    internal_function["push"]=&Parser<Application>::push; 
	    internal_function["repeat"]=&Parser<Application>::repeat; 
	    internal_function["return"]=&Parser<Application>::_return; 
	    internal_function["save"]=&Parser<Application>::save;
	    internal_function["select"]=&Parser<Application>::select;
	    internal_function["sort_fn"]=&Parser<Application>::sort_fn;
	    internal_function["stacktrace"]=&Parser<Application>::stacktrace;
	    internal_function["valueof"]=&Parser<Application>::valueof;
	    internal_function["vardump"]=&Parser<Application>::vardump;
#ifdef USE_SQUIRREL
	    
        internal_function["start_sq"]=&Parser<Application>::start_sq;
        internal_function["stop_sq"]=&Parser<Application>::stop_sq;
        internal_function["call_sq"]=&Parser<Application>::call_sq;
        internal_function["eval_sq"]=&Parser<Application>::eval_sq;
        internal_function["load_sq"]=&Parser<Application>::load_sq;
        internal_function["get_sq"]=&Parser<Application>::get_sq;
#endif

	    variable["VERSION"]=VERSION;
	    variable["SYSTEM"]=SYSTEM;
	    variable["HOME"]=getenv("HOME");
	    variable["SAVEDIR"]=CCG_SAVEDIR;
	    variable["DATADIR"]=CCG_DATADIR;
	}
}

#endif
