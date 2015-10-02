#include <stdio.h>
#include <boost/program_options.hpp>
#include <llvm/ADT/STLExtras.h>
#include "AST.h"

namespace po = boost::program_options;

int yyparse(Helen::AST*& ast);
extern FILE* yyin;

int main(int argc, char** argv)
{
    Helen::AST::module = llvm::make_unique<Module>("helen-module", getGlobalContext());
    yyin = (argc == 1) ? stdin : fopen(argv[1], "r");
    Helen::AST* result;
    yyparse(result);
    result->codegen();
    Helen::AST::module->dump();
    return 0;
}