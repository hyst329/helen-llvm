#include <stdio.h>
#include <boost/program_options.hpp>
#include <llvm/ADT/STLExtras.h>
#include "AST.h"
#include "BuiltinFunctions.h"

namespace po = boost::program_options;
using namespace Helen;

int yyparse(AST*& ast);
extern FILE* yyin;

int main(int argc, char** argv)
{
    AST::module = llvm::make_unique<Module>("helen-module", getGlobalContext());
    BuiltinFunctions::createMainFunction();
    yyin = (argc == 1) ? stdin : fopen(argv[1], "r");
    AST* result;
    yyparse(result);
    result->codegen();
    AST::module->dump();
    return 0;
}