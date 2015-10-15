%option case-insensitive
%option yylineno
%{
#include "../AST.h"
#include "../Error.h"
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
%}
%%
if                       return IF;
else                     return ELSE;
endif                    return ENDIF;
fun                      return FUN;
endfun                   return ENDFUN;
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
in                       return IN;
out                      return OUT;
debugvar                 return DEBUGVAR;
style                    return STYLE;
ptr                      return PTR;
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
[\+\-\*\/\<\>\=@]+       yylval.vstr = strdup(yytext); return OPERATOR;
[\+\-]?[0-9]+[\.\,][0-9]+       yylval.vreal = atof(yytext); return REALLIT;
[\+\-]?[0-9]+                   yylval.vint = atoi(yytext); return INTLIT;
'[^(')]'                 yylval.vchar = yytext[1]; return CHARLIT;
\"[^(\")]+\"             yylval.vstr = strdup(yytext + 1); yylval.vstr[strlen(yylval.vstr) - 1] = 0; return STRLIT;
<<EOF>>                  static int once = 0; return once++ ? 0 : NEWLINE;
%%