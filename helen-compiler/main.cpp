#include <stdio.h>
#include <boost/program_options.hpp>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include "AST.h"
#include "BuiltinFunctions.h"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

namespace po = boost::program_options;
using namespace Helen;

int yyparse(AST*& ast);
extern FILE* yyin;
namespace llvm
{
Pass* createAlwaysInlinerPass(); // TODO: search for include file
}

int main(int argc, char** argv)
{
    AST::module = llvm::make_unique<Module>("helen-module", getGlobalContext());
    AST::fpm = llvm::make_unique<legacy::FunctionPassManager>(AST::module.get());
    AST::fpm->add(createBasicAliasAnalysisPass());
    AST::fpm->add(createPromoteMemoryToRegisterPass());
    AST::fpm->add(createInstructionCombiningPass());
    AST::fpm->add(createReassociatePass());
    AST::fpm->add(createGVNPass());
    AST::fpm->add(createCFGSimplificationPass());
    //AST::fpm->add(createAlwaysInlinerPass());
    AST::fpm->doInitialization();
    BuiltinFunctions::createMainFunction();
    yyin = (argc == 1) ? stdin : fopen(argv[1], "r");
    AST* result;
    yyparse(result);
    result->codegen();
    AST::builder.CreateRet(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0));
    AST::module->dump();
    string filename = (argc >= 3) ? argv[2] : (argv[1] + string(".bc"));
    std::error_code ec;
    raw_fd_ostream fdos(filename, ec, sys::fs::OpenFlags::F_None);
    WriteBitcodeToFile(AST::module.get(), fdos);
    return 0;
}