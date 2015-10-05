#include "BuiltinFunctions.h"
#include "AST.h"
#include "FunctionNameMangler.h"

namespace Helen
{

using namespace llvm;

void BuiltinFunctions::createMainFunction()
{
    vector<Type*> v;
    FunctionType* ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()), v, false);
    string name = FunctionNameMangler::mangleName("main", v);
    Function* f = Function::Create(ft, Function::ExternalLinkage, name, AST::module.get());
    AST::functions[name] = f;
    BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "start", f);
    AST::builder.SetInsertPoint(bb);
}
}