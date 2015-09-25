%{
#include "helen.parser.hpp"

extern int yylex();
void yyerror(const char*);
%}
%token IF ELSE ENDIF
%token LOOP ENDLOOP
%token FUN ENDFUN DECLARE
%token SIZE RESIZE
%token RETURN
%token IN OUT DEBUGVAR
%token USE
%token INT REAL LOGICAL CHAR STRING
%token INTLIT REALLIT CHARLIT STRLIT
%token ID OPERATOR
%token NEWLINE
%token LPAREN RPAREN
%token SEMI COMMA POINT
%start program

%union
{
    char* vstr;
    double vreal;
    int vint;
    char vchar;
}
%%
program : instseq {

}
instseq: instseq instruction {
}
| instruction {
}
instruction : statement NEWLINE {

}
| NEWLINE {
    $$ = nullptr;
}
| IF expression NEWLINE instseq ENDIF {

}
| IF expression NEWLINE instseq ELSE NEWLINE instseq ENDIF {

}
| LOOP statement COMMA expression COMMA statement NEWLINE instseq ENDLOOP {

}
| FUN funprot NEWLINE instseq ENDFUN {

}
| DECLARE funprot {

}
| RETURN expression {

}
| RESIZE IDENT LPAREN expression RPAREN {

}
%%