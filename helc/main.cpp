#include "AST.h"
#include "BuiltinFunctions.h"
#include "Error.h"
#include "cmake/config.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <stdio.h>
#include <sys/stat.h>

namespace fs = boost::filesystem;
using namespace Helen;
using namespace std;

vector<string> includePaths;

int yyparse(AST *&ast);
extern FILE *yyin;

inline bool exists_test(const std::string &name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

bool checkPath(string relative, string &absolute) {
  for (string p : includePaths)
    if (exists_test(p + "/" + relative)) {
      absolute = p + "/" + relative;
      return true;
    }
  return false;
}

int main(int argc, char **argv) {
  cl::OptionCategory helcOptions("Helen compiler options");
  cl::opt<string> inputFilename(cl::Positional, cl::desc("<input file>"),
                                cl::Required, cl::cat(helcOptions));
  cl::opt<string> outputFilename("o", cl::desc("Specify output filename"),
                                 cl::value_desc("filename"), cl::init("-"),
                                 cl::cat(helcOptions));
  cl::opt<bool> dump("D", cl::desc("Dump LL output to stdout (for debug)"),
                     cl::cat(helcOptions));
  cl::list<string> includePath("I", cl::desc("Path to include files"),
                               cl::cat(helcOptions));
  cl::alias includePathAlias("include-path", cl::desc("same as -I"),
                             cl::cat(helcOptions), cl::aliasopt(includePath));
  cl::ParseCommandLineOptions(argc, argv);
  // Boost Program Options setup
  /*po::options_description desc("Compiler options");
  desc.add_options()
      ("help,h", "display this help")
      ("version,v", "display version")
      ("dump,D", "dump LL output to stdout (for debug)")
      ("input-file", po::value<string>(), "input file")
      ("include-path,I", po::value<vector<string> >(), "path to include files");
  po::positional_options_description p;
  p.add("input-file", -1);
  po::variables_map vm;
  po::store(po::command_line_parser(argc,
  argv).options(desc).positional(p).run(), vm); po::notify(vm);*/
  /*if(vm.count("help") || vm.empty()) {
      cout << desc << "\n";
      return 0;
  }
  if(vm.count("version")) {
      cout << PROJECT_VERSIONED_NAME << "\n";
      return 0;
  }
  if(vm.count("include-path"))
      includePaths = vm["include-path"].as<vector<string> >();*/
  if (!includePath.empty())
    includePaths = includePath;
  includePaths.insert(includePaths.begin(), ".");
  // TODO: add stdlib dirs to include paths
  AST::module = llvm::make_unique<Module>("__helenmodule__", AST::context);
  AST::fpm = llvm::make_unique<legacy::FunctionPassManager>(AST::module.get());
  AST::dataLayout = llvm::make_unique<DataLayout>(AST::module.get());
  // AST::fpm->add(createPromoteMemoryToRegisterPass());
  // AST::fpm->add(createInstructionCombiningPass());
  AST::fpm->add(createReassociatePass());
  AST::fpm->add(createGVNPass());
  AST::fpm->add(createCFGSimplificationPass());
  // AST::fpm->add(createFunctionInliningPass());
  legacy::PassManager pm;
  pm.add(createAlwaysInlinerLegacyPass());
  AST::fpm->doInitialization();
  // yyin = fopen(vm["input-file"].as<string>().c_str(), "r");
  yyin = fopen(inputFilename.c_str(), "r");
  AST::isMainModule = false;
  AST *result;
  yyparse(result);
  if (Helen::Error::errorFlag) { // don't even try to compile if syntax errors
                                 // present
    fprintf(stderr, "Fatal errors detected: translation terminated\n");
    return 1;
  }
  BuiltinFunctions::createMainFunction(AST::isMainModule);
  result->codegen();
  if (Helen::Error::errorFlag) {
    fprintf(stderr, "Fatal errors detected: translation terminated\n");
    return 1;
  }
  AST::builder.CreateRet(ConstantInt::get(Type::getInt32Ty(AST::context), 0));
  pm.run(*(AST::module.get()));
  /*if(vm.count("dump")) {
      AST::module->print(llvm::errs(), nullptr);
      fflush(stdout);
  }*/
  if (dump) {
    AST::module->print(llvm::errs(), nullptr);
    fflush(stdout);
  }
  // FIXME: Replace with positional options
  // string filename = vm["input-file"].as<string>() + ".bc";
  std::error_code ec;
  if (outputFilename == "-")
    outputFilename = inputFilename + ".bc";
  raw_fd_ostream fdos(outputFilename, ec, sys::fs::OpenFlags::F_None);
  WriteBitcodeToFile(*AST::module.get(), fdos);
  return 0;
}
