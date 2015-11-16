#include <stdio.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include "AST.h"
#include "Error.h"
#include "BuiltinFunctions.h"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO.h>

namespace po = boost::program_options;
using namespace Helen;
using namespace std;

vector<string> includePaths;

int yyparse(AST*& ast);
extern FILE* yyin;

int main(int argc, char** argv)
{
    // Boost Program Options setup
    po::options_description desc("Compiler options");
    desc.add_options()
    ("help,h", "display this help")
    ("version,v", "display version")
    ("input-file", po::value<string>(), "input file")
    ("include-path,I", po::value<vector<string> >(), "path to include files")
    ;
    po::positional_options_description p;
    p.add("input-file", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);
    if (vm.count("help") || vm.empty()) {
        cout << desc << "\n";
        return 1;
    }
    if(vm.count("include-path"))
        includePaths = vm["include-path"].as<vector<string> >();
    includePaths.push_back(".");
    //TODO: add stdlib dirs to include paths
    AST::module = llvm::make_unique<Module>("__helenmodule__", getGlobalContext());
    AST::fpm = llvm::make_unique<legacy::FunctionPassManager>(AST::module.get());
    AST::dataLayout = llvm::make_unique<DataLayout>(AST::module.get());
    AST::fpm->add(createBasicAliasAnalysisPass());
    AST::fpm->add(createPromoteMemoryToRegisterPass());
    AST::fpm->add(createInstructionCombiningPass());
    AST::fpm->add(createReassociatePass());
    AST::fpm->add(createGVNPass());
    AST::fpm->add(createCFGSimplificationPass());
    //AST::fpm->add(createFunctionInliningPass());
    legacy::PassManager pm;
    pm.add(createAlwaysInlinerPass());
    AST::fpm->doInitialization();
    yyin = fopen(vm["input-file"].as<string>().c_str(), "r");
    AST::isMainModule = false;
    AST* result;
    yyparse(result);
    if (Error::errorFlag) { // don't even try to compile if syntax errors present
        fprintf(stderr, "Fatal errors detected: translation terminated\n");
        return 1;
    }
    BuiltinFunctions::createMainFunction(AST::isMainModule);
    result->codegen();
    if (Error::errorFlag) {
        fprintf(stderr, "Fatal errors detected: translation terminated\n");
        return 1;
    }
    AST::builder.CreateRet(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0));
    pm.run(*(AST::module.get()));
    AST::module->dump();
    //TODO: Add output file to program options
    string filename = (argc >= 3) ? argv[2] : (argv[1] + string(".bc"));
    std::error_code ec;
    raw_fd_ostream fdos(filename, ec, sys::fs::OpenFlags::F_None);
    WriteBitcodeToFile(AST::module.get(), fdos);
    return 0;
}
