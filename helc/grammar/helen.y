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
%token FUN ENDFUN DECLARE STYLE METHOD OPERATORKW
%token CONSTRUCTOR DESTRUCTOR
%token SIZE RESIZE
%token RETURN
%token IN OUT
%token USE MAINMODULE
%token TYPE ENDTYPE INT REAL LOGICAL CHAR ARRAY INTERFACE ALIAS GENERIC
%token NEW DELETE PTR CAST TO SHIFTBY
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
%type<vstr> basetype
%type<vint> interface
%type<vint> INTLIT
%type<vreal> REALLIT
%type<vchar> CHARLIT
%type<vstr> STRLIT
%right OPERATOR
%start program
%parse-param {Helen::AST *&result}
%define parse.error verbose
%code requires {
    struct NamedArgs {
        std::vector<Type*> types;
        std::vector<std::string> names;
    };
}

%union
{
    char* vstr;
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
| TYPE ALIAS ID OPERATOR type {
    if(strcmp($4, "=")) Error::error(ErrorType::UnexpectedOperator, {$4});
    if(AST::types.find($3) != AST::types.end()) Error::error(ErrorType::TypeRedefined, {$3});
    AST::types[$3] = $5;
    $$ = new NullAST();
}
| TYPE interface ID basetype NEWLINE properties ENDTYPE {
    if(AST::types.find($3) != AST::types.end()) Error::error(ErrorType::TypeRedefined, {$3});
    $$ = new CustomTypeAST($3, *$6, $4, $2);
    ((CustomTypeAST*)$$)->compileTime();
}
| MAINMODULE {
    AST::isMainModule = true;
    $$ = new NullAST();
}
| OPERATORKW OPERATOR USE REALLIT {
    prec[operatorMarker + $2] = $4;
    $$ = new NullAST();
}
| DELETE ID {
    $$ = new DeleteAST($2);
}
basetype: COLON ID {
    $$ = $2;
}
| /* empty */ {
    $$ = "";
}
interface: INTERFACE {
    $$ = 1;
}
| /* empty */ {
    $$ = 0;
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
| DECLARE funprot {
    $$ = $2;
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
    $$ = new FunctionPrototypeAST($1, $3->types, $3->names, Type::getVoidTy(getGlobalContext()), $5, vector<string>());
}
| ID LPAREN arglist RPAREN RARROW type style {
    $$ = new FunctionPrototypeAST($1, $3->types, $3->names, $6, $7, vector<string>());
}
| OPERATORKW OPERATOR LPAREN arglist RPAREN RARROW type {
    if(!prec.count(operatorMarker + $2)) {
        // TODO: Warning
        prec[operatorMarker + $2] = 5; // default value
    }
    $$ = new FunctionPrototypeAST(operatorMarker + $2, $4->types, $4->names, $7, "Helen", vector<string>());
}
| CONSTRUCTOR ID LPAREN arglist RPAREN {
    $$ = new FunctionPrototypeAST("__ctor", $4->types, $4->names,
                                  llvm::Type::getVoidTy(getGlobalContext()), string("__method_") + $2, vector<string>());
}
| DESTRUCTOR ID LPAREN RPAREN {
    $$ = new FunctionPrototypeAST("__dtor", std::vector<Type*>(), std::vector<std::string>(),
                                  llvm::Type::getVoidTy(getGlobalContext()), string("__method_") + $2, vector<string>());
}
style: STYLE LPAREN ID RPAREN {
    $$ = $3;
}
| METHOD LPAREN ID RPAREN {
    if(AST::types.find($3) == AST::types.end()) Error::error(ErrorType::UndeclaredType, {$3});
    $$ = strdup((string("__method_") + $3).c_str());
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
| LOGICAL {
    $$ = llvm::Type::getInt1Ty(getGlobalContext());
}
| ID {
    if(AST::types.find($1) == AST::types.end()) {
        // maybe it's a type currently being declared?
        $$ = 0;
        //Error::error(ErrorType::UndeclaredType, {$1});
    }
    else $$ = llvm::PointerType::get(AST::types[$1], 0);
}
| INT LPAREN INTLIT RPAREN {
    $$ = llvm::IntegerType::get(getGlobalContext(), $3);
}
| ARRAY LPAREN INTLIT RPAREN type {
    $$ = llvm::ArrayType::get($5, $3);
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
| term SHIFTBY term {
    $$ = new ShiftbyAST(shared_ptr<AST>($1), shared_ptr<AST>($3));
}
| NEW type {
    $$ = new NewAST($2);
}
| NEW type LPAREN exprlist RPAREN {
    $$ = new NewAST($2, *$4);
}
| CAST expression TO type {
    $$ = new CastAST(shared_ptr<AST>($2), $4);
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