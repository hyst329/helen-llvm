%{
#include "../AST.h"
#include "helen.parser.hpp"
#include <vector>
#include <memory>

using namespace Helen;

extern int yylex();
void yyerror(Helen::AST* ast, const char*);
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
%type<ast> program
%type<ast> instseq
%type<ast> instruction
%type<ast> statement
%type<ast> expression
%right OPERATOR
%start program
%parse-param {Helen::AST *&result}

%union
{
    char *vstr;
    double vreal;
    int vint;
    char vchar;
    Helen::AST *ast;
}
%%
program: instseq {
    $$ = $1;
    result = $$;
}
instseq: instseq instruction {
    (dynamic_cast<SequenceAST*>($1))->getInstructions().push_back(shared_ptr<Helen::AST>($2));
    $$ = $1;
}
| instruction {
    $$ = new SequenceAST();
    (dynamic_cast<SequenceAST*>($$))->getInstructions().push_back(shared_ptr<Helen::AST>($1));
}
instruction: statement NEWLINE {
    $$ = $1;
}
| NEWLINE {
    $$ = new Helen::NullAST;
}
| IF expression NEWLINE instseq ENDIF {
    $$ = new ConditionAST(shared_ptr<Helen::AST>($2),
                          shared_ptr<Helen::AST>($4),
                          shared_ptr<Helen::AST>(new Helen::NullAST));
}
| IF expression NEWLINE instseq ELSE NEWLINE instseq ENDIF {
    $$ = new ConditionAST(shared_ptr<Helen::AST>($2),
                          shared_ptr<Helen::AST>($4),
                          shared_ptr<Helen::AST>($7));
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
| ID {
    
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