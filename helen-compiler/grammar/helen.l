%option case-insensitive
%option yylineno
%{
extern "C" int yywrap() {
    return 1;
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
