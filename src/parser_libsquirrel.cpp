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

#include <ctype.h>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#if !defined(__BCPLUSPLUS__) && !defined(_MSC_VER)
# include <sys/time.h>
# include <unistd.h>
#endif
#include <math.h>
#include "parser.h"
#include "parser_functions.h"
#include "localization.h"
#ifdef WIN32
#include <windows.h>
#undef max
#undef min
#endif

// Use doubles for floats in squirrel.
// TODO: Perhaps it's better to be sure Squirrel was compiled this way.
// Defining this changes the API, but I'm pretty sure it doesn't change the ABI
// so those two will disagree on the size of an SQFloat.
// #define SQUSEDOUBLE 1
#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
//#include "error.h"
#include <stdarg.h> // For the print function for Squirrel
#include <sstream>

using namespace std;

namespace Evaluator
{
    namespace Libsquirrel
    {

        HSQUIRRELVM squirrel_vm;
        

        bool squirrel_started = false;
        
        // Print function, can't handle more than 1024 characters.
        // This does properly use the cout stream however.
        static void gccg_sq_print_func(HSQUIRRELVM sq, const SQChar *format, ...)
        {
            char str[1024];
            va_list ap;
            va_start(ap, format);
            vsnprintf(str, 1024, format, ap);
            va_end(ap);
            cout << str << endl;
        }

        static void gccg_sq_compiler_error_handler(
            HSQUIRRELVM sq, const SQChar *err, const SQChar *filename,
            SQInteger line, SQInteger column)
        {
            stringstream ss;
            ss << "Error while compiling Squirrel (file=\"" << filename << "\" line=" 
               << line << " column=" << column << "):" << err;
            throw LangErr("<sq>", ss.str());
        }

        // Error handler for Squirrel runtime exceptions.
        static SQInteger gccg_sq_error_handler(HSQUIRRELVM sq)
        {
            const char *str = NULL;
            SQInteger nargs = sq_gettop(sq);
        
            if (nargs != 2) 
            {
                throw LangErr("<sq>", "Unexpected number of arguments in error handler: " + nargs);
            }
            
            sq_tostring(sq, 2);
            sq_getstring(sq, 2, &str);
            sq_pop(sq, 2); // Remove stringified object

            cout << "Error in Squirrel function: " << str << endl << endl;
            
            sqstd_printcallstack(sq);

            throw LangErr("<sq>", "Error in Squirrel function: " + string(str));
        }



        
        // Convert an Squirrel type code to a human readable name.
        //
        // Most of this function is copied from the Squirrel manual and adapted.
        static string sq_type_to_string(SQObjectType sq_type)
        {
            switch (sq_type)
            {
            case OT_NULL:
                return "null";
            case OT_INTEGER:
                return "integer";
            case OT_FLOAT:
                return "float";
            case OT_STRING:
                return "string";
            case OT_TABLE:
                return "table";
            case OT_ARRAY:
                return "array";
            case OT_USERDATA:
                return "userdata";
            case OT_CLOSURE:        
                return "closure(function)";    
            case OT_NATIVECLOSURE:
                return "native closure(C function)";
            case OT_GENERATOR:
                return "generator";
            case OT_USERPOINTER:
                return "userpointer";
            case OT_CLASS:
                return "class";
            case OT_INSTANCE:
                return "instance";
            case OT_WEAKREF:
                return "weak reference";
            default:
                return "unknown type (probably an error)";
            }
        }


        // Convert the given data to Squirrel and push it onto the stack.
        static void data_to_sq(HSQUIRRELVM sq, const Data& arg)
        {
            if (arg.IsNull())
            {
                sq_pushnull(sq);
            }
            else if (arg.IsInteger())
            {
                sq_pushinteger(sq, arg.Integer());
            }
            else if (arg.IsReal())
            {
                // Depending on how Squirrel was compiled an SQFloat could be
                // a double (like GCCG's Data-real-values) or a float.
                sq_pushfloat(sq, static_cast<SQFloat>(arg.Real()));
            }
            else if (arg.IsString())
            {
                sq_pushstring(sq, arg.String().data(), arg.String().size());
            }
            else if (arg.IsList())
            {
                sq_newarray(sq, arg.Size());
                for (int i = 0; i < (int) arg.Size(); i++)
                {
                    data_to_sq(sq, arg[i]);
                    sq_arrayappend(sq, -2);
                }
            }
        }

        // Convert the indexed argument on the squirrel stack to data.
        static Data sq_to_data(HSQUIRRELVM sq, int idx)
        {
            if (idx < 0)
            {
                // Negative indexing tends to blow up when things are pushed,
                // better convert it to positive indexing.
              
                // Due to Lua-compatible 1 based indexing with the stack 
                // sq_gettop returns the highest (last pushed) index on the stack.
                // The -1 index is the highest index, using negative notation.
                // Hence we need to make the two equal by adding one to the
                // negative index.
                idx = sq_gettop(sq) + (idx + 1);
            }

            switch (sq_gettype(sq, idx))
            {
            case OT_NULL:
                return Null;
            case OT_BOOL:
                SQBool b;	
                sq_getbool(sq, idx, &b);
                return Data(static_cast<int>(b));
            case OT_INTEGER:
                int i;
                sq_getinteger(sq, idx, &i);
                return Data(static_cast<int>(i));
            case OT_FLOAT:
                // We consider a double with nothing behind the dot to be an integer
                SQFloat f;
                sq_getfloat(sq, idx, &f);
                return Data(static_cast<double>(f));
            case OT_STRING: 
                {
                    const char *str;
                    sq_getstring(sq, idx, &str);
                    return Data(str);
                }
            case OT_ARRAY:
                {
                    vector<Data> lst = vector<Data>();
                   
                    sq_pushnull(sq); // Key null means start at start of array.
                    
                    while(sq_next(sq, idx) != 0)
                    {
                        lst.push_back(sq_to_data(sq, -2)); // -2 == key, -1 == value
                        sq_poptop(sq); // remove the value, leave the key for sq_next
                    }

                    return Data(lst);
                }
            case OT_TABLE:
                {
                    vector<Data> lst = vector<Data>();
                   
                    sq_pushnull(sq); // Key null means start at start of array.
                    
                    while(sq_next(sq, idx) != 0)
                    {
                        vector<Data> pair = vector<Data>();
                        pair.push_back(sq_to_data(sq, -1)); // key 
                        pair.push_back(sq_to_data(sq, -2)); // value 
                        lst.push_back(Data(pair));
                        sq_poptop(sq); // remove the value, leave the key for sq_next
                    }

                    return Data(lst);
                }
            default:

                throw LangErr("<sq>", "Unconvertable type from Squirrel: " + sq_gettype(sq, idx));  

                throw LangErr("<sq>", "Unconvertable type from Squirrel: " + 
                                        sq_type_to_string(sq_gettype(sq, idx)));  

            }
        }


        // This function was copied and adapted from the Squirrel docs
        static SQInteger register_global_sq_func(HSQUIRRELVM sq, SQFUNCTION f, const char *fname)
        {
            sq_pushroottable(sq);
            sq_pushstring(sq, fname, -1); // -1 = calculate length using strlen
            sq_newclosure(sq, f, 0); //create a new function
            sq_createslot(sq, -3); 
            sq_pop(sq, 1); //pops the root table
            return SQ_OK;
        }

        
        // Call gccg from Squirrel.
        // First argument is the name of the GCCG function,
        // second is the argument.
        //
        // As GCCG sometimes uses a list as an argument in Perl style
        // (multiple arguments to function wrapped in a list) and sometimes

        // a single value argument a heuristic is used. If only one argument
        // is given it's passed directly, if multiple are given they're first
        // put in a list, if no argument is given Null is passed.

        // a single value argument heuristics are applied:
        //   * No argument is translated to Null.
        //   * One argument is passed through directly.
        //   * Multiple arguments are first put in a list.

        static SQInteger call_gccg(HSQUIRRELVM sq)
        {
            const char *name;
            Data arg;

            int top_before = sq_gettop(sq); // Not used for cutting down stack this time

            int stack_top = sq_gettop(sq);

            
            // Stack:
            // [1] = root table
            // [2] = function name
            // ... = arguments


            if (sq_gettype(sq, 2) != OT_STRING)
                return sq_throwerror(sq, "First argument to call_gccg must be a string.");

            try

            try 

            {
                if (sq_gettype(sq, 2) != OT_STRING)
                    return sq_throwerror(sq, "First argument to call_gccg must be a string.");


                sq_getstring(sq, 2, &name);

                if (top_before > 3)


                if (stack_top < 3) 

                {

                    // Multiple arguments
                    vector<Data> lst;
                    for (int i = 3; i <= top_before; i++)
                        lst.push_back(sq_to_data(sq, i));
                    arg = Data(lst);

                    arg = Null;

                }

                else if (top_before == 3)

                else if (stack_top == 3)

                {
                    arg = sq_to_data(sq, 3);

                    

                }
                else
                {

                    arg = Data(); // Null

                    vector<Data> lst;
                    for (int i = 3; i <= stack_top; i++)
                    {
                        lst.push_back(sq_to_data(sq, i));
                    }
                    arg = Data(lst);

                }

                // Needed for the calling convention of call().
                vector<Data> pair;
                pair.push_back(Data(string(name)));
                pair.push_back(arg);
                Data dpair = Data(pair);



                if(external_function.find(name) != external_function.end())
                {
                    Data ret = (*external_function[name])(arg);
                    data_to_sq(sq, ret);
                    return 1;
                }
                else
                {
                    return sq_throwerror(sq, ("Undefined GCCG function: " + string(name)).c_str());
                }

                Libsquirrel::SQProxyHelper* ph = static_cast<Libsquirrel::SQProxyHelper*>(sq_getforeignptr(sq));

                data_to_sq(sq, ph->call(dpair));
                return 1; // Value pushed

            }
            // Make sure Quit is reraised. This is pretty safe as Quit is intended
            // to be fatal, so the fact that we're messing up the Squirrel stack
            // doesn't matter anymore.
            catch (Error::Quit &e)
            { 
                throw e;
            }
            // For all non-fatal errors however use the Squirrel mechanism of
            // error throwing so it properly unwinds it's stack.
            catch (Error::General &e)
            { 
                return sq_throwerror(sq, e.Message().c_str());
            }
        }

        // This function was copied and adapted from the Squirrel docs
        static SQInteger register_global_sq_func(HSQUIRRELVM sq, SQFUNCTION f, const char *fname)
        {
            sq_pushroottable(sq);
            sq_pushstring(sq, fname, -1); // -1 = calculate length using strlen
            sq_newclosure(sq, f, 0); //create a new function
            sq_createslot(sq, -3); 
            sq_pop(sq, 1); //pops the root table
            return SQ_OK;
        }
        

        // Start Squirrel. This is automatically called by any of the Squirrel invoking functions.

        Data start_sq(const Data& args)

        HSQUIRRELVM start_sq_impl(const Data& args)

        {

            squirrel_vm = sq_open(1024); // This seems to be the standard stack space. 
            if (!squirrel_vm)

            HSQUIRRELVM sq = sq_open(1024); // This seems to be the standard stack space. 
            if (!sq)

            {
                throw LangErr("Couldn't start Squirrel: out of memory.");
            }


            sq_setprintfunc(squirrel_vm, gccg_sq_print_func);
            sq_setcompilererrorhandler(squirrel_vm, gccg_sq_compiler_error_handler);
            sq_newclosure(squirrel_vm, gccg_sq_error_handler, 0);
            sq_seterrorhandler(squirrel_vm);

            sq_setprintfunc(sq, gccg_sq_print_func);
            sq_setcompilererrorhandler(sq, gccg_sq_compiler_error_handler);
            sq_newclosure(sq, gccg_sq_error_handler, 0);
            sq_seterrorhandler(sq);

            

            sqstd_register_mathlib(squirrel_vm);
            sqstd_register_stringlib(squirrel_vm);

            register_global_sq_func(squirrel_vm, call_gccg, "call_gccg");

            sqstd_register_mathlib(sq);
            sqstd_register_stringlib(sq);
            register_global_sq_func(sq, call_gccg, "call_gccg");



            squirrel_started = true;

            return Null;

            return sq;

        }

        // Stop Squirrel. This will rarely have to be called.

        Data stop_sq(const Data& args)

        Data stop_sq_impl(HSQUIRRELVM sq, const Data& args)

        {

            if (squirrel_vm)

            if (sq)

            {

                sq_close(squirrel_vm);
                squirrel_started = false;

                sq_close(sq);

            }

            return Null;
        }

        // Call Squirrel. Args must be of the form
        // (squirrel_func_name, list_of_args).

        Data call_sq(const Data& args)

        Data call_sq_impl(HSQUIRRELVM sq, const Data& args)

        {
            size_t list_size;
            int func_pos;
            int top_before;


            // Automatically start Squirrel if it hasn't been started yet
            if (!squirrel_started)
                start_sq(Null);


            if(!args.IsList(2) || (!args[0].IsString()) || (!args[1].IsList()))
            {
                ArgumentError("call_sq",args);
                return Null;
            }

            list_size = args[1].Size();
            

            top_before = sq_gettop(squirrel_vm);

            top_before = sq_gettop(sq);


            // Squirrel calling convention: push the function first and then the arguments.
            // The first argument must be the environment object (root table for globals).
        

            sq_pushroottable(squirrel_vm);
            sq_pushstring(squirrel_vm, args[0].String().c_str(), -1);
            if (SQ_FAILED(sq_get(squirrel_vm, -2))) // Lookup the function in the root table

            sq_pushroottable(sq);
            sq_pushstring(sq, args[0].String().c_str(), -1);
            if (SQ_FAILED(sq_get(sq, -2))) // Lookup the function in the root table

                throw LangErr("<sq>", "Couldn't find Squirrel function \"" + args[0].String() + "\"");

            sq_pushroottable(squirrel_vm); 		// Environment, the root table.

            sq_pushroottable(sq); 		// Environment, the root table.

        

            func_pos = sq_gettop(squirrel_vm);

            func_pos = sq_gettop(sq);

            for (size_t i = 0; i < list_size; i++)
            {

                data_to_sq(squirrel_vm, args[1][i]);

                data_to_sq(sq, args[1][i]);

            }

            // Every Squirrel function has as it's first argument it's environment,
            // so it gets nargs+1 arguments.
            // A failure here will result in the error handler being invoked which will
            // raise a GCCG exception.

            sq_call(squirrel_vm, list_size+1, SQTrue, SQTrue);
            Data rv = sq_to_data(squirrel_vm, -1);

            sq_call(sq, list_size+1, SQTrue, SQTrue);
            Data rv = sq_to_data(sq, -1);

        

            sq_settop(squirrel_vm, top_before); // Cut the stack down to what it was before.

            sq_settop(sq, top_before); // Cut the stack down to what it was before.

        
            return rv;
        }
        

        static Data eval_sq_real(const string& codestr, const string& filename)

        static Data eval_sq_real(HSQUIRRELVM sq, const string& codestr, const string& filename)

        {
            int top_before;
            

            // Automatically start Squirrel if it hasn't been started yet
            if (!squirrel_started)
                start_sq(Null);

            top_before = sq_gettop(squirrel_vm);

            top_before = sq_gettop(sq);


            // On error the compile error handler should fire and raise an exception.
            //
            // Note though that I've seen Squirrel use longjmp in it's compiler error code,
            // hopefully that doesn't interfere with C++'s exceptions.

            sq_compilebuffer(squirrel_vm, codestr.c_str(), codestr.size(), filename.c_str(), SQTrue);

            sq_compilebuffer(sq, codestr.c_str(), codestr.size(), filename.c_str(), SQTrue);

            

            sq_pushroottable(squirrel_vm); // For the call to sq_call.

            sq_pushroottable(sq); // For the call to sq_call.


            // Every Squirrel function has as it's first argument it's environment,
            // so it gets nargs+1 arguments.

            sq_call(squirrel_vm, 1, SQTrue, SQTrue);

            sq_call(sq, 1, SQTrue, SQTrue);



            Data rv = sq_to_data(squirrel_vm, -1);
            sq_settop(squirrel_vm, top_before); // Cut the stack down to what it was before.

            Data rv = sq_to_data(sq, -1);
            sq_settop(sq, top_before); // Cut the stack down to what it was before.

            return rv;
        }
        

        // Evaluate the given string in Squirrel.
        //
        // Returns the result of evaluating the string. 

        Data eval_sq(const Data& arg)

        Data eval_sq_impl(HSQUIRRELVM sq, const Data& arg)

        {
            if (!arg.IsString())
            {
                ArgumentError("eval_sq", arg);
                return Null;
            }

            return eval_sq_real(arg.String(), "<eval>");

            return eval_sq_real(sq, arg.String(), "<eval>");

        }
        
        
        // Load the file with the given name in Squirrel.
        //
        // Returns the result of loading the file.
        //
        // Most of this code is adapted from xml_parser.cpp.

        Data load_sq(const Data& arg)

        Data load_sq_impl(HSQUIRRELVM sq, const Data& arg)

        {
            int top_before;
            string input;
            FILE *file;
            char buf[1024];
            size_t read_size;
            
            if (!arg.IsString())
            {
                ArgumentError("eval_sq", arg);
                return Null;
            }
            
            string filename = arg.String();
            security.Execute(filename); // Check if we may read the file
                    

            // Automatically start Squirrel if it hasn't been started yet
            if (!squirrel_started)
                start_sq(Null);

            top_before = sq_gettop(squirrel_vm);

            top_before = sq_gettop(sq);


            file = fopen(filename.c_str(), "r");
            if (!file)
                throw Error::IO("load_sq", "Couldn't open \"" + filename + "\"");
            
            while ((read_size = fread(buf, 1, 1024, file)))
            {
                input.append(string(buf, read_size));
            }

            if (ferror(file))
            {
                throw Error::IO("load_sq", "Couldn't (fully) read \"" + filename + "\"");
            }
            fclose(file);


            Data rv = eval_sq_real(input, filename);

            Data rv = eval_sq_real(sq, input, filename);

            
            return rv;
        }

        // Get the value of the given Squirrel variable. 

        Data get_sq(const Data& arg)

        Data get_sq_impl(HSQUIRRELVM sq, const Data& arg)

        {

            int top_before = sq_gettop(squirrel_vm);

            int top_before = sq_gettop(sq);

            if (!arg.IsString())
            {
                ArgumentError("get_sq", arg);
                return Null;
            }

            sq_pushroottable(squirrel_vm);
            sq_pushstring(squirrel_vm, arg.String().c_str(), -1);
            if (SQ_FAILED(sq_get(squirrel_vm, -2))) // Lookup the value in the root table

            sq_pushroottable(sq);
            sq_pushstring(sq, arg.String().c_str(), -1);
            if (SQ_FAILED(sq_get(sq, -2))) // Lookup the value in the root table

                throw LangErr("<sq>", "Couldn't find Squirrel value \"" + arg.String() + "\"");

            Data rv = sq_to_data(squirrel_vm, -1);
            sq_settop(squirrel_vm, top_before);

            Data rv = sq_to_data(sq, -1);
            sq_settop(sq, top_before);

            return rv;
        }
    }


    void InitializeLibsquirrel()
    {
        external_function["start_sq"]=&Libsquirrel::start_sq;
        external_function["stop_sq"]=&Libsquirrel::stop_sq;
        external_function["call_sq"]=&Libsquirrel::call_sq;
        external_function["eval_sq"]=&Libsquirrel::eval_sq;
        external_function["load_sq"]=&Libsquirrel::load_sq;
        external_function["get_sq"]=&Libsquirrel::get_sq;
    }

}

