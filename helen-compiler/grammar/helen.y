%{
#include "../AST.h"
#include "../Error.h"
#include <float.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <memory>
#include "helen.parser.hpp"

using namespace Helen;

extern int yylex();
void yyerror(Helen::AST* ast, const char*);



const std::string operatorMarker = "__operator_";
const std::string unaryOperatorMarker = "__unary_operator_";

std::map<std::string, double> prec = {
    {operatorMarker + "^",  9},
    {operatorMarker + "*",  8},
    {operatorMarker + "/",  8},
    {operatorMarker + "%",  8},
    {operatorMarker + "!",  8},
    {operatorMarker + "+",  7},
    {operatorMarker + "-",  7},
    {operatorMarker + "<",  6},
    {operatorMarker + "<=", 6},
    {operatorMarker + ">",  6},
    {operatorMarker + ">=", 6},
    {operatorMarker + "==", 5},
    {operatorMarker + "!=", 5},
    {operatorMarker + "&",  4},
    {operatorMarker + "@",  3},
    {operatorMarker + "|",  2},
    {operatorMarker + "=",  1}
};
std::set<std::string> rightAssoc = {operatorMarker + "^", operatorMarker + "="};

static bool lastTerm = 0;

%}
%token IF ELSE ENDIF
%token LOOP ENDLOOP
%token FUN ENDFUN DECLARE STYLE
%token SIZE RESIZE
%token RETURN
%token IN OUT DEBUGVAR
%token USE MAINMODULE
%token TYPE ENDTYPE INT REAL LOGICAL CHAR STRING
%token NEW DELETE PTR
%token INTLIT REALLIT CHARLIT STRLIT
%token ID OPERATOR
%token NEWLINE
%token LPAREN RPAREN
%token LARROW RARROW
%token SEMI COLON COMMA POINT
%type<ast> program
%type<ast> instseq
%type<ast> instruction
%type<ast> statement
%type<ast> expression
%type<ast> term
%type<ast> literal
%type<ast> funprot
%type<ast> declaration
%type<ast> property
%type<type> type
%type<arglist> arglist
%type<exprlist> exprlist
%type<exprlist> properties
%type<vstr> style
%type<vstr> OPERATOR
%type<vstr> ID
%type<vstr> qid
%type<vint> INTLIT
%type<vreal> REALLIT
%type<vchar> CHARLIT
%type<vstr> STRLIT
%right OPERATOR
%start program
%parse-param {Helen::AST *&result}

%code requires {
    struct NamedArgs {
        std::vector<Type*> types;
        std::vector<std::string> names;
    };
}

%union
{
    char *vstr;
    double vreal;
    uint64_t vint;
    char vchar;
    Helen::AST *ast;
    llvm::Type* type;
    NamedArgs* arglist;
    std::vector<shared_ptr<Helen::AST> >* exprlist;
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
                          shared_ptr<Helen::AST>(new Helen::ConstantIntAST(0)));
}
| IF expression NEWLINE instseq ELSE NEWLINE instseq ENDIF {
    $$ = new ConditionAST(shared_ptr<Helen::AST>($2),
                          shared_ptr<Helen::AST>($4),
                          shared_ptr<Helen::AST>($7));
}
| LOOP statement COMMA expression COMMA statement NEWLINE instseq ENDLOOP {
    $$ = new LoopAST(shared_ptr<Helen::AST>($2),
                     shared_ptr<Helen::AST>($4),
                     shared_ptr<Helen::AST>($6),
                     shared_ptr<Helen::AST>($8));
}
| LOOP expression NEWLINE instseq ENDLOOP {
    $$ = new LoopAST(shared_ptr<Helen::AST>(0),
                     shared_ptr<Helen::AST>($2),
                     shared_ptr<Helen::AST>(0),
                     shared_ptr<Helen::AST>($4));
}
| FUN funprot NEWLINE instseq ENDFUN {
    $$ = new FunctionAST(shared_ptr<FunctionPrototypeAST>(dynamic_cast<FunctionPrototypeAST*>($2)), shared_ptr<AST>($4));
}
| DECLARE funprot {
    $$ = $2;
}
| RETURN expression {
    $$ = new ReturnAST(shared_ptr<AST>($2));
}
| RESIZE ID LPAREN expression RPAREN {
    $$ = new FunctionCallAST("__resize", {shared_ptr<AST>($4)});
}
| TYPE ID OPERATOR type {
    if(strcmp($3, "=")) Error::error(ErrorType::UnexpectedOperator, {$3});
    if(AST::types.find($2) != AST::types.end()) Error::error(ErrorType::TypeRedefined, {$2});
    AST::types[$2] = $4;
    $$ = new NullAST();
}
| TYPE ID NEWLINE properties ENDTYPE {
    if(AST::types.find($2) != AST::types.end()) Error::error(ErrorType::TypeRedefined, {$2});
    $$ = new CustomTypeAST($2, *$4);
    ((CustomTypeAST*)$$)->compileTime();
}
| MAINMODULE {
    AST::isMainModule = true;
    $$ = new NullAST();
}
| DELETE ID {
    $$ = new DeleteAST($2);
}
qid: qid COLON ID {
    $$ = strdup((std::string($1) + "-" + $3).c_str());
}
| ID {
    $$ = strdup($1);
}
properties: properties property NEWLINE {
    $1->push_back(shared_ptr<AST>($2));
    $$ = $1;
}
| property NEWLINE {
    $$ = new vector<shared_ptr<AST> >;
    $$->push_back(shared_ptr<AST>($1));
}
property: type ID {
    $$ = new DeclarationAST($1, $2);
}
statement: declaration {
    $$ = $1;
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
    $$ = $1;
}
declaration: type ID OPERATOR expression {
    if(strcmp($3, "=")) Error::error(ErrorType::UnexpectedOperator, {$3});
    $$ = new DeclarationAST($1, $2, shared_ptr<AST>($4));
}
| type ID {
    $$ = new DeclarationAST($1, $2);
}
funprot: ID LPAREN arglist RPAREN style {
    $$ = new FunctionPrototypeAST($1, $3->types, $3->names, Type::getVoidTy(getGlobalContext()), $5);
}
| ID LPAREN arglist RPAREN RARROW type style {
    $$ = new FunctionPrototypeAST($1, $3->types, $3->names, $6, $7);
}
style: STYLE LPAREN ID RPAREN {
    $$ = $3;
}
| /* empty */ {
    $$ = "Helen";
}
arglist: arglist COMMA type ID {
    $1->types.push_back($3);
    $1->names.push_back($4);
    $$ = $1;
}
| type ID {
    $$ = new NamedArgs;
    $$->types.push_back($1);
    $$->names.push_back($2);
}
| /* empty */ {
    $$ = new NamedArgs;
}
type: INT {
    $$ = llvm::Type::getInt64Ty(getGlobalContext());
}
| REAL {
    $$ = llvm::Type::getDoubleTy(getGlobalContext());
}
| CHAR {
    $$ = llvm::Type::getInt8Ty(getGlobalContext());
}
| STRING {
    $$ = llvm::Type::getInt8PtrTy(getGlobalContext());
}
| LOGICAL {
    $$ = llvm::Type::getInt1Ty(getGlobalContext());
}
| ID {
    if(AST::types.find($1) == AST::types.end()) Error::error(ErrorType::UndeclaredType, {$1});
    $$ = $$ = llvm::PointerType::get(AST::types[$1], 0);
}
| INT LPAREN INTLIT RPAREN {
    $$ = llvm::IntegerType::get(getGlobalContext(), $3);
}
| type POINT INTLIT {
    $$ = llvm::ArrayType::get($1, $3);
}
| PTR type {
    $$ = llvm::PointerType::get($2, 0);
}
exprlist: exprlist COMMA expression {
    $1->push_back(shared_ptr<AST>($3));
    $$ = $1;
}
| expression {
    $$ = new vector<shared_ptr<AST> >;
    $$->push_back(shared_ptr<AST>($1));
}
| /* empty*/ {
    $$ = new vector<shared_ptr<AST> >;
}
expression: expression OPERATOR expression {
    
    double last = DBL_MAX;
    if(dynamic_cast<FunctionCallAST*>($3) && !lastTerm)
        last = prec.find(((FunctionCallAST*)($3))->getFunctionName()) == prec.end() ?
               INT_MAX : prec[((FunctionCallAST*)($3))->getFunctionName()];
      double added = prec.find(operatorMarker + $2) == prec.end() ?
                   -DBL_MAX : prec[operatorMarker + $2];
      //printf("Added: %g; Last: %g\n", added, last);
    if(rightAssoc.count(operatorMarker + $2) ? added <= last : added < last) {
        std::vector<shared_ptr<AST> > v = { shared_ptr<AST>($1), shared_ptr<AST>($3) };
        $$ = new FunctionCallAST(operatorMarker + $2, v);
    }
    else {
        shared_ptr<AST> tmp = ((FunctionCallAST*)($3))->getArguments()[0];
        ((FunctionCallAST*)($3))->getArguments()[0] = 
        shared_ptr<AST>(new FunctionCallAST(operatorMarker + $2, {shared_ptr<AST>($1), tmp}));
        $$ = $3;
    }
    lastTerm = 0;
    //$$ = new FunctionCallAST(operatorMarker + $2,
    //                         {shared_ptr<AST>($1), shared_ptr<AST>($3)});
}
| OPERATOR expression {
    $$ = new FunctionCallAST(unaryOperatorMarker + $1,
                            {shared_ptr<AST>($2)});
}
| term {
    $$ = $1;
    lastTerm = 1;
}
term: literal {
    $$ = $1;
}
| LPAREN expression RPAREN {
    $$ = $2;
}
| ID LPAREN exprlist RPAREN {
    $$ = new FunctionCallAST($1, *$3);
}
| ID {
    $$ = new VariableAST($1);
}
| ID POINT term {
    $$ = new FunctionCallAST("__index",
                             {shared_ptr<AST>(new VariableAST($1)), shared_ptr<AST>($3)});
}
| SIZE ID {
    $$ = new FunctionCallAST("__size", {shared_ptr<AST>(new VariableAST($2))});
}
| NEW type {
    $$ = new NewAST($2);
}
literal: INTLIT {
     $$ = new ConstantIntAST($1);
}
| INTLIT COLON INTLIT {
     $$ = new ConstantIntAST($1, $3);
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