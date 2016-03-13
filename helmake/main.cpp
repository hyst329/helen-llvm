#include <boost/any.hpp>
#include <map>
#include <string>
#include <vector>
#include <process.h>

using namespace std;

map<string, vector<boost::any> > variableMap;

int yyparse();
extern FILE* yyin;

int main()
{
    yyin = fopen("helmkf", "r");
    if(!yyin) {
        fprintf(stderr, "*** No helmkf file in the current dir. Stop. ***\n");
        return 1;
    }
    // Setting default values
    variableMap["helen_compiler"] = vector<boost::any>(1, boost::any(string("helen_compiler")));
    yyparse();
    return 0;
}