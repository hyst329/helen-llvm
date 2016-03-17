%option yylineno
%{
#include <boost/any.hpp>
#include <string>
#include "helmake.parser.hpp"

extern "C" int yywrap() {
    return 1;
}
void yyerror(const char* msg) {
    /* To be fixed*/
    printf("%s in line %d", msg, yylineno);
}
%}

%%
=                        return ASSIGN;
\+=                      return ASSIGNADD;
\n                       return NEWLINE;
[ \t]                    ;
[a-zA-Z_][a-zA-Z_0-9]*   yylval.vstr = strdup(yytext); return VAR;
[\+\-]?[0-9]+            yylval.vint = atoi(yytext); return NUM;
[0-9]+(.[0-9]+){0,3}     yylval.vstr = strdup(yytext); return VERSION;
\"([^\n\\]|\\[^\n])+\"   yylval.vstr = strdup(yytext + 1); yylval.vstr[yyleng - 2] = 0; return STRING;
%%