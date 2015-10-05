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
    AST::fpm = llvm::make_unique<legacy::FunctionPassManager>(AST::module.get());
    AST::fpm->add(createBasicAliasAnalysisPass());
    AST::fpm->add(createInstructionCombiningPass());
    AST::fpm->add(createReassociatePass());
    AST::fpm->add(createGVNPass());
    AST::fpm->add(createCFGSimplificationPass());
    AST::fpm->doInitialization();
    BuiltinFunctions::createMainFunction();
    yyin = (argc == 1) ? stdin : fopen(argv[1], "r");
    AST* result;
    yyparse(result);
    result->codegen();
    AST::module->dump();
    return 0;
}