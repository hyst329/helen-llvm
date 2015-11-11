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
    Error::error(ErrorType::SyntaxError, {msg});
}
stack<YY_BUFFER_STATE> includeStack;
set<string> includedFiles;
stack<string> filenameStack;
string currentFilename;
extern void yy_switch_to_buffer(YY_BUFFER_STATE);
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
string                   return STRING;
array                    return ARRAY;
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
include                  BEGIN(INCLUDE);
\n                       return NEWLINE;
[ \t]                    ;
[a-zA-Z][a-zA-Z0-9]*     yylval.vstr = strdup(yytext); return ID;
"<-"                     return LARROW;
"->"                     return RARROW;
\(                       return LPAREN;
\)                       return RPAREN;
\;                       return SEMI;
\:                       return COLON;
\,                       return COMMA;
\.                       return POINT;
#                        BEGIN(COMMENT);
<COMMENT>[^\n]*          ;
<COMMENT>\n              BEGIN(INITIAL);
<INCLUDE>[ \t]*          ;
<INCLUDE>[^ \t\n]+ {
                         string path = yytext; // checkFilename(yytext);
                         if(includedFiles.count(path)) goto begin;
                         includedFiles.insert(path);
                         currentFilename = path;
                         filenameStack.push(path);
                         includeStack.push(YY_CURRENT_BUFFER);
                         yyin = fopen(path.c_str(), "r");
                         if(!yyin)
                             Error::error(ErrorType::FileNotFound, { path });
                         yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
                         begin:
                         BEGIN(INITIAL);        
}
[\+\-\*\/\<\>\=@]+       yylval.vstr = strdup(yytext); return OPERATOR;
[\+\-]?[0-9]+[\.\,][0-9]+       yylval.vreal = atof(yytext); return REALLIT;
[\+\-]?[0-9]+                   yylval.vint = atoi(yytext); return INTLIT;
'[^(')]'                 yylval.vchar = yytext[1]; return CHARLIT;
\"[^(\")]+\"             yylval.vstr = strdup(yytext + 1); yylval.vstr[strlen(yylval.vstr) - 1] = 0; return STRLIT;
<<EOF>> {
                         if (includeStack.empty())
                         {
                             yyterminate();
                         }
                         else
                         {
                             yy_delete_buffer( YY_CURRENT_BUFFER );
                             yy_switch_to_buffer(includeStack.top());
                             includeStack.pop();
                             currentFilename = filenameStack.top();
                             filenameStack.pop();
                         }
}
%%