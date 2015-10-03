%{
#include "../AST.h"
#include "helen.parser.hpp"
#include <vector>
#include <string>
#include <memory>

using namespace Helen;

extern int yylex();
void yyerror(Helen::AST* ast, const char*);

const std::string operatorMarker = "__operator_";
const std::string unaryOperatorMarker = "__unary_operator_";
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
%type<ast> term
%type<ast> literal
%type<vstr> OPERATOR
%type<vstr> ID
%type<vint> INTLIT
%type<vreal> REALLIT
%type<vchar> CHARLIT
%type<vstr> STRLIT
%right OPERATOR
%start program
%parse-param {Helen::AST *&result}

%union
{
    char *vstr;
    double vreal;
    uint64_t vint;
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
    $$ = new FunctionCallAST("__in", {shared_ptr<AST>($2)});
}
| OUT expression {
    $$ = new FunctionCallAST("__out", {shared_ptr<AST>($2)});
}
| DEBUGVAR {
    $$ = new FunctionCallAST("__debugvar");
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
    $$ = new FunctionCallAST(operatorMarker + $2,
                             {shared_ptr<AST>($1), shared_ptr<AST>($3)});
}
| OPERATOR expression {
    $$ = new FunctionCallAST(unaryOperatorMarker + $1,
                            {shared_ptr<AST>($2)});
}
| term {
    $$ = $1;
}
term: literal {
    $$ = $1;
}
| LPAREN expression RPAREN {
    $$ = $2;
}
| ID {
    $$ = new VariableAST($1);
}
| ID POINT expression {
    $$ = new FunctionCallAST("__index",
                             {shared_ptr<AST>(new VariableAST($1)), shared_ptr<AST>($3)});
}
| SIZE ID {
    $$ = new FunctionCallAST("__size", {shared_ptr<AST>(new VariableAST($2))});
}
literal: INTLIT {
     $$ = new ConstantIntAST($1);
}
| REALLIT {
    $$ = new ConstantRealAST($1);
}
| CHARLIT {
    $$ = new ConstantCharAST($1);
}
| STRLIT {
    $$ = new ConstantStringAST($1);
}
%%