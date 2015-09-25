%option case-insensitive
%option yylineno
%{
#include "helen.parser.hpp"
extern "C" int yywrap() {
    return 1;
}
void yyerror(const char* msg) {
    printf("%s in line %d", msg, yylineno);
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
int                      return INT;
real                     return REAL;
logical                  return LOGICAL;
char                     return CHAR;
string                   return STRING;
in                       return IN;
out                      return OUT;
debugvar                 return DEBUGVAR;
\n                       return NEWLINE;
[ \t]                    ;
[a-zA-Z][a-zA-Z0-9]*     yylval.vstr = strdup(yytext); return ID;
"<-"                     return LARROW;
"->"                     return RARROW;
\(                       return LPAREN;
\)                       return RPAREN;
\;                       return SEMI;
\,                       return COMMA;
\.                       return POINT;
[\+\-\*\/\<\>\=]         yylval.vstr = strdup(yytext); return OPERATOR;
[0-9]+[\.\,][0-9]+       yylval.vreal = atof(yytext); return REALLIT;
[0-9]+                   yylval.vint = atoi(yytext); return INTLIT;
'[^(')]'                 yylval.vchar = yytext[0]; return CHARLIT;
\"[^(\")]+\"             yylval.vstr = strdup(yytext); return STRLIT;
<<EOF>>                  static int once = 0; return once++ ? 0 : NEWLINE;
%%