#include "parser.h"

using namespace std;
using namespace Evaluator;

int main(int argc,char** argv)
{
    try
    {
	Data stats;

	// Scan through all argument filenames.
	for(int i=1; i<argc; i++)
	{
	    // Read in the file.
	    stats=ReadValue(argv[i]);
	    
	    // Find the real name.
	    string realname="nobody";

	    if(stats[3][0][Data("realname")].IsString())
		realname=stats[3][0][Data("realname")].String();

	    cout << "Realname: " << realname << endl;
	    
	    // Find the keys of the match result dictionary.
	    Data matches=stats[3][3].Keys();
	    cout << "Game type keys: " << tostr(matches) << endl;

	    // Loop over all game formats.
 	    for(size_t j=0; j<matches.Size(); j++)
	    {
		Data players=matches[j];
		cout << "For " << players << " players: " << tostr(stats[3][3][players]) << endl;
		
		// Loop over result types.
		Data results=stats[3][3][players].Keys();
		for(size_t k=0; k<results.Size(); k++)
		{
		    // And here we have everything now in normal C++ types.
		    string result_type=results[k].String();
		    int count=stats[3][3][players][results[k]].Integer();
		    int num_players=players.Integer();

		    cout << "  Result found: " << result_type << " " << count << " times with " << num_players << " player(s)" << endl;
		}
	    }
	}
    }
    catch(Error::General e)
    {
	cerr << e.Message() << endl;
	return 1;
    }
}
