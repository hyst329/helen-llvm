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
%token LARROW RARROW
%token SEMI COMMA POINT

%right OPERATOR
%start program

%union
{
    char* vstr;
    double vreal;
    int vint;
    char vchar;
}
%%
program: instseq {

}
instseq: instseq instruction {
}
| instruction {
}
instruction: statement NEWLINE {

}
| NEWLINE {
    //$$ = nullptr;
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
| RESIZE ID LPAREN expression RPAREN {

}
statement: declaration {

}
| IN expression {

}
| OUT expression {

}
| DEBUGVAR {

}
| expression {

}
declaration: type ID OPERATOR expression {

}
| type ID {

}
funprot: ID LPAREN arglist RPAREN {

}
| ID LPAREN arglist RPAREN RARROW type {

}
arglist: arglist COMMA type ID {

}
| type ID {

}
| /* empty */ {

}
type: INT {

}
| REAL {

}
| CHAR {

}
| STRING {

}
| type POINT expression {

}
expression: expression OPERATOR expression {
}
| OPERATOR expression {

}
| term {

}
term: literal {

}
| LPAREN expression RPAREN {

}
| ID POINT expression {

}
| SIZE ID {

}
literal: INTLIT {

}
| REALLIT {

}
| CHARLIT {

}
| STRLIT {

}
%%