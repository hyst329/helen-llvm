%{
#include <boost/any.hpp>
#include <map>  
#include <string> 
#include <vector> 
#include "helmake.parser.hpp"

extern int yylex();
void yyerror(const char*);

using namespace std;
extern map<string, vector<boost::any> > variableMap;
static vector<boost::any> v;
enum OperatorType {
    OT_ASSIGN,
    OT_ASSIGNADD
};
%}
%union
{
    char* vstr;
    int vint;
    boost::any *vba;
}
%token ASSIGN ASSIGNADD
%token VAR NUM STRING VERSION
%token NEWLINE
%type<vint> operator
%type<vstr> VAR STRING VERSION
%type<vint> NUM
%type<vba> elem
%start makefile
%%
makefile: makefile NEWLINE instruction {
    
}
| instruction {
    
}
instruction: VAR operator expr {
    switch($2) {
        //TODO: Replace v with $3?
        case OT_ASSIGN:
            variableMap[$1] = v;
            break;
        case OT_ASSIGNADD:
            variableMap[$1].insert(variableMap[$1].end(), v.begin(), v.end());
            break;
    }
}
operator: ASSIGN {
    $$ = OT_ASSIGN;
}
| ASSIGNADD {
    $$ = OT_ASSIGNADD;
}
expr: expr elem {
    v.push_back(*$2);
}
| elem {
    v.clear(); // no memory leakage, elements are destroyed properly
    v.push_back(*$1);
}
elem: STRING {
    $$ = new boost::any(string($1));
}
| NUM {
    $$ = new boost::any($1);
}
| VERSION {
    $$ = new boost::any(string($1));
}
%%