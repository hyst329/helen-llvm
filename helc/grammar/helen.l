%option case-insensitive
%option yylineno
%x COMMENT
%x INCLUDE
%{
#include "../AST.h"
#include "../Error.h"
#include <stack>
#include <set>
#include "helen.parser.hpp"

using namespace Helen;

extern "C" int yywrap() {
    return 1;
}
void yyerror(Helen::AST* ast, const char* msg) {
    /* To be fixed*/
    printf("%s in line %d", msg, yylineno);
    Helen::Error::error(ErrorType::SyntaxError, {msg + string("\n")});
}
stack<YY_BUFFER_STATE> includeStack;
set<string> includedFiles;
stack<string> filenameStack;
string currentFilename;
extern vector<string> includePaths;
extern void yy_switch_to_buffer(YY_BUFFER_STATE);
extern bool checkPath(string, string&);
%}
%%
if                       return IF;
else                     return ELSE;
endif                    return ENDIF;
fun                      return FUN;
endfun                   return ENDFUN;
operator                 return OPERATORKW;
declare                  return DECLARE;
loop                     return LOOP;
endloop                  return ENDLOOP;
return                   return RETURN;
use                      return USE;
resize                   return RESIZE;
size                     return SIZE;
type                     return TYPE;
endtype                  return ENDTYPE;
int                      return INT;
real                     return REAL;
logical                  return LOGICAL;
char                     return CHAR;
array                    return ARRAY;
interface                return INTERFACE;
generic                  return GENERIC;
alias                    return ALIAS;
in                       return IN;
out                      return OUT;
method                   return METHOD;
constructor              return CONSTRUCTOR;
destructor               return DESTRUCTOR;
style                    return STYLE;
new                      return NEW;
delete                   return DELETE;
ptr                      return PTR;
cast                     return CAST;
to                       return TO;
mainmodule               return MAINMODULE;
shiftby                  return SHIFTBY;
include                  BEGIN(INCLUDE);
\n                       return NEWLINE;
[ \t]                    ;
[a-zA-Z][a-zA-Z0-9]*     yylval.vstr = strdup(yytext); return ID;
"<-"                     return LARROW;
"->"                     return RARROW;
\.\.\.                   return ELLIPSIS;
\(                       return LPAREN;
\)                       return RPAREN;
\[                       return LBRK;
\]                       return RBRK;
\{                       return LBRACE;
\}                       return RBRACE;
\;                       return SEMI;
\:                       return COLON;
\,                       return COMMA;
\.                       return POINT;
#                        BEGIN(COMMENT);
<COMMENT>[^\n]*          ;
<COMMENT>\n              BEGIN(INITIAL);
<INCLUDE>[ \t]*          ;
<INCLUDE>[^ \t\n]+ {
                         string path = yytext;
                         string abspath, filecurrent;
                         if(includedFiles.count(path)) goto begin;
                         includedFiles.insert(path);
                         currentFilename = path;
                         filenameStack.push(path);
                         includeStack.push(YY_CURRENT_BUFFER);
                         if(!checkPath(path, abspath))
                             Helen::Error::error(ErrorType::FileNotFound, { path });
                         filecurrent = abspath;
                         while(!filecurrent.empty())
                         {
                             if (filecurrent.back() != '/' && filecurrent.back() != '\\')
                                filecurrent.pop_back();
                             else break;
                         }
                         if(filecurrent == "")
                             filecurrent = ".";
                         else filecurrent.pop_back();
                         yyin = fopen(abspath.c_str(), "r");
                         if(!yyin)
                             Helen::Error::error(ErrorType::FileNotFound, { path });
                         includePaths.insert(includePaths.begin(), filecurrent);
                         yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
                         begin:
                         BEGIN(INITIAL);
}
[\+\-\*\/\<\>\=@\&\|\~\!]+       yylval.vstr = strdup(yytext); return OPERATOR;
[\+\-]?[0-9]+[\.\,][0-9]+       yylval.vreal = atof(yytext); return REALLIT;
[\+\-]?[0-9]+                   yylval.vint = atoi(yytext); return INTLIT;
'[^(')]'                 yylval.vchar = yytext[1]; return CHARLIT;
\"[^(\")]+\"             yylval.vstr = strdup(yytext + 1); yylval.vstr[strlen(yylval.vstr) - 1] = 0; return STRLIT;
<<EOF>> {
                         if (includeStack.empty())
                         {
                             yyterminate();
                             return NEWLINE;
                         }
                         else
                         {
                             yy_delete_buffer( YY_CURRENT_BUFFER );
                             yy_switch_to_buffer(includeStack.top());
                             includeStack.pop();
                             currentFilename = filenameStack.top();
                             filenameStack.pop();
                             includePaths.erase(includePaths.begin());
                         }
                         return NEWLINE;
}
%%
